#include <stdio.h>


void strcat2(char *s, char *t);

int main(){

    char s[] = "string1", t[] = "string3";


    strcat2(s, t);
    printf("%s\n", s);




return 0;

}

void strcat2(char *s, char *t){

    while(*s){
        s++;;
    }


    while ( *t ){
        *s = *t;
        s++;
        t++;
    }
}

