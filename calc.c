#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
	time, // seg
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
	while (ch >= 'a' && ch <= 'z') {
		str[count++] = (char) ch;
		nextch();
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
		KW("s",  time, 1);
		KW("min",time, 60);
		KW("h",  time, 3600);
		KW("day",time, 3600*24);
		KW("rad",angle, 3.14159268);
		#undef KW
	}
end:
	return value; 
}

int precedence (char op) {
	#define OP(C, P) case C: return P;
	switch (op) {
	OP('(', 0)
	OP(')', 0)
	OP('f', 0)
	OP('+', 1)
	OP('-', 1)
	OP('*', 2)
	OP('/', 2)
	OP('^', 3)
	}
}

Expr parse_expr () {
	Expr expr;
	expr.items = 0;
	expr.len = 0;

	Item items[100];
	char ops[30];
	struct {
		char* str;
		int args;
	} calls[16];
	int itemc=0, opc=0, callc=0;

	#define pushop() items[itemc++].kind = ops[--opc]

	// Para que los operadores no se salgan
	ops[opc++] = '(';

	ensuretk();

	goto valstate;
	while (token.kind) {
		switch (token.kind) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '^':
				break;
			default:
				error("Expected operator");
				goto clear;
		}

		int p = precedence(token.kind);
		while (p <= precedence(ops[opc-1]))
			pushop();

		ops[opc++] = token.kind;
		nexttk();
	valstate:
		while (token.kind == '(') {
			ops[opc++] = '(';
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
				ops[opc++] = 'f';
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
			char op = ops[opc-1];
			if (op == 'f') {
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
			if (ops[opc-1] != 'f') {
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





//=== Exec ===//

//Object V0 = (Value){unitless, 0};

Object callstack[16];
int stacklen = 0;

#define CHK_K(N, T) callstack[N].kind == T
#define VALUE(N) (Object){.kind = 'n', ._ = {.n = {.n = N}}}
#define GET_N(I) callstack[I]._.n.n

Object V0 = VALUE(0);

Object my_log () {
	if (stacklen < 1 || stacklen > 2 || !CHK_K(0, 'n') || stacklen==2 && !CHK_K(1, 'n')) {
		error("Incorrect arguments");
		return V0;
	}
	double x = GET_N(0);
	double b = stacklen>1? GET_N(1) : 10;
	double r = log(x)/log(b);
	return VALUE(r);
}

Object my_sin () {
	if (stacklen != 1 || !CHK_K(0, 'n')) {
		error("Argument mismatch");
		return V0;
	}
	double n = GET_N(0);
	return VALUE(sin(n));
}

Object my_cos () {
	if (stacklen != 1 || !CHK_K(0, 'n')) {
		error("Argument mismatch");
		return V0;
	}
	double n = GET_N(0);
	return VALUE(cos(n));
}

Object my_tan () {
	if (stacklen != 1 || !CHK_K(0, 'n')) {
		error("Argument mismatch");
		return V0;
	}
	double n = GET_N(0);
	return VALUE(tan(n));
}

Object gcd () {
	if (stacklen == 1 && CHK_K(0, 'r')) {
		error("GCD of arrays unsupported");
		return V0;
	}
	if (stacklen < 1) {
		error("At least one argument required");
		return V0;
	}
	if (!CHK_K(0, 'n')) {
		error("Only number arguments");
		return V0;
	}
	int r = GET_N(0);
	if (r<0) r*=-1;
	int i;
	for (i=1; i<stacklen; i++) {
		if (!CHK_K(i, 'n')) {
			error("Only number arguments");
			return V0;
		}
		int a = GET_N(i);
		if (a<0) a*=-1;
		while (a != 0) {
			int tmp = a;
			a = r%a;
			r = tmp;
		}
	}
	return VALUE(r);
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
	if (stacklen == 1 && CHK_K(0, 'n')) {
		high = GET_N(0);
	} else if (stacklen == 2 && CHK_K(0, 'n') && CHK_K(1, 'n')) {
		low = GET_N(0);
		high = GET_N(1);
	} else if (stacklen == 3 && CHK_K(0,'n') && CHK_K(1,'n') && CHK_K(2,'n')) {
		low = GET_N(0);
		high = GET_N(1);
		step = GET_N(2);
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

typedef Object (*ftype)();
#define FUNCTION(F) (Object){.kind = 'f', ._ = {.f = F}}

struct {
	char *key;
	Object o;
} consts[] = {
	{"log", FUNCTION(my_log)},
	{"sin", FUNCTION(my_sin)},
	{"cos", FUNCTION(my_cos)},
	{"tan", FUNCTION(my_tan)},
	{"gcd", FUNCTION(gcd)},
	{"mcd", FUNCTION(gcd)},
	{"pi", VALUE(3.14159265)},
	{"vec", FUNCTION(vector)},
	{"vector", FUNCTION(vector)},
	{"range", FUNCTION(range)}
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
			stacklen = 0;
			pos -= item.args;
			while (stacklen < item.args)
				callstack[stacklen++] = stack[pos + stacklen];
			stack[pos++] = v._.f();
			stacklen = 0;
			continue;
		}

		Object b = stack[--pos];
		Object a = stack[--pos];

		if (b.kind != 'n' && a.kind != 'n') {
			error("Operands are not numbers");
			return V0;
		}

		Object r = {.kind = 'n'};
		switch (item.kind) {
			case '+':
				r._.n.n = a._.n.n + b._.n.n;
				break;
			case '-':
				r._.n.n = a._.n.n - b._.n.n;
				break;
			case '*':
				r._.n.n = a._.n.n * b._.n.n;
				break;
			case '/':
				r._.n.n = a._.n.n / b._.n.n;
				break;
			case '^':
				r._.n.n = pow(a._.n.n, b._.n.n);
				break;
			default:
				error("Unknown operator %c", item.kind);
				return V0;
		}
		stack[pos++] = r;
	}
	return stack[0];
}

void main (int in_argc, char** in_argv) {
	argc = in_argc;
	argv = in_argv;

	prepareargs();

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


