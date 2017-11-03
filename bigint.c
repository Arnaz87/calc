#include <stdio.h>
#include <stdlib.h>

#define BIGINT_MAX 100
#define BI_MAX BIGINT_MAX
#define RADIX BIGINT_MAX

// En esta librería, con dígito me refiero a digitos RADIX
// es decir cada componente del bigint, no a digitos decimales
// a menos que indique lo contrario

typedef struct bigint {
	int len;
	int plen;
	unsigned char *n;
} bigint;

void bi_init (bigint *this, int n) {
	this->len = 0;

	int x = n;
	while(x>0) {
		this->len++;
		x /= BIGINT_MAX;
	}

	this->plen = (this->len > 0) ? this->len : 1;
	this->n = malloc(this->plen * sizeof(unsigned char));

	int i = 0;
	while (i < this->len) {
		this->n[i++] = n % RADIX;
		n /= RADIX;
	}
}

// Asegurarse de que in enga suficiente espacio para n digitos
void bi_ensure (bigint *in, int n) {
	if (in->plen >= n) return;

	unsigned char *old = in->n;
	in->n = malloc(n * sizeof(unsigned char));
	in->plen = n;

	int i = 0;
	for (;i < in->len; i++) {
		in->n[i] = old[n];
	}

	free(old);
}

// Discards the most significant digits that are zero
// without reallocating the array
void bi_shrink (bigint *in) {
	int i = in->len;
	// termina cuando p[i] > 0
	while (--i>=0 && in->n[i]==0);
	// i es el último digito > 0
	in->len = i+1;
}

// Hace cero todos los dígitos y pone el tamaño full
// el resultado no es un bigint válido, pero lo hace
// más fácil de usar para algunas operaciones
void bi_empty (bigint *this) {
	int i = 0;
	while (i < this->plen)
		this->n[i++] = 0;
	this->len = this->plen;
}

// r = a*b
void bi_mult (bigint *r, bigint *a, bigint *b) {

	bi_ensure(r, a->len + b->len);
	bi_empty(r);

	if (b->len > a->len) {
		bigint *tmp = a;
		a = b; b = tmp;
	}

	int i, j;
	for (i=0; i < b->len; i++) {
		int bn = b->n[i];
		int carry = 0;
		for (j=0; j < a->len; j++) {
			int an = a->n[j];
			int rn = r->n[j+i];
			int n = rn + an*bn + carry;

			r->n[j+i] = n % RADIX;
			carry = n / RADIX;
		}
		if (carry)
			r->n[j+i] = carry;
	}
	bi_shrink(r);
}

// Guarda el resultado de la división en in
// devuelve el sobrante
int bi_div (bigint *in, int d) {
	unsigned char *x = in->n;
	int rem = 0;
	int i = in->len - 1;
	while (i >= 0) {
		int n = RADIX*rem + x[i];
		x[i--] = n/d;
		rem = n%d;
	}
	bi_shrink(in);
	return rem;
}

int printbigint (bigint *in, char *str) {
	char *chars = "0123456789";

	char tmp_str[30];
	int len = 0;

	while ( in->len > 0 ) {
		int digit = bi_div(in, 10);
		tmp_str[len++] = chars[digit];
	}

	int i = 0;
	while (i<len) str[i++] = tmp_str[len-i-1];
	return len;
}

int inspect (bigint *this) {
	printf("plen: %d\nlen: %d\n", this->plen, this->len);
	//printf("d.p = %p\n", this->d.p);
	int len = this->plen;
	if (this->len < len)
		len = this->len;

	int i;
	int value = 0;
	for (i=0; i<len; i++) {
		int n = this->n[i];
		printf("p[%d] = %d\n", i, n);
	}
	for (i=len-1; i>=0; i--) {
		value = value*RADIX + this->n[i];
	}
	printf("value: %d\n\n", value);
}

void main () {
	bigint num;
	bi_init(&num, 102);

	/*char str[10];
	int len = printbigint(&num, str);
	str[len] = 0;
	printf("n: %s\n", str);

	bi_init(&num, 2017);
	len = printbigint(&num, str);
	str[len] = 0;
	printf("n: %s\n", str);*/

	bigint a, b;
	bi_init(&a, 1111);
	bi_init(&b, 15);
	bi_mult(&num, &a, &b);
	inspect(&a);
	inspect(&b);
	inspect(&num);
	/*len = printbigint(&num, str);
	str[len] = 0;
	printf("2017*0 = %s\n", str);*/
}
