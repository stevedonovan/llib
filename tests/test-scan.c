/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <llib/scan.h>
#include <llib/value.h>

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
        // this gives us a properly ref-counted string...
        char *str = scan_get_str(ts);
        printf("%d '%s'\n",ts->type,str);
        unref(str);
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

    ////// scan_scanf //////

    const char *xml = "<boo a='woo' b='doll'>bonzo dog <(10,20,30),(1,2,3);";

    set(ts, scan_new_from_string(xml));
    //scan_set_flags(ts,C_NUMBER) is currently not consistent with scan_scanf!
    char *tag, *attrib, *value;
    scan_scanf(ts,"<%s",&tag);
    // %s is any iden, and %q is a _quoted_ string
    while (scan_scanf(ts,"%s=%q",&attrib,&value)) {
        printf("tag '%s' attrib '%s' value '%s'\n",tag,attrib,value);
        dispose(attrib,value);
    }
    dispose(tag);
    assert(ts->type == '>');

    scan_get_upto(ts,"<",buff,BUFSZ);
    scan_advance(ts,-1);
    printf("got '%s' (%c)\n",buff,scan_getch(ts));

    int i,j,k;
    char ch;
    // %d, %f and %c are what you expect...
    while (scan_scanf(ts,"(%d,%d,%d)%c",&i,&j,&k,&ch) && (ch == ',' || ch == ';'))
        printf("values %d %d %d\n",i,j,k);

    assert(ts->type == ';');

    // %l means rest of current line...
    const char *config_data = "A=cool stuff\nB=necessary nonsense\nC=10,20\n";
    set(ts,scan_new_from_string(config_data));
    char *key, *v;
    while (scan_scanf(ts,"%s=%l",&key,&v)) {
        printf("%s='%s'\n",key,v);
        dispose(key,v);
    }

    // %v means read next token as a Value
    config_data = "alpha=1 beta=2 gamma=hello delta='frodo'";
    PValue val;
    set(ts,scan_new_from_string(config_data));
    while (scan_scanf(ts,"%s=%v",&key,&val)) {
        printf("%s=",key);
        if (value_is_string(val))
            printf("'%s'\n",(char*)val);
        else if (value_is_int(val))
            printf("%d\n",*(intptr_t*)val);
        dispose(key,val);
    }
    unref(ts);
    printf("kount = %d\n",obj_kount());

}
