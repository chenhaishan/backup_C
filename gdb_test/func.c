#include "func.h"
int func(int l, int r){
    if(l > r)
        return l-r;
    else
        return r-l;
}
