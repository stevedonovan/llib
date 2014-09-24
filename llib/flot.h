/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_FLOT_H
#define _LLIB_FLOT_H
#include "json.h"
#include "str.h"
#include "farr.h"
// exposing some details: list_new_ptr is needed for now
#include "list.h"

#define FlotMax 1e100
#define FlotMin -FlotMax

typedef struct flot_ {
    void *series;  // list of Series*
    const char* caption;
    const char *id;
    const char* xtitle;
    const char* ytitle;
    int width;
    int height;
    void *map;
    void *text_marks;
    void *vmap;
} Flot;

typedef struct Series_ {
    const char *name;
    double *X;
    double *Y;
    Flot *plot;
    void *map;
} Series;

extern PValue True, False;

#define flot_new(...) flot_new_(value_map_of_str(__VA_ARGS__))

Flot *flot_new_(PValue options);

void flot_comment (str_t text);
void flot_option(Flot *P, str_t key, const void* value) ;
void flot_text_mark(Flot *P, double x, double y, str_t text);

enum {
    FlotLines = 1,
    FlotPoints = 2,
    FlotBars = 4,
    FlotFill = 8
};

#define flot_series_new(p,x,y,flags,...) flot_series_new_(p,x,y,flags,value_map_of_str(__VA_ARGS__))

Series *flot_series_new_(Flot *p, double *X, double *Y, int flags, PValue options);

void flot_series_option(Series *S, str_t key, const void* value);
void *flot_create(str_t title);
void flot_render(str_t name);
char *flot_rgba(int r, int g, int b,int a);
PValue flot_gradient(str_t start, str_t finish);

#define flot_markings(...) "grid.markings",VA(__VA_ARGS__)

#define flot_line(axis,x,colour,line_width) VMS("color",colour,"lineWidth",VF(line_width), axis,VMS("from",VF(x),"to",VF(x)))
#define flot_vert_line(x,c,w) flot_line("xaxis",x,c,w)
#define flot_horz_line(x,c,w) flot_line("xaxis",x,c,w)

PValue flot_region(str_t axis, double x1, double x2, str_t colour);

#define flot_vert_region(x1,x2,c) flot_region("xaxis",x1,x2,c)
#define flot_horz_region(x1,x2,c) flot_region("yaxis",x1,x2,c)

#define flot_empty list_new_ptr

#endif

