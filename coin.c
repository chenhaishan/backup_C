#include <stdio.h>

int main(){
	unsigned long long l, r;
	l = r = 1;
	for(unsigned long long i = 1; i != 60; ++i){
		unsigned long long t = r*2 + l;
		r = l;
		l = t;
	} 
	
	printf("l:r = %lld : %lld\n", l, r);
	return 0;
}
