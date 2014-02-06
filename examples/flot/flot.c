#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <llib/file.h>
#include <llib/list.h>
#include <llib/map.h>
#include <llib/str.h>
#include <llib/template.h>
#define LLIB_JSON_EASY
#include <llib/json.h>

#ifndef STANDALONE_FLOT
#define FLOT_CDN "file:///home/user/dev/flot"
#define JQUERY_CDN FLOT_CDN "/jquery.min.js"
#else
#define FLOT_CDN "http://www.flotcharts.org/flot/"
#define JQUERY_CDN "http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"
#endif

typedef double (*MapFun)(double);

typedef double* farr_t;

double *farr_range(double x1, double x2, double dx) {
    int n = ceil((x2 - x1)/dx);
    double *arr = array_new(double,n);
    int i = 0;
    for (double x = x1; x < x2; x += dx) {
        arr[i++] = x;
    }
    return arr;
}

double *farr_map(double *a, MapFun f) {
    int n = array_len(a);
    double *b = array_new(double,n);
    FOR(i,n) {
        b[i] = f(a[i]);
    }
    return b;
}

static farr_t sample_helper(void *A, int i1, int istep, int *pi2) {
    if (*pi2 == -1)
        *pi2 = array_len(A);
    int n = (*pi2 - i1)/istep;
    return array_new(double,n);    
}

double *farr_sample(double *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

double *farr_sample_float(float *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

double *farr_sample_int(int *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

void farr_scale(double *A, double m, double c) {
    FOR(i,array_len(A))
        A[i] = m*A[i] + c;
}


#define seq_add2(s,x1,x2) {seq_add(s,x1); seq_add(s,x2);}

#define farr_from_seq(s) ((double*)seq_array_ref(s))

#define farr_copy(arr) array_new_copy(double,arr,sizeof(arr)/sizeof(double))

double *farr_2(double x1,double x2) {
    double *res = array_new(double,2);
    res[0] = x1;
    res[1] = x2;
    return res;
}

double *farr_4(double x1,double x2,double x3,double x4) {
    double *res = array_new(double,4);
    res[0] = x1;
    res[1] = x2;
    res[2] = x3;
    res[3] = x4;
    return res;
}

#define FlotMax 1e100
#define FlotMin -FlotMax


typedef struct flot_ {
    List *series;
    const char* caption;
    const char *id;
    const char* xtitle;
    const char* ytitle;
    int width;
    int height;
    Map *map;
    List *text_marks;
} Flot;

typedef struct Series_ {
    const char *name;
    double *X;
    double *Y;
    Flot *plot;
    Map *map;
} Series;

typedef char *Str;
typedef const char *CStr;

static List *plugins = NULL;

static void list_add_unique(List *ls, void *data) {
    if (! list_find(ls,data))
        list_add(ls,data);
}

static double **interleave(double *X, double *Y) {
    int n = array_len(X);
    double **pnts = array_new_ref(double*,n);
    FOR(i,n) {
        pnts[i] = farr_2(X[i],Y[i]);
    }
    return pnts;
}

static void flot_dispose(Flot *P) {
    obj_unref_v(P->series, P->caption, P->xtitle, P->ytitle);
}

PValue True = NULL, False = NULL;
static List *plots = NULL;
static int kount = 1;

//? these go in map.h?
#define map_gets(m,key) map_get(m,(void*)key)
#define map_puts(m,key,val) map_put(m,(void*)key,(void*)val)

static char *splitdot(char *key) {
    char *p = strchr(key,'.');
    if (p) { // sub.key
        *p = '\0';
        return p+1;
    } else 
        return NULL;
}

#define MAX_KEY 128

// key may be in dotted form, e.g. "points.show" - in this case 'points' refers to a submap
// and 'show' is the subkey into that submap.
static void put_submap(Map *map, CStr ckey, const void* value) {
    char key[MAX_KEY];
    strcpy(key,ckey);
    char *subkey = splitdot(key);
    if (! subkey) {
        map_puts(map,key,value);
    } else {
        Map *sub = map_gets(map,key);
        //printf("sub %s %s %p\n",subkey,key,sub);
        if (! sub) {
            sub = map_new_str_ref();
            map_puts(map,key,sub);
        }        
        put_submap(sub,subkey,value);
    }
}

static PValue get_submap(Map *map, CStr ckey) {
    char key[MAX_KEY];
    strcpy(key,ckey);
    char *subkey = splitdot(key);
    if (! subkey) {
        return map_gets(map,key);
    } else {
        Map *sub = map_gets(map,key);
        if (! sub)
            return NULL;
        return get_submap(sub,subkey);
    }
}

static void update_options(Map *map, PValue options) {
    if (! options) return;
    if (options) {
        char **omap = (char**)options;
        for (int i = 0, n = array_len(omap); i < n; i+=2) {
            char *key = omap[i];
            void *value = omap[i+1];
            put_submap(map,key,value);
        }
    }
}

#define flot_new(...) flot_new_(value_map_of_str(__VA_ARGS__))

Flot *flot_new_(PValue options)  {
    Flot *P = obj_new(Flot,flot_dispose);
    if (! True) {
        True = VB(true);
        False = VB(false);
        plots = list_new_ptr();
        plugins = list_new_str();
    }
    P->xtitle = NULL;
    P->ytitle = NULL;
    P->text_marks = NULL;
    P->width = 600;
    P->height = 300;
    P->series = list_new_ref();
    P->map = map_new_str_ref();
    P->id = str_fmt("placeholder%d",kount++);
    update_options(P->map,options);
    P->caption = map_get(P->map,"caption");
    if (get_submap(P->map,"xaxis.mode")) {
        list_add_unique(plugins,"time");
    }
    list_add(plots,P);
    return P;
}

void flot_option(Flot *P, CStr key, const void* value) {
    put_submap(P->map,key,value);
}

typedef struct TextMark_ {
    double x,y;
    const char *text;
} TextMark;

void flot_text_mark(Flot *P, double x, double y, const char *text) {
    if (! P->text_marks) 
        P->text_marks = list_new_ref();
    TextMark *mark = obj_new(TextMark,NULL);
    mark->x = x;
    mark->y = y;
    mark->text = text;
    list_add(P->text_marks, mark);
}

static void Series_dispose(Series *S) {
    obj_unref_v(S->X, S->Y);
}

enum {
    FlotLines = 1,
    FlotPoints = 2,
    FlotBars = 4,
    FlotFill = 8
};

#define flot_series_new(p,x,y,flags,...) flot_series_new_(p,x,y,flags,value_map_of_str(__VA_ARGS__))

Series *flot_series_new_(Flot *p, double *X, double *Y, int flags, PValue options) {
    Series *s = obj_new(Series,Series_dispose);
    s->plot = p;
    s->map = map_new_str_ref();
    if (flags == FlotBars) {
        put_submap(s->map,"bars.show",True);
    } else {
        if (flags & FlotLines) {
            put_submap(s->map,"lines.show",True);
            if (flags & FlotFill) {
                put_submap(s->map,"lines.fill",VF(0.5));
            }
        }
        if (flags & FlotPoints) {
            put_submap(s->map,"points.show",True);
        }
    }
    update_options(s->map,options);
    double **xydata = NULL;
    if (! Y) { 
        // might already be an array in the correct form!
        void **D = (void*)X;
        if (obj_refcount(D[0]) != -1 && array_len(D[0]) > 0) {
            xydata = (double**)X;
        } else { // even values are X, odd values are Y
            double *vv = X;
            int n = array_len(vv);
            double *X = array_new(double,n/2), *Y = array_new(double,n/2);
            for (int i = 0, k = 0; i < n; i+=2,k++) {
                X[k] = vv[i];
                Y[k] = vv[i+1];
            }
            s->X = X;
            s->Y = Y;
            obj_unref(vv);
        }
    } else {
        s->X = obj_ref(X);
        s->Y = obj_ref(Y);
    }
    if (! xydata)
        xydata = interleave(s->X,s->Y);
    map_puts(s->map,"data",xydata);
    if (get_submap(s->map,"points.errorbars")) {
        list_add_unique(plugins,"errorbars");
    }
    list_add(p->series,s);
    return s;
}

void flot_series_option(Series *S, CStr key, const void* value) {
    put_submap(S->map,key,value);
}

void flot_render(CStr name) {
    // load the template...
    char *tpl = file_read_all("flot.tpl",false);
    if (! tpl) {
        perror("template read");
        exit(1);
    }
    
    List *plist = list_new_ref();
    FOR_LIST(iter,plots) {
        Map *pd = map_new_str_ref();
        Flot *P = iter->data;
        map_puts(pd,"title",VS(P->caption));
        map_puts(pd,"width",VI(P->width));
        map_puts(pd,"height",VI(P->height));
        map_puts(pd,"div",VS(P->id));
        
        if (P->text_marks) {
            char **out = strbuf_new();
            FOR_LIST(p,P->text_marks) {
                TextMark *m = p->data;
                strbuf_addf(out,"text_marking(plot_%s,'%s',%f,%f,'%s')\n",P->id,P->id,m->x,m->y,m->text);
            }
            map_puts(pd,"textmarks",strbuf_tostring(out));
        }
    
        // an array of all the data objects of the series
        PValue *series = array_new_ref(PValue,list_size(P->series));
        int i = 0;
        FOR_LIST(p,P->series) {
            Series *S = p->data;
            series[i++] = S->map;
        }        
    
        map_puts(pd,"data",json_tostring(series));
        map_puts(pd,"options",json_tostring(P->map));
        list_add(plist,pd);
    }

    
    // and write the substitution out to the HTML output...
    StrTempl st = str_templ_new(tpl,"@()");
    Map *data = map_new_str_ref();
    map_puts(data,"flot",FLOT_CDN);
    map_puts(data,"jquery",JQUERY_CDN);
    map_puts(data,"title",name); // for now...
    map_puts(data,"plots",plist);
    map_puts(data,"plugins",plugins);
    
    char *res = str_templ_subst_values(st,data);
    FILE *out = fopen(str_fmt("%s.html",name),"w");
    fputs(res,out);
    fclose(out);
}

char *flot_rgba(int r, int g, int b,int a) {
    return str_fmt("rgba(%d,%d,%d,%d)",r,g,b,a);
}

PValue flot_gradient(const char *start, const char *finish) {
    return VMS("colors",VAS(start,finish));
}

#define flot_markings(...) "grid.markings",VA(__VA_ARGS__)

#define flot_line(axis,x,colour,line_width) VMS("color",colour,"lineWidth",VF(line_width), axis,VMS("from",VF(x),"to",VF(x)))
#define flot_vert_line(x,c,w) flot_line("xaxis",x,c,w)
#define flot_horz_line(x,c,w) flot_line("xaxis",x,c,w)

// does not work for flot -run:  undefined symbol 'value_map_of_str'
//~ PValue flot_line(const char *axis, double x, const char *colour, int line_width) {
    //~ return VMS("color",colour,"lineWidth",VF(line_width), axis,VMS("from",VF(x),"to",VF(x)));
//~ }


PValue flot_region(const char *axis, double x1, double x2, const char *colour) {
    Map *m = map_new_str_ref();
    Map *s = map_new_str_ref();
    map_puts(m,"color",VS(colour));
    map_puts(m,axis,s);
    if (x1 > FlotMin)
        map_puts(s,"from",VF(x1));
    if (x2 < FlotMax)
        map_puts(s,"to",VF(x2));
    return m;
    //return value_array_values_(3,"color","#f6f6f6","yaxis",value_array_values_(3,"to",VF(x),NULL),NULL);
}

#define flot_vert_region(x1,x2,c) flot_region("xaxis",x1,x2,c)
#define flot_horz_region(x1,x2,c) flot_region("yaxis",x1,x2,c)


#define flot_empty list_new_ptr

/*
int main()
{
    Flot *P = flot_new("caption", "First Test",
        "grid.backgroundColor",flot_gradient("#FFF","#EEE"),
        // 'nw' is short for North West, i.e. top-left
        "legend.position","nw"
    );

    // Series data must be llib arrays of doubles 
    double X[] = {1,2,3,4,5};
    double Y1[] = {10,15,23,29,31};
    double *xv = array_new_copy(double,X,5);
    double *yv1 = array_new_copy(double,Y1,5);
    // be careful with this one - values must be explicitly floating-point!
    double *yv2 = farr_values(9.0,9.0,25.0,28.0,32.0);
    
    const char *gray = "#f6f6f6";

    flot_series_new(P,xv,yv1, FlotLines,"label","cats",
        "lines.lineWidth",VF(1),
        "lines.fill",VF(0.2)  // specified as an alpha to be applied to line colour
    );
    flot_series_new(P,xv,yv2, FlotPoints,"label","dogs","color","#F00");
    flot_series_new(P,farr_values(2.0,12.0,4.0,25.0),NULL,FlotLines,"label","lizards");

    // this is based on the Flot annotations example
    
    Flot *P2 = flot_new("caption","Second Test",
        "legend.show",False, // no legend
        // and explicitly give ourselves some more vertical room
        "yaxis.min",VF(-2),"yaxis.max",VF(2),
        flot_markings (
            flot_horz_region(1,PlotMax,gray),
            flot_horz_region(PlotMin,-1,gray),
            flot_vert_line(2,"#000",1),
            flot_vert_line(10,"red",1)
        ),
        // grid lines switched off by making them white and the border explicitly black
        "grid.color","#FFF","grid.borderColor","#000",
        // switch off ticks with an empty list/array
        "xaxis.ticks",flot_empty()
    );
    
    flot_text_mark(P2,2,-1.2,"Warming up");

    // can pack X and Y as even and odd values in a single array
    double *vv = array_new(double,40);
    int k = 0;
    FOR(i,20) {
        vv[k++] = i;
        vv[k++] = sin(i);
    }

    flot_series_new(P2,vv,NULL,FlotBars,
       "color","#333", "bars.barWidth",VF(0.5),"bars.fill",VF(0.6)
    );

    flot_render("test");

    return 0;
}
*/