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
				return expr;
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
			nexttk();
			if (token.kind == '(') {
				nexttk();
				calls[callc].str = token.str;
				ops[opc++] = 'f';
				if (token.kind == ')') { 
					calls[callc++].args = 0;
				} else {
					calls[callc++].args = 1;
					goto valstate;
				}
			} else {
				items[itemc].str = token.str;
				items[itemc++].kind = 'v';
			}
		}

		while (token.kind == ')') {
			while (precedence(ops[opc-1]) > 0) pushop();
			if (opc < 2) {
				error("Unmatched closing parenthesis");
				return expr;
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
			while (precedence(ops[opc-1])) pushop();
			if (ops[opc-1] != 'f') {
				error("Misplaced comma");
				return expr;
			}
			calls[callc-1].args++;
			nexttk();
			goto valstate;
		}

		if (token.kind == '\n' || token.kind == ';') {
			consumetk();
			break;
		}
	}	
	while (opc > 1) pushop();
	#undef pushop

	int bytes = itemc * sizeof(Item);
	expr.items = malloc(bytes);
	memcpy(expr.items, items, bytes);
	expr.len = itemc;

	return expr;
}





//=== Exec ===//

Value V0 = (Value){unitless, 0};

Value callstack[16];
int stacklen = 0;

Value my_log () {
	if (stacklen < 1 || stacklen > 2) {
		error("Argument mismatch");
		return V0;
	}
	double x = callstack[0].n;
	double b = stacklen>1? callstack[1].n : 10;
	double r = log(x)/log(b);
	return (Value){.n = r};
}

Value my_sin () {
	if (stacklen != 1) {
		error("Argument mismatch");
		return V0;
	}
	return (Value){.n = sin(callstack[0].n)};
}

Value my_cos () {
	if (stacklen != 1) {
		error("Argument mismatch");
		return V0;
	}
	return (Value){.n = cos(callstack[0].n)};
}

Value my_tan () {
	if (stacklen != 1) {
		error("Argument mismatch");
		return V0;
	}
	return (Value){.n = tan(callstack[0].n)};
}

typedef Value (*ftype)();

struct {
	char *key;
	ftype f;
} funcs[] = {
	{"log", my_log},
	{"sin", my_sin},
	{"cos", my_cos},
	{"tan", my_tan}
};

int fcount = sizeof(funcs) / sizeof(*funcs);

Value eval_expr (Expr expr) {
	int pos = 0;
	Value stack[30];

	int i = 0;
	while (i < expr.len) {
		Item item = expr.items[i++];

		if (item.kind == 'n') {
			stack[pos++] = item.n;
			continue;
		}
		if (item.kind == 'v') {
			if (strcmp(item.str, "pi") == 0) {
				stack[pos++].n = 3.14159265;
				continue;
			}
			error("Unknown variable %s", item.str);
			return V0;
		}
		if (item.kind == 'f') {
			//printf("f: %s(%d)\n", item.str, item.args); return V0;
			int i = 0;
			ftype f = 0;
			for(i=0; i<fcount; i++) {
				if (strcmp(funcs[i].key, item.str) == 0)
					f = funcs[i].f;
			}
			if (!f) {
				error("Unknown function %s", item.str);
				return V0;
			}
			stacklen = 0;
			pos -= item.args;
			while (stacklen < item.args)
				callstack[stacklen++] = stack[pos + stacklen];
			stack[pos++] = f();
			stacklen = 0;
			continue;
		}
		Value a = stack[--pos];
		Value b = stack[--pos];
		Value r;
		switch (item.kind) {
			case '+':
				r.n = a.n + b.n;
				break;
			case '-':
				r.n = a.n - b.n;
				break;
			case '*':
				r.n = a.n * b.n;
				break;
			case '/':
				r.n = a.n / b.n;
				break;
			case '^':
				r.n = pow(a.n, b.n);
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
		Value v = eval_expr(expr);
		printf("%g\n", v.n);
		ensuretk();
	}
}


