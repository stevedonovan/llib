#include <stdio.h>
#include <llib/scan.h>
#include <llib/value.h>

int main() {
    char buff[128];
    ScanState *ts = scan_new_from_file("subst.c");
    scan_set_flags(ts,C_WSPACE | C_STRING_QUOTE);
    FILE *out = fopen("subst.co","w");
    while (scan_next(ts)) {
        scan_get_tok(ts,buff,sizeof(buff));
        fprintf(out,"%s",buff);
    }    
    unref(ts);
    fclose(out);
    return 0;
}


