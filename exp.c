#include <stdio.h>
#include <math.h>

int ipow (int b, int e) {
	int r = 1;
	while (e-- > 0) r *= b;
	return r;
}

#define tpow(B,E) printf(#B" ^ "#E" = %d\n", ipow(B,E))

int gcd (int a, int b) {
	if (a<0) a *= -1;
	if (b<0) b *= -1;
	while (b != 0) {
		int tmp = b;
		b = a%b;
		a = tmp;
	}
	return a;
}

#define tgcd(A,B) printf("gcd "#A" "#B" = %d\n", gcd(A,B))

typedef unsigned long long int bitt;

void bitsof (double d) {
	bitt bits = *((bitt*) &d);

	long long mantissa = bits & 0xfffffffffffff;
	int exponent = (bits >> 52) & 0x7ff;
	exponent -= 0x3ff;

	/*printf("%g: %llx\n", d, bits);
	printf(" mantissa: %llx\n", mantissa);
	printf(" exp: %d\n", exponent);*/
}

// en.wikipedia.org/wiki/Cotinued_fraction
// Usando fracciones continuadas, encontrar la primera fracción aproximada
// cuyo denominador es menor o igual que size 
void fround (int *_n, int *_d, int nmax, int dmax) {
	int n = *_n, d = *_d;
	if (nmax == 0) nmax=0x10000000;
	if (dmax == 0) dmax=0x10000000;
	
	int neg = n<0 ^ d<0;
	if (n<0) n *= -1;
	if (d<0) d *= -1;

	int div = gcd(n, d);
	n /= div; d /= div;
	
	int n2 = 0, n1 = 1;
	int d2 = 1, d1 = 0;
	
	while (d>0) {
		int a = n/d; // Parte entera de la fracción
		int tmp = n-(d*a); // Numerador de la fracción menos a
		n = d; d = tmp;  // Recíproco
		
		// El nuevo aproximante
		int n = a*n1 + n2;
		int d = a*d1 + d2;

		// Es muy grande. No guardar y terminar
		if (n > nmax || d > dmax) break;
		
		// Actualizar historial de aproximantes
		n2 = n1; n1 = n;
		d2 = d1; d1 = d;
	}
	*_n = neg ? -n1 : n1;
	*_d = d1;
}

void tround (int n, int d) {
	printf("%d/%d", n, d);
	printf("=%g", (double)n/d);
	fround(&n, &d, 1000, 1000);
	printf(" => %d/%d = %g\n", n, d, (double)n/d);
}

void main () {
	/*tpow(2, 0);
	tpow(2, 1);
	tpow(2, 8);
	tpow(3, 3);
	tgcd(12, 32);
	tgcd(24, 32);
	tgcd(31, 32);

	bitsof(0.125);
	bitsof(0.25);
	bitsof(0.375);
	bitsof(0.5);
	bitsof(0.625);
	bitsof(0.75);
	bitsof(0.875);
	bitsof(1.0);
	bitsof(1.125);
	bitsof(2.0);
	bitsof(2.125);
	bitsof(2.25);
	bitsof(2.375);
	bitsof(2.5);

	printf("%x", 1024);

	tround(930249, 416020); // 2889/1292 sqrt(5)
	tround(5419351, 1725033); // 103993/33102 PI
	*/

	while (1) {
		int bn, bd, en, ed;
		printf("> ");
		scanf("%d/%d %d/%d", &bn, &bd, &en, &ed);

		{
			double b = (double)bn/bd;
			double e = (double)en/ed;
			printf("%g^%g = %g\n", b, e, pow(b, e));
		}
		
		#define TT int

		TT xn = 1;
		TT xd = 1;

		int i;
		for (i=0; i<20; i++) {
			// Delta
			TT A = ipow(bd,en) * ipow(xn,ed);
			TT dn = xn*( ipow(bn,en) * ipow(xd,ed) - A );
			TT dd = xd*ed*A;
			fround(&dn, &dd, 1000, 0);

			// x + delta
			TT _xn = xn*dd + xd*dn;
			TT _xd = xd*dd;
			fround(&_xn, &_xd, 3000, 1000);

			// Actualizar
			xn = _xn; xd = _xd;

			/*int An = bn * ipow(xd, n-1);
			int Ad = bd * ipow(xn, n-1);
			int Bn = An*xd - Ad*xn;
			int Bd = n * An * xd;

			xn += Bn; xd += Bd;*/

			/*TT _xn = xn*xn*bd + xd*xd*bn;
			TT _xd = 2*xd*xn*bd;

			xn = _xn; xd = _xd;

			//tround(xn, xd);
			fround(&xn, &xd, 5000);*/

			printf(" %d: %g\t%d/%d\n", i, (double)xn/xd, xn, xd);
		}
	}
	
}

