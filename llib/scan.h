#ifndef LLIB_SCAN_H
#define LLIB_SCAN_H
#include <stdio.h>
#include "obj.h"

typedef enum {
   T_END, T_EOF=0,
   T_TOKEN, T_IDEN=1,
   T_NUMBER,
   T_DOUBLE,
   T_INT,
   T_HEX,
   T_OCT,
   T_STRING,
   T_CHAR,
   T_NADA
} ScanTokenType;

typedef enum {
   C_IDEN = 1,
   C_NUMBER = 2,
   C_STRING = 4,
   C_WSPACE = 16
} ScanFlagType;

typedef void (*ScanNumberFun) (void *data, double x);
typedef void (*ScanStringFun) (void *data, char *buff);

// public fields of ScanState are `line`, `type` and `int_type`
typedef struct {
    int line;
    ScanTokenType type;
    ScanTokenType int_type;
#ifdef _INSIDE_SCAN_C
_INSIDE_SCAN_C
#endif
}  ScanState;

ScanState *scan_new_from_string(const char *str);
ScanState *scan_new_from_file(const char *fname);
ScanState *scan_new_from_stream(FILE *stream);

void scan_set_str(ScanState* ts, const char *str);
void scan_set_flags(ScanState* ts, int flags);
void scan_set_line_comment(ScanState *ts, const char *cc);

bool scan_fetch_line(ScanState* ts, int skipws);
void scan_force_line_mode(ScanState* ts);
void scan_push_back(ScanState* ts);
char scan_getch(ScanState* ts);
bool scan_skip_whitespace(ScanState* ts);
void scan_skip_space(ScanState* ts);
void scan_skip_digits(ScanState* ts);
ScanTokenType scan_next(ScanState* ts);
char *scan_get_line(ScanState *ts, char *buff, int len);
const char *scan_next_line(ScanState *ts);
char *scan_get_tok(ScanState* ts, char *tok, int len);
char *scan_get_str(ScanState* ts);
double scan_get_number(ScanState* ts);
bool scan_skip_until(ScanState *ts, ScanTokenType type);
bool scan_next_number(ScanState *ts, double *val);
char *scan_next_iden(ScanState *ts, char *buff, int len);
bool scan_next_item(ScanState *ts, ScanTokenType type, char *buff, int sz);
int scan_numbers(ScanState *ts, double *values, int sz);
void scan_numbers_fun(ScanState *ts, ScanNumberFun fn, void *data);
void scan_iden_fun(ScanState *ts, ScanStringFun fn, void *data);
#endif
