#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXSIZE 5

struct BIT{
   unsigned int a:30;
   short b:4;
   unsigned char c:2;  
};

int main(){

    printf("%ld\n", sizeof(struct BIT));
    struct BIT sb = {0x3fffffff,15,2};
    unsigned char *p = (unsigned char*)&sb;
    printf("sb.a = %x\nsb.b=%x\nsb.c=%x\n", sb.a, sb.b, sb.c); 
    for(int i = 0; i < sizeof(struct BIT); ++i){
        printf("%x\n", *(p+i));
    }
        
    char buf[MAXSIZE];
    //char *buf;
    while(1){
        //scanf("%s", buf);
        //getline(&buf, 0, stdin);
        fgets(buf, MAXSIZE, stdin);
        
        for(int i = 0; i < sizeof(buf); ++i){
            printf("%d ", buf[i]);
        }
        printf("\n");
        
        printf("strlen(buf)=%ld, sizeof(buf)=%ld\n",
                strlen(buf), sizeof(buf));
        
    }
    
    return 0;
}
