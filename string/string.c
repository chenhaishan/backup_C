#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/*
char *strcpy(char *dest, const char *src){
    char *tmp = dest;
    while((*dest++ = *src++) != '\0');
    return tmp;
}

char *strncpy(char *dest, const char *src, size_t count){
    char *tmp = dest;
    while(count){
        if((*tmp = *src) != 0)
            src++;
        tmp++;
        count--;
    }
    return dest;
}



char *strcat(char *dest, const char *src){
    char *tmp = dest;
    while(*dest++ != 0);
    while((*dest++ = *src++) != 0);
    return tmp;
}

char *strncat(char *dest, const char *src, size_t n){

    char *tmp = dest;
    
    if(count){
        while(*dest)
            dest++;
        while((*dest++ = *src++) != 0){
            if(--count == 0){
                *dest = '\0';
                break;
            }
        }
    }
    
    return tmp;
}

*/

int memcmp(const void *cs, const void *ct, size_t count){
    const unsigned char *su1, *su2;
    int res = 0;
    
    for(su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--){
        if((res = *su1 - *su2) != 0)
            break;
    }
    return res;
}

void *memset(void *s, int c, size_t count){
    char *xs = s;
    while(count--)
        *xs++ = c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t count){
    char *tmp = dest;
    const char *s = src;
    while(count--)
        *tmp++ = *s++;
    return dest;
}

char *strdup(const char *s){
    if(s == NULL){
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *copy;
    if(!(copy = malloc(len))
        return NULL;
    memcpy(copy, str, len);
    return copy;
}


char *strReplace(const char *original, const char *substr, const char *replace){
    if(original == NULL || substr == NULL || replace == NULL)
        return original;
    
    char *newstr = strdup(original);
    char *oldstr = NULL, *tok = NULL;
    char *head = newstr;
    
    while((tok = strstr(newstr, substr))){
        oldstr = newstr;
        newstr = malloc(strlen(original)+strlen(replace)-strlen(substr)+1);
        if(newstr == NULL){
            free(oldstr);
            return NULL;
        }
        memcpy(newstr, oldstr, tok-oldstr);
        memcpy(newstr+(tok-oldstr), replace, strlen(replace));
        memcpy(newstr+(tok-oldstr)+strlen(replace), tok+strlen(substr),
                strlen(oldstr)-(tok-oldstr)-strlen(substr));
        memset(newstr+strlen(oldstr)+strlen(replace)-strlen(substr), 0, 1);
        head = newstr + (tok-oldstr) + strlen(replace);
        free(oldstr);
    }
    
    return newstr;
}

/*search s2 in s1, return first match*/
char *strstr(const char *s1, const char *s2){
    size_t l1, l2;
    l2 = strlen(s2);
    if(!l2)
        return (char *)s1;
    
    l1 = strlen(s1);
    while(l1 >= l2){
        l1--;
        if(!memcmp(s1, s2, l2))
                return (char *)s1;
        s1++;
    }
    return NULL;
}

char *strncpy_in(char *dest, const char *src, size_t count){
    char *tmp = dest;
    size_t slsrc = strlen(src);
    size_t sldest = strlen(dest);
    if(sldest < count || slsrc < count){
        count = sldest < slsrc ? sldest : slsrc;
    }
    if(src < dest){
        while(count){
            tmp[count-1] = *(src+count-1);
            count--;
        }
    }
    else{
        return strncpy(dest, src, count);
    }
    
    return dest;
}

int main(){
    char src[] = {'a','b','c','d','e','f','g'};
    char *dest = malloc(sizeof(char) * 10);
    char *n = NULL;
    //char *dest = src+3;//p point to "defg";
    printf("src=%s\ndest=%s\n", src, dest);
    //char *new = strncpy(dest, src, 3);
    //char *new = strncpy_in(dest, src, 3);
    //printf("new string is : %s\n", new);
    //char *new = strncpy_in(src, dest, 3);
    //printf("new string is : %s\n", new);
    //strcpy(dest, src);
    //printf("dest=%s\n", dest);
    //strcpy(src, NULL);
    //printf("src = %s\n", src);
    printf("%d\n", strcmp(src, n));
    printf("%d\n", strcmp(n, src));
    return 0;
}
