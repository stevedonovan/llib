#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <llib/file.h>
#include <llib/list.h>
#include <llib/map.h>
#include <llib/str.h>
#include <llib/template.h>
#define LLIB_JSON_EASY
#include <llib/json.h>

#ifndef STANDALONE_FLOT
#define FLOT_CDN "file:///home/user/libs//flot"
#define JQUERY_CDN FLOT_CDN "/jquery.min.js"
#else
#define FLOT_CDN "http://www.flotcharts.org/flot/"
#define JQUERY_CDN "http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"
#endif

typedef double (*MapFun)(double);

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

static double **interleave(double *X, double *Y) {
    int n = array_len(X);
    double **pnts = array_new_ref(double*,n);
    FOR(i,n) {
        double *pnt = array_new(double,2);
        pnt[0] = X[i];
        pnt[1] = Y[i];
        pnts[i] = pnt;
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

static void put_submap(Map *map, CStr key, CStr subkey, const void* value) {
    Map *sub = map_gets(map,key);
    if (! sub) {
        sub = map_new_str_ref();
        map_puts(map,key,sub);
    }
    map_puts(sub,subkey,value);
}

static void update_options(Map *map, PValue options) {
    if (! options) return;
    if (options) {
        char **omap = (char**)options;
        for (int i = 0, n = array_len(omap); i < n; i+=2) {
            char *key = omap[i];
            void *value = omap[i+1];
            char *p = strchr(key,'.');
            if (p) { // sub.key
                char *subkey = p+1;
                *p = '\0';
                put_submap(map,key,subkey,value);
            } else {
                map_put(map,key,value);
            }
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
    }
    P->xtitle = NULL; //str_ref(xtitle);
    P->ytitle = NULL; //str_ref(ytitle);
    P->text_marks = NULL;
    P->width = 600;
    P->height = 300;
    P->series = list_new_ref();
    P->map = map_new_str_ref();
    P->id = str_fmt("placeholder%d",kount++);
    update_options(P->map,options);
    P->caption = map_get(P->map,"caption");
    list_add(plots,P);
    return P;
}

void flot_option(Flot *P, CStr key, CStr subkey, const void* value) {
    put_submap(P->map,key,subkey,value);
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
    PlotLines = 1,
    PlotPoints = 2,
    PlotBars = 4,
    PlotFill = 8
};

#define flot_series_new(p,x,y,flags,...) flot_series_new_(p,x,y,flags,value_map_of_str(__VA_ARGS__))

Series *flot_series_new_(Flot *p, double *X, double *Y, int flags, PValue options) {
    Series *s = obj_new(Series,Series_dispose);
    s->plot = p;
    s->map = map_new_str_ref();
    if (flags == PlotBars) {
        put_submap(s->map,"bars","show",True);
    } else {
        if (flags & PlotLines) {
            put_submap(s->map,"lines","show",True);
            if (flags & PlotFill) {
                put_submap(s->map,"lines","fill",VF(0.5));
            }
        }
        if (flags & PlotPoints) {
            put_submap(s->map,"points","show",True);
        }
    }
    update_options(s->map,options);
    if (! Y) { // even values are X, odd values are Y
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
    } else {
        s->X = obj_ref(X);
        s->Y = obj_ref(Y);
    }
    double **xydata = interleave(s->X,s->Y);
    map_puts(s->map,"data",xydata);
    list_add(p->series,s);
    return s;
}

void flot_series_option(Series *S, CStr key, CStr subkey, const void* value) {
    put_submap(S->map,key,subkey,value);
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
    map_puts(data,"plots",plist);
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

#define PlotMin -1e-100
#define PlotMax +1e-100

PValue flot_region(const char *axis, double x1, double x2, const char *colour) {
    Map *m = map_new_str_ref();
    Map *s = map_new_str_ref();
    map_puts(m,"color",VS(colour));
    map_puts(m,axis,s);
    if (x1 > PlotMin)
        map_puts(s,"from",VF(x1));
    if (x2 < PlotMax)
        map_puts(s,"to",VF(x2));
    return m;
    //return value_array_values_(3,"color","#f6f6f6","yaxis",value_array_values_(3,"to",VF(x),NULL),NULL);
}

#define flot_vert_region(x1,x2,c) flot_region("xaxis",x1,x2,c)
#define flot_horz_region(x1,x2,c) flot_region("yaxis",x1,x2,c)


#define flot_empty list_new_ptr

#define farr_values(...) farr_values_(__VA_ARGS__,PlotMax)

double *farr_values_(double x,...) {
    va_list ap;
    double v, *res;
    int n = 1;
    va_start(ap,x);
    while (va_arg(ap,double) != PlotMax)
        ++n;
    va_end(ap);
    res = array_new(double,n);
    n = 1;
    res[0] = x;
    va_start(ap,x);
    while ((v = va_arg(ap,double)) != PlotMax) {
        res[n++] = v;
    }
    va_end(ap);
    return res;
}

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

    flot_series_new(P,xv,yv1, PlotLines,"label","cats",
        "lines.lineWidth",VF(1),
        "lines.fill",VF(0.2)  // specified as an alpha to be applied to line colour
    );
    flot_series_new(P,xv,yv2, PlotPoints,"label","dogs","color","#F00");
    flot_series_new(P,farr_values(2.0,12.0,4.0,25.0),NULL,PlotLines,"label","lizards");

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

    flot_series_new(P2,vv,NULL,PlotBars,
       "color","#333", "bars.barWidth",VF(0.5),"bars.fill",VF(0.6)
    );

    flot_render("test");

    return 0;
}

