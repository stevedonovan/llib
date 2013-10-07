/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <string.h>
#include <llib/scan.h>

void dump(void *d, double x) { printf("%f\n",x); }

void dumpall(double *x, int len) {
    for (int i = 0; i < len; ++i)
        printf("%f ",x[i]);
    printf("\n");
}

#define BUFSZ 128

#define set(var,value) (obj_unref(var),var = value)

int main() {
    ScanState *ts;
    char buff[BUFSZ];
    double values[100];
    int t;
    ts = scan_new_from_file("test1.dat");
    scan_numbers_fun(ts,dump,NULL);
    // scan_numbers goes over the whole stream;
    // using scan_next_line can make it work over current line
    set (ts,scan_new_from_file("test1.dat"));
    while (scan_next_line(ts)) {
        int n = scan_numbers(ts,values,100);
        dumpall(values,n);
    }

    set(ts,scan_new_from_string("hello = (10,20,30)"));
    puts(scan_next_iden(ts,buff,BUFSZ));
    printf("%c \n",scan_next(ts));
    // after this, next call to scan_next will return this token again
    if (scan_next(ts) == '(') {
        scan_push_back(ts);
    }
    while (scan_next(ts)) {
        printf("%d \n",ts->type);
    }

    // words, strings and numbers
    set(ts,scan_new_from_string("here 'we go' again 10 "));
    while (scan_next(ts)) {
        printf("%d '%s'\n",ts->type,scan_get_str(ts));
    }

    // extracting C strings from this file
    set(ts,scan_new_from_file("test-scan.c"));
   // set this if you want to decode C string escapes
   // scan_set_flags(ts,C_STRING);
    if (ts) {
        while (scan_next(ts)) {
            if (ts->type == T_STRING) {
                scan_get_tok(ts,buff,BUFSZ);
                printf("string '%s'\n",buff);
            }
        }
    }

    set(ts,scan_new_from_file("test.cfg"));
    scan_set_line_comment(ts,"#");
    while (scan_next(ts)) {
      scan_get_tok(ts,buff,BUFSZ);
      printf("key %s ",buff);
      scan_next(ts);
      scan_get_line(ts,buff,BUFSZ);
      printf("value '%s'\n",buff);
   }
   unref(ts);
   printf("kount = %d\n",obj_kount());

}
