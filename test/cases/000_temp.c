// #include <stdio.h>
#define m1(arg,arg2) arg2 ## arg
#ifdef m1
 #define m2 (arg2)
#else
 #define m3
#endif

#if m1(3, a)
 #define TR 
#else
 #define FL
#endif
int main()
{
	m1(12,
		dddddd);
	printf("Hello World!\n");
	if (a=='A')
		a <<= 3;
	return 0;
}

/* test comment */
