#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

int main(void){
    mtrace();
    
    int *a = NULL;
    
    a = malloc(sizeof(int));
    
    if(a == NULL)return 1;
    
    //free(a);
    
    muntrace();
    
    return 0;
}
