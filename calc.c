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

typedef struct {
	char kind; // w: word, v: value, f: function, \0: no token
	Value value;
	char str[16];
} Token;

Token token = { .kind = 0 };

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
		else return n;
		nextch();
	}
}

float readvalue () {
	int n = 0;
	ensurech();
	if (ch == '0') {
		nextch();	
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
		nextch();
		if (ch == '_') nextch();
		if (ch == '.') { st=1; nextch(); }
		if (ch == '\'') { st=2; nextch(); }
	}
	if (period > 1) {
		n -= n/period;
		period--;
	}
	int denom = div*period;
	return (float) n/denom;
}

void readtoken (Token *token) {
	ensurech();

	// strip space
	while (ch == ' ') nextch();

	if ( iseof ) {
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
		nextch();
	}

	if (count) {
		token->str[count] = 0;
		token->kind = 'w';
	} else {
		token->kind = (char) ch;
		consumech();
	}
}

void ensuretk () { if (!token.kind) readtoken(&token); }
void consumetk () { token.kind = 0; }
void nexttk () { readtoken(&token); }

char ops[16];
int opc = 0;
Value values[16];
int valuec = 0;

void execop ();

void dovalue () {
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
		value.v *= sign;
		nexttk();
	} else {
		printf("Expected value\n");
		exit(1);
	}

	if (token.kind == 'w') {
		float v = value.v;	
		#define KW(W, U, X) \
			if (strcmp(token.str, W) == 0) {\
				value.v = X;\
				consumetk();\
				goto end;\
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
end:
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
	int match = 0;
	ensuretk();
	switch (token.kind) {
		case '+':
		case '-':
		case '*':
		case '/': match = 1;
	}
	if (!match) {
		fprintf(stderr, "Expected operator\n");
		exit(1);
	}
	int p = precedence(token.kind);
	while (p <= precedence(ops[opc-1]))
		execop();
	ops[opc++] = token.kind;
	consumetk();
}

Value doline () {
	// El '(' original...
	ops[opc++] = '(';
	
	goto first;
	while (token.kind) {
		dobinop();
	first:
		ensuretk();
		while (token.kind == '(') {
			ops[opc++] = '(';
			nexttk();
		}
		dovalue();
		ensuretk();
		while (token.kind == ')') {
			while (ops[opc-1] != '(')
				execop();
			// Se eliminaron todos los operadores, queda el '('
			opc--;
			nexttk();
		}
		if (token.kind == '!') {
			int i=0;
			printf("ops:");
			while (i < opc) printf(" %c", ops[i++]);
			printf("\nvalues:");
			i=0; while(i < valuec)
				printf(" %.2f", values[i++].v);
			printf("\n");
			nexttk();
		}
		if (token.kind == '\n' || token.kind == ';') {
			consumetk();
			break;
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

	readtoken(&token);
	
	/*while (token.kind) {
		switch (token.kind) {
			case 'w': printf("w: %s\n", token.str); break;
			case 'f': printf("f: %s\n", token.str); break;
			case 'n': printf("n: %f\n", token.value.v); break;
			default: printf("char: %c\n", token.kind);
		}
		nexttk();
	}*/
	
	while (token.kind) {
		Value v = doline();
		printf("%f\n", v.v);
		ensuretk();
	}
}


