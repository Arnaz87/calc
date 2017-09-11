#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Unit {
	unitless,
	angle, // radian
	distance, // metro
	area, // metro2
	volume, // metro3 / litro
	speed, // kmph
	mass, // kg
	time, // seg
	information, // byte
	money, // dólar
};

typedef struct {
	enum Unit unit;
	float v;
} Value;

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

int ch;
int iseof = 0;
void nextchar () {
	if (iseof) return;
	if (!argv) {
		ch = getchar();
		iseof = ch==EOF;
		return;
	}
	ch = word[0];
	word++; // Siguiente letra
	if (ch == '\0') {
		ch = ' ';
		argc--;
		argv++;
		word = argv[0];
		iseof = argc < 1;
	}
}

void stripspace () { while ((ch == ' ' || ch == '\t') && !iseof) nextchar(); }

typedef struct {
	char kind; // w: word, v: value, f: function, \0: no token
	Value value;
	char str[16];
} Token;

Token token1 = { .kind = 0 };
Token token2 = { .kind = 0 };

int readhex () {
	nextchar();
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
		nextchar();
	}
	return n;
}

int readbin () {
	nextchar();
	int n = 0;
	while (1) {
		if (ch == '0')
			n = n*2;
		else if (ch == '1')
			n = n*2+1;
		else return n;
		nextchar();
	}
}

float readvalue () {
	int n = 0;
	if (ch == '0') {
		nextchar();
		if (ch == 'x')
			return (float) readhex();
		if (ch == 'b')
			return (float) readbin();
	}
	int div = 1;
	int period = 1;
	// 0: int, 1: decimal, 2: period
	char st = 0;
	while (ch >= '0' && ch <= '9') {
		n = n*10 + (ch - '0');
		if (st==1) { div *= 10; }
		if (st==2) { period *= 10; }
		nextchar();
		if (ch == '_') nextchar();
		if (ch == '.') { st=1; nextchar(); }
		if (ch == '\'') { st=2; nextchar(); }
	}
	stripspace();
	if (period > 1) {
		n -= n/period;
		period--;
	}
	int denom = div*period;
	return (float) n/denom;
}

void readtoken (Token *token) {
	stripspace();
	if (iseof) {
		token->kind = 0;
		return;
	}

	if (ch >= '0' && ch <= '9') {
		float val = readvalue();
		token->kind = 'n';
		token->value.v = val;
		token->value.unit = unitless;
		return;
	}

	// Intentar leer un nombre
	int count = 0;
	while (ch >= 'a' && ch <= 'z') {
		token->str[count++] = (char) ch;
		nextchar();
	}

	if (count) {
		token->str[count] = 0;
		token->kind = 'w';
	} else {
		token->kind = (char) ch;
		nextchar();
	}
}

void poptoken () {
	if (token1.kind) {
		token1 = token2;
		readtoken(&token2);
	}
}

char ops[16];
int opc = 0;
Value values[16];
int valuec = 0;

void execop ();

void dovalue () {
	float sign = 1;
	Value value;
	if (token1.kind == '-') {
		sign = -1;
		poptoken();
	}
	if (token1.kind == 'n') {
		value = token1.value;
		value.unit = unitless;
		value.v *= sign;
		poptoken();
	} else {
		printf("Expected value\n");
		exit(1);
	}

	if (token1.kind == 'w') {
		float v = value.v;	
		#define KW(W, U, X) \
			if (strcmp(token1.str, W) == 0) {\
				poptoken();\
				value.v = X;\
			}
		KW("mm", distance, v/1000);
		KW("cm", distance, v/100);
		KW("m",  distance, v);
		KW("km", distance, v*1000);
		KW("g",  mass, v/1000);
		KW("kg", mass, v);
		KW("s",  time, v);
		KW("min",time, v*60);
		KW("h",  time, v*3600);
		KW("day",time, v*3600*24);
		KW("rad",angle, v/3.141592);
		#undef KW
	}
	values[valuec++] = value;
}

int precedence (char op) {
	#define OP(C, P) case C: return P;
	switch (op) {
	OP('(', 0)
	OP(')', 0)
	OP('+', 1)
	OP('-', 1)
	OP('*', 2)
	OP('/', 2)
	OP('^', 3)
	}
}

void execop () {
	char op = ops[--opc];
	Value b = values[--valuec];
	Value a = values[--valuec];
	Value r;
	switch (op) {
		case '+': r.v = a.v + b.v; break;
		case '-': r.v = a.v - b.v; break;
		case '*': r.v = a.v * b.v; break;
		case '/': r.v = a.v / b.v; break;
	}
	values[valuec++] = r;
}

void dobinop () {
	int fail = 0;
	switch (token1.kind) {
		case '+':
		case '-':
		case '*':
		case '/': fail = 1;
	}
	if (!fail) {
		fprintf(stderr, "Expected operator\n");
		exit(1);
	}
	int p = precedence(token1.kind);
	while (p <= precedence(ops[opc-1]))
		execop();
	ops[opc++] = token1.kind;
	poptoken();
}

Value doline () {
	// El '(' original...
	ops[opc++] = '(';

	goto first;
	while (token1.kind) {
		dobinop();
	first:
		while (token1.kind == '(') {
			ops[opc++] = '(';
			poptoken();
		}
		dovalue();
		while (token1.kind == ')') {
			while (ops[opc-1] != '(')
				execop();
			// Se eliminaron todos los operadores, queda el '('
			opc--;
			poptoken();
		}
		if (token1.kind == '!') {
			int i=0;
			printf("ops:");
			while (i < opc) printf(" %c", ops[i++]);
			printf("\nvalues:");
			i=0; while(i < valuec)
				printf(" %.2f", values[i++].v);
			printf("\n");
			poptoken();
		}
	}
	// Hasta que quede un operador: el '(' original!
	while (opc > 1) execop();

	return values[valuec-1];
}

void main (int in_argc, char** in_argv) {
	argc = in_argc;
	argv = in_argv;

	prepareargs();
	
	nextchar(); // Leer el primer caracter.
	stripspace();

	readtoken(&token1);
	readtoken(&token2);

	/*switch (token2.kind) {
		case 'w': printf("w: %s\n", token2.str); break;
		case 'f': printf("f: %s\n", token2.str); break;
		case 'n': printf("n: %f\n", token2.value.v); break;
		default: printf("char: %c\n", token2.kind);
	}*/

	Value v = doline();
	printf("%f\n", v.v);
}


