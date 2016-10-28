#include "func.h"
#include <stdio.h>

int main(){
    int left, right;
    scanf("%d %d", &left, &right);
    printf("abs diff between left and right is :%d\n", func(left, right));
    return 0;
}
