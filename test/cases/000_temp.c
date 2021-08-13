// #include <stdio.h>
#define m1(arg) zz ## arg
#ifdef m1
 #define m2 (arg2)
#else
 #define m3
#endif

#if m1(a)
 #define TR 
#else
 #define FL
#endif
int main()
{
	printf("Hello World!\n");
	if (a=='A')
		a <<= 3;
	return 0;
}

/* test comment */
