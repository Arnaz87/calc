#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Bigint en JS en el dominio público
// https://github.com/Evgenus/BigInt/blob/master/src/BigInt.js

// Otra librería JS
// https://github.com/silentmatt/javascript-biginteger

// Números reales basado en la librería anterior
// https://github.com/jtobey/javascript-bignum

#define error(FMT,ARGS...) fprintf(stderr, FMT "\n", ## ARGS)




//=== Types ===//

enum Unit {
	unitless,
	angle, // radian
	distance, // metro
	area, // metro2
	volume, // litro
	speed, // kmph
	mass, // kg
	u_time, // seg
	information, // byte
	money, // dólar
};

typedef struct {
	enum Unit unit;
	double n;
} Value;

typedef struct {
	int len;
	Value *p;
} Vector;

struct Object;

typedef struct Object {
	// n: Número, v: Vector, f: Función
	char kind;
	union {
		Value n;
		Vector v;
		struct Object (*f)();
	} _;
} Object;


typedef struct {
	// n: Número, v: Variable, f: Función (Invocación)
	char kind;
	Value n;
	char *str;
	int args;
} Item;

typedef struct {
	int len;
	Item *items;
} Expr;





//=== Read ===///

int argc = 0;
char **argv = NULL;
char *word = NULL;

void prepareargs () {
	if (argc < 2) {
		argv = NULL;
		return;
	}
	// Saltar el primer argumento, el nombre del ejecutable
	argv++; argc--;
	word = argv[0];
}

int ch = 0;
int iseof = 0;
char getnextchchar () {
	if (iseof) return 0;
	if (!argv) {
		int ch = getchar();
		if (ch==EOF) {
			iseof = 1;
			return 0;
		}
		return (char) ch;
	}
	char ch = word[0];
	word++; // Siguiente letra
	if (ch == '\0') {
		ch = ' ';
		argc--;
		argv++;
		word = argv[0];
		iseof = argc < 1;
	}
	return ch;
}

// Asegurar que haya un caracter leído
void ensurech () { if (!ch) ch = getnextchchar(); }
// Descartar el caracter actual para poder pasar al siguiente
void consumech () { ch = 0; }
// Descartar lo que sea que haya y psar al siguiente caracter
void nextch () { ch = getnextchchar(); }

struct {
	char kind; // w: word, v: value, f: function, \0: no token
	Value value;
	char *str;
} token = { .kind = 0 };


int readhex () {
	nextch(); // el caracter leído es la 'x' de "0x"
	int n = 0;
	while (1) {
		int d = -1;
		if (ch >= '0' && ch <= '9')
			d = ch-'0';
		if (ch >= 'a' && ch <= 'f')
			d = ch-'a'+10;
		if (ch >= 'A' && ch <= 'F')
			d = ch-'A'+10;
		if (d == -1) break;
		n = n*16+ d;
		nextch();
	}
	return n;
}

int readbin () {
	nextch(); // el caracter leído es la 'b' de "0b"
	int n = 0;
	while (1) {
		if (ch == '0')
			n = n*2;
		else if (ch == '1')
			n = n*2+1;
		else break;
		nextch();
	}
	return n;
}

double readvalue () {
	int n = 0;
	ensurech();
	if (ch == '0') {
		nextch();	
		if (ch == 'x') {
			return readhex();
		}
		if (ch == 'b') {
			return readbin();
		}
	}
	int div = 1;
	int period = 1;
	// 0: int, 1: decimal, 2: period
	char st = 0;
	goto point;
	while (ch >= '0' && ch <= '9') {
		n = n*10 + (ch - '0');
		if (st==1) { div *= 10; }
		if (st==2) { period *= 10; }
		nextch();
		if (ch == '_') nextch();
point:
		if (ch == '.') { st=1; nextch(); }
		if (ch == '\'') { st=2; nextch(); }
	}
	if (period > 1) {
		n -= n/period;
		period--;
	}
	int denom = div*period;
	return (double) n/denom;
}

void readtoken () {
	ensurech();

	// strip space
	while (ch == ' ') nextch();

	if ( iseof ) {
		token.kind = 0;
		return;
	}

	if (ch >= '0' && ch <= '9') {
		token.value.n = readvalue();
		token.kind = 'n';
		token.value.unit = unitless;
		return;
	}

	// Intentar leer un nombre
	int count = 0;
	char str[32];

	#define IS_WORD (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_')

	if (count == 0) {
		while (IS_WORD) {
			str[count++] = (char) ch;
			nextch();
		}
	}
	if (count == 0) {
		while (!IS_WORD && !(ch >= '0' && ch <= '9') &&
			ch != ' ' && ch != '\n' && ch != '\t' &&
			ch != ',' && ch != '(' && ch != ')') {
			str[count++] = (char) ch;
			nextch();
		}
	}

	if (count) {
		token.str = malloc(count+1);
		strncpy(token.str, str, count);
		token.str[count] = 0;
		token.kind = 'w';
	} else {
		token.kind = (char) ch;
		consumech();
	}
}

void ensuretk () { if (!token.kind) readtoken(&token); }
void consumetk () { token.kind = 0; }
void nexttk () { readtoken(&token); }






//=== Parse ===//

Value parse_value () {
	float sign = 1;
	Value value;
	ensuretk();
	if (token.kind == '-') {
		sign = -1;
		nexttk();
	}
	if (token.kind == 'n') {
		value = token.value;
		value.unit = unitless;
		value.n *= sign;
		nexttk();
	} else {
		printf("Expected value\n");
		exit(1);
	}

	if (token.kind == 'w') {
		#define KW(W, U, N) \
			if (strcmp(token.str, W) == 0) {\
				value.unit = U;\
				value.n *= N;\
				nexttk();\
				goto end;\
			}
		KW("mm", distance, 0.0001);
		KW("cm", distance, 0.001);
		KW("m",  distance, 1);
		KW("km", distance, 1000);
		KW("g",  mass, 0.0001);
		KW("kg", mass, 1);
		KW("s",  u_time, 1);
		KW("min",u_time, 60);
		KW("h",  u_time, 3600);
		KW("day",u_time, 3600*24);
		KW("rad",angle, 3.14159268);
		#undef KW
	}
end:
	return value; 
}

int precedence (char *op) {
	#define OP(C, P) if (!strcmp(op, C)) return P;
	OP("(", 0)
	OP(")", 0)
	OP("f", 0)

	OP("=", 1)
	OP("<", 1)
	OP(">", 1)
	OP("<=", 1)
	OP(">=", 1)
	OP("!=", 1)

	OP("+", 2)
	OP("-", 2)

	OP("*", 3)
	OP("/", 3)

	OP("div", 3)
	OP("mod", 3)
	OP("rem", 3)

	OP("^", 4)
	OP("**", 4)
	return -1;
}

Expr parse_expr () {
	Expr expr;
	expr.items = 0;
	expr.len = 0;

	Item items[100];
	char *ops[30];
	struct {
		char* str;
		int args;
	} calls[16];
	int itemc=0, opc=0, callc=0;

	#define pushop() items[itemc++] = (Item){.kind = 'f', .args = 2, .str = ops[--opc]}

	// Para que los operadores no se salgan
	ops[opc++] = "(";

	ensuretk();

	goto valstate;
	while (token.kind) {

		int p;
		if (token.kind != 'w' || (p = precedence(token.str)) == -1) {
			error("Expected operator");
			goto clear;
		}
		while (p <= precedence(ops[opc-1]))
			pushop();

		ops[opc++] = token.str;
		nexttk();
	valstate:
		while (token.kind == '(') {
			ops[opc++] = "(";
			nexttk();
		}
		
		if (token.kind == 'n') {
			items[itemc].n = parse_value();
			items[itemc++].kind = 'n';
		} else if (token.kind == 'w') {
			char *str = token.str;
			nexttk();
			if (token.kind == '(') {
				nexttk();
				calls[callc].str = str;
				ops[opc++] = "f";
				if (token.kind == ')') { 
					calls[callc++].args = 0;
				} else {
					calls[callc++].args = 1;
					goto valstate;
				}
			} else {
				items[itemc].str = str;
				items[itemc++].kind = 'v';
			}
		}

		while (token.kind == ')') {
			while (precedence(ops[opc-1]) > 0) pushop();
			if (opc < 2) {
				error("Unmatched closing parenthesis");
				goto clear;
			}
			if (strcmp(ops[opc-1], "f") == 0) {
				callc--;
				items[itemc].kind = 'f';
				items[itemc].str = calls[callc].str;
				items[itemc].args = calls[callc].args;
				itemc++;
			}
			opc--;
			nexttk();
		}

		if (token.kind == ',') {
			nexttk();
			while (precedence(ops[opc-1])) pushop();
			if (strcmp(ops[opc-1], "f") != 0) {
				error("Misplaced comma");
				goto clear;
			}
			calls[callc-1].args++;
			goto valstate;
		}

		if (token.kind == '\n' || token.kind == ';') {
			consumetk();
			break;
		}
	}	
	while (opc > 1 && precedence(ops[opc-1])) pushop();
	if (opc > 1) {
		error("Unclosed parenthesis");
		return expr;
	}
	#undef pushop

	int bytes = itemc * sizeof(Item);
	expr.items = malloc(bytes);
	memcpy(expr.items, items, bytes);
	expr.len = itemc;
	return expr;

clear:
	while(token.kind) {
		if (token.kind == '\n' || token.kind == ';') {
			consumetk();
			break;
		} else nexttk();
	}
	return expr;
}




//=== Vector ===//

#define EMPTY_VEC ((Vector){ .len = 0, .p = 0 })

void add_vec (Vector *vec, Value v) {
	vec->len ++;
	vec->p = realloc(vec->p, vec->len * sizeof(Value));
	vec->p[vec->len-1] = v;
}





//=== Exec ===//

typedef Object (*ftype)();

Object callstack[16];
int stacklen = 0;

#define CHK_K(N, T) callstack[N].kind == T
#define VALUE(N) (Object){.kind = 'n', ._.n.n = N}

Object V0 = (Object) { .kind = 'v', ._.v.len = 0, ._.v.p = 0 };

#define GET_N(NAME, I) { \
	if (callstack[I].kind != 'n') { \
		error("Parameter %d in %s must be a number", I+1, fname); \
		return V0; } \
	NAME = callstack[I]._.n.n; }

//Object V0 = VALUE(0);

char *fname = 0;

// Operators are always called with 2 parameters
#define BINOP(NAME, EXPR) Object NAME () {\
	double a, b; \
	GET_N(a, 0); \
	GET_N(b, 1); \
	return VALUE(EXPR); }

BINOP(add, a+b)
BINOP(sub, a-b)
BINOP(mul, a*b)
BINOP(my_div, a/b)
BINOP(idiv, floor(a/b))
BINOP(my_mod, fmod(a,b))
BINOP(my_pow, pow(a,b))

BINOP(eq, a==b?1:0)
BINOP(lt, a<b?1:0)
BINOP(gt, a>b?1:0)
BINOP(neq, a!=b?1:0)
BINOP(lte, a<=b?1:0)
BINOP(gte, a>=b?1:0)

#define UNOP(NAME, EXPR) Object NAME () {\
	if (stacklen != 1) { \
		error("%s expects 1 parameter", fname); \
		return V0; \
	} \
	double x; GET_N(x, 0); \
	return VALUE(EXPR); }\

UNOP(my_sqrt, sqrt(x))
UNOP(my_sin, sin(x))
UNOP(my_cos, cos(x))
UNOP(my_tan, tan(x))
UNOP(my_asin, asin(x))
UNOP(my_acos, acos(x))
UNOP(my_atan, atan(x))
UNOP(my_abs, fabs(x))
UNOP(my_round, round(x))
UNOP(my_floor, floor(x))
UNOP(my_ceil, ceil(x))
UNOP(my_trunc, trunc(x))
UNOP(sign, x>0?1: x<0?-1: 0)


Object my_log () {
	if (stacklen < 1 || stacklen > 2) {
		error("Argument mismatch in %s", fname);
		return V0;
	}

	double x, b;

	GET_N(x, 0);
	if (stacklen>1) {
		GET_N(b, 1);
	} else { b = 10; }

	double r = log(x)/log(b);
	return VALUE(r);
}

Object gcd () {
	if (stacklen == 1 && CHK_K(0, 'v')) {
		Vector v = callstack[0]._.v;
		int i, r = v.p[0].n;
		for (i=0; i<v.len; i++) {
			int a = v.p[i].n;
			if (a<0) a*=-1;
			while (a != 0) {
				int tmp = a;
				a = r%a;
				r = tmp;
			}
		}
		return VALUE(r);
	}
	if (stacklen < 1) {
		error("%s requires at least one parameter");
		return V0;
	}
	int r; GET_N(r, 0);
	if (r<0) r*=-1;
	int i;
	for (i=1; i<stacklen; i++) {
		int a; GET_N(a, i);
		if (a<0) a*=-1;
		while (a != 0) {
			int tmp = a;
			a = r%a;
			r = tmp;
		}
	}
	return VALUE(r);
}

Object mult ();

Object lcm () {
	int dv = gcd()._.n.n;
	if (stacklen == 1 && CHK_K(0, 'v')) {
		int ml = mult()._.n.n;
		return VALUE(ml/dv);
	}
	int i, r = 1;
	for (i=0; i<stacklen; i++) {
		int a;
		GET_N(a, i);
		r *= a;
	}
	return VALUE(r/dv);
}

Object vector () {
	Vector vec;
	vec.p = malloc(stacklen * sizeof(Value));
	vec.len = stacklen;
	int i;
	for(i=0; i < stacklen; i++) {
		if (!CHK_K(i, 'n')) {
			error("Vectors can only contain numbers");
			free(vec.p);
			return V0;
		}
		vec.p[i] = callstack[i]._.n;
	}
	return (Object){.kind = 'v', ._.v = vec};
}

Object range () {
	double low=1, high=1, step=1;

	if (stacklen == 1) {
		GET_N(high, 0);
	} else if (stacklen == 2) {
		GET_N(low, 0);
		GET_N(high, 1);
	} else if (stacklen == 3) {
		GET_N(low, 0);
		GET_N(high, 1);
		GET_N(step, 2);
	} else {
		error("%s must have 1 to 3 parameters", fname);
		return V0;
	}

	if (step == 0) {
		error("Step cannot be 0");
		return V0;
	}
	if (step < 0) step *= -1;
	if (high < low) step *= -1;
	Vector vec;
	vec.len = 1 + (high - low) / step;
	vec.p = malloc(vec.len * sizeof(Value));
	double n = low;
	int i = 0;
	while (i<vec.len) {
		vec.p[i++].n = n;
		n += step;
	}
	return (Object){.kind = 'v', ._.v = vec};
}

Object factorize () {
	if (stacklen != 1) {
		error("%s expects 1 parameter", fname);
		return V0;
	}
	double _n; GET_N(_n, 0);
	Vector v = EMPTY_VEC;

	if (fmod(_n, 1) != 0)
		return (Object){.kind = 'v', ._.v = v};

	int i=2, n = (int)_n;
	while (n > 1 && i<n) {
		if (n%i == 0) {
			add_vec(&v, (Value){.n = i});
			n /= i;
			i=2;
		} else { i++; }
	}
	if (n > 1) add_vec(&v, (Value){.n = n});
	return (Object){.kind = 'v', ._.v = v};
}

Object map () {
	if (stacklen != 2 || !CHK_K(0, 'v') || !CHK_K(1, 'f')) {
		error("%s expects a vector and a function", fname);
		return V0;
	}
	Vector v = callstack[0]._.v;
	ftype f = callstack[1]._.f;
	int i;
	for (i=0; i<v.len; i++) {
		stacklen = 1;
		callstack[0] = (Object){.kind = 'n', ._.n = v.p[i]};
		Object r = f();
		if (r.kind != 'n') {
			error("mapped functions must return numbers");
			return V0;
		}
		v.p[i] = r._.n;
	}
	return (Object){.kind = 'v', ._.v = v};
}

Object filter () {
	if (stacklen != 2 || !CHK_K(0, 'v') || !CHK_K(1, 'f')) {
		error("%s expects a vector and a function", fname);
		return V0;
	}
	Vector v = callstack[0]._.v;
	ftype f = callstack[1]._.f;

	Vector nv = EMPTY_VEC;
	int i;
	for (i=0; i<v.len; i++) {
		stacklen = 1;
		callstack[0] = (Object){.kind = 'n', ._.n = v.p[i]};
		Object r = f();
		if (r.kind != 'n') {
			error("mapped functions must return numbers");
			return V0;
		}
		if (r._.n.n != 0)
			add_vec(&nv, v.p[i]);
	}
	return (Object){.kind = 'v', ._.v = nv};
}

Object my_rand () {
	if (stacklen == 0) {
		double r = (double)rand() / RAND_MAX;
		return VALUE(r);
	} else if (stacklen == 1 && CHK_K(0, 'v')) {
		Vector v = callstack[0]._.v;
		int i = rand() % v.len;
		return (Object){.kind = 'n', ._.n = v.p[i]};
	} else {
		error("%s expects 0 parameters or a vector");
		return V0;
	}
}

Object len () {
	if (stacklen != 1 || !CHK_K(0, 'v')) {
		error("%s expects a vector");
		return V0;
	}
	Vector v = callstack[0]._.v;
	return VALUE(v.len);
}

Object sum () {
	if (stacklen != 1 || !CHK_K(0, 'v')) {
		error("%s expects a vector");
		return V0;
	}
	Vector v = callstack[0]._.v;
	double sum = 0;
	int i;
	for (i=0; i<v.len; i++)
		sum += v.p[i].n;
	return VALUE(sum);
}

Object mult () {
	if (stacklen != 1 || !CHK_K(0, 'v')) {
		error("%s expects a vector");
		return V0;
	}
	Vector v = callstack[0]._.v;
	double r = 1;
	int i;
	for (i=0; i<v.len; i++)
		r *= v.p[i].n;
	return VALUE(r);
}

Object mean () {
	if (stacklen != 1 || !CHK_K(0, 'v')) {
		error("%s expects a vector");
		return V0;
	}
	Vector v = callstack[0]._.v;
	double sum = 0;
	int i;
	for (i=0; i<v.len; i++)
		sum += v.p[i].n;
	return VALUE(sum / v.len);
}

#define FUNCTION(F) (Object){.kind = 'f', ._ = {.f = F}}
#define FF(SYM, FNM) { #SYM, (Object){.kind = 'f', ._.f = FNM} }

struct {
	char *key;
	Object o;
} consts[] = {
	{"pi", VALUE(3.14159265)},
	{"e", VALUE(2.18281828)},
	{"phi", VALUE(1.68)},

	{"+", FUNCTION(add)},
	{"-", FUNCTION(sub)},
	{"*", FUNCTION(mul)},
	{"/", FUNCTION(my_div)},
	{"div", FUNCTION(idiv)},
	{"rem", FUNCTION(my_mod)},
	{"mod", FUNCTION(my_mod)},
	{"^", FUNCTION(my_pow)},
	{"**", FUNCTION(my_pow)},

	{"=", FUNCTION(eq)},
	{"<", FUNCTION(lt)},
	{">", FUNCTION(gt)},
	{"!=", FUNCTION(neq)},
	{"<=", FUNCTION(lte)},
	{">=", FUNCTION(gte)},

	{"sin", FUNCTION(my_sin)},
	{"cos", FUNCTION(my_cos)},
	{"tan", FUNCTION(my_tan)},
	{"asin", FUNCTION(my_asin)},
	{"acos", FUNCTION(my_acos)},
	{"atan", FUNCTION(my_atan)},

	FF(sign, sign),
	FF(abs, my_abs),
	FF(round, my_round),
	FF(floor, my_floor),
	FF(ceil, my_ceil),
	FF(int, my_trunc),

	{"log", FUNCTION(my_log)},
	{"rand", FUNCTION(my_rand)},

	{"gcd", FUNCTION(gcd)},
	{"mcd", FUNCTION(gcd)},
	{"lcm", FUNCTION(lcm)},
	{"mcm", FUNCTION(lcm)},

	{"vec", FUNCTION(vector)},
	{"vector", FUNCTION(vector)},
	{"range", FUNCTION(range)},
	{"factorize", FUNCTION(factorize)},
	{"map", FUNCTION(map)},
	FF(filter, filter),

	FF(len, len),
	FF(sum, sum),
	FF(mult, mult),
	FF(mean, mean),
};

int const_count = sizeof(consts) / sizeof(*consts);

Object eval_expr (Expr expr) {
	int pos = 0;
	Object stack[30];

	int i = 0;
	while (i < expr.len) {
		Item item = expr.items[i++];

		if (item.kind == 'n') {
			stack[pos++] = (Object){kind: 'n', ._.n = item.n};
			continue;
		}
		if (item.kind == 'f' || item.kind == 'v') {
			Object v = {.kind = 0};
			int i;
			for(i=0; i < const_count; i++) {
				if (strcmp(consts[i].key, item.str) == 0)
					v = consts[i].o;
			}
			if (v.kind == 0) {
				error("Unknown constant %s", item.str);
				return V0;
			}
			if (item.kind == 'v') {
				stack[pos++] = v;
				continue;
			} // item.kind == 'f'
			if (v.kind != 'f') {
				error("%s is not a function", item.str);
				return V0;
			}
			fname = item.str;

			stacklen = 0;
			pos -= item.args;
			while (stacklen < item.args)
				callstack[stacklen++] = stack[pos + stacklen];
			stack[pos++] = v._.f();
			stacklen = 0;
			continue;
		}

		error("Unknown kind %c", item.kind);
		return V0;
	}
	return stack[0];
}

void main (int in_argc, char** in_argv) {
	argc = in_argc;
	argv = in_argv;

	prepareargs();

	srand(time(0));

	printf("Calc\n");

	readtoken(&token);
	
	while (token.kind) {
		Expr expr = parse_expr();
		/*printf("expr: ");
		int i;for(i=0;i<expr.len;i++) printf("%c ",expr.items[i]);
		printf("\n");*/
		Object v = eval_expr(expr);
		if (v.kind == 'n') printf("%g\n", v._.n.n);
		else if (v.kind == 'v') {
			printf("[");
			int i = 0;
			while (i<v._.v.len) {
				if (i>0) printf(", ");
				printf("%g", v._.v.p[i++].n);
			}
		       	printf("]\n");
		}
		ensuretk();
	}
}


