#include <stdio.h>
#include <klee/klee.h>
#define MAXL 6

/*
int foo(int x){

	if(x>0)
		return x;
	else
		return x-1;
}
*/

void bubble( int v[MAXL], int n )
{
	int i, j, k;
	//int tmp;
	//tmp = foo(n);
	if(n >= MAXL)
		return;
	for ( i = n; i > 1; --i )
		for ( j = 1; j < i; ++j )
			if ( v[j] > v[j + 1] )	/* compare */
			{
				k = v[j];	/* exchange */
				v[j] = v[j + 1];
				v[j + 1] = k;
			}
}

int main()
 {
		int v[MAXL];
		int n;

		klee_make_symbolic(&v, sizeof(v), "v");
		klee_make_symbolic(&n, sizeof(n), "n");

		bubble(v,n);
		return 0;
 }
