#include <stdio.h>
#include <string.h>

void func(char *s){
    char *a = NULL;
    strcpy(a, s);
}

int main(){
    char *s = "1234567890";
    func(s);
    return 0;
}
