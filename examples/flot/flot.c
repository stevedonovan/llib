#include <stdio.h>
#include <stdlib.h>
#include <llib/file.h>
#include <llib/list.h>
#include <llib/map.h>
#include <llib/str.h>
#include <llib/template.h>
#define LLIB_JSON_EASY
#include <llib/json.h>
#include <llib/farr.h>

#ifdef STANDALONE_FLOT
#define FLOT_CDN "file:///home/steve/projects/flot"
#define JQUERY_CDN FLOT_CDN "/jquery.min.js"
#else
#define FLOT_CDN "http://www.flotcharts.org/flot/"
#define JQUERY_CDN "http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"
#endif

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
    void *vmap;
} Flot;

typedef struct Series_ {
    const char *name;
    double *X;
    double *Y;
    Flot *plot;
    Map *map;
} Series;

typedef char *Str;
typedef const char *str_t;

static List *plugins = NULL;

str_t tpl = 
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN' 'http://www.w3.org/TR/html4/loose.dtd'>\n"
"<html>\n"
" <head>\n"
" <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
" <title>@(title)</title>\n"
" @(flot-headers)\n"
" </head>\n"
"<body>\n"
"@(for plots | @(do-plot _) |)\n"
"@(flot-scripts)\n"
" </body>\n"
"</html>\n"
;

str_t flot_plot = 
"<h2>@(title)</h2>\n"
"<div id='@(div)' style='width:@(width)px;height:@(height)px'></div>\n"
;

str_t flot_headers =
"    <!--[if lte IE 8]><script language='javascript' type='text/javascript' src='@(flot)/excanvas.min.js'></script><![endif]-->\n"
"    <script language='javascript' type='text/javascript' src='@(jquery)'></script>\n"
"    <script language='javascript' type='text/javascript' src='@(flot)/jquery.flot.js'></script>\n"
"    @(for plugins |\n"
"        <script language='javascript' type='text/javascript' src='@(flot)/jquery.flot.@(_).js'></script>\n"
"    |)\n"
;

str_t flot_scripts = 
"<script type='text/javascript'>\n"
"    function text_marking(plot,id,xp,yp, text) {\n"
" 		var o = plot.pointOffset({ x: xp, y: yp});\n"
"		// Append it to the placeholder that Flot already uses for positioning\n"
"		$('#'+id).append(\"<div style='position:absolute;left:\" + (o.left + 4) + 'px;top:' + o.top + 'px;color:#666;font-size:smaller\">' + text + '</div>');\n"
"    }\n"
"\n"
"$(function () {\n"
"@(for plots |\n"
"  var plot_@(div) = $.plot( $('#@(div)'),\n"
"    @(data),\n"
"    @(options)\n"
" )\n"
" @(textmarks)\n"
"|)\n"
"});\n"
"</script>\n"
;

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
static void put_submap(Map *map, str_t ckey, const void* value) {
    char key[MAX_KEY];
    strcpy(key,ckey);
    char *subkey = splitdot(key);
    if (! subkey) {
        map_puts(map,key,value);
    } else {
        Map *sub = map_gets(map,key);
        if (! sub) {
            sub = map_new_str_ref();
            map_puts(map,key,sub);
        }        
        put_submap(sub,subkey,value);
    }
}

static PValue get_submap(Map *map, str_t ckey) {
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

void flot_option(Flot *P, str_t key, const void* value) {
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

void flot_series_option(Series *S, str_t key, const void* value) {
    put_submap(S->map,key,value);
}

static StrTempl *flot_plot_tpl;

static char *flot_plot_impl(void *arg, StrTempl *stl) {
    str_t title = (str_t)arg;
    Flot *plot = NULL;
    FOR_LIST(iter,plots) {
        Flot *P = iter->data;
        if (str_eq(P->caption,title)) {
            plot = P;
            break;
        }
    }    
    return str_templ_subst_values(flot_plot_tpl, plot->vmap);
}

void *flot_create(str_t title) {
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
        P->vmap = pd;
    }

    // and write the substitution out to the HTML output...    
    Map *data = map_new_str_ref();
    map_puts(data,"flot",FLOT_CDN);
    map_puts(data,"jquery",JQUERY_CDN);
    map_puts(data,"title",title);
    map_puts(data,"plots",plist);
    map_puts(data,"plugins",plugins);
    
    flot_plot_tpl = str_templ_new(flot_plot,"@()");
    str_templ_add_macro("do-plot",flot_plot_tpl,NULL);
    str_templ_add_builtin("flot-plot",flot_plot_impl);
    str_templ_add_macro("flot-headers",str_templ_new(flot_headers,"@()"),data);
    str_templ_add_macro("flot-scripts",str_templ_new(flot_scripts,"@()"),data);
    return data;
}

void flot_render(str_t name) {
    void *data = flot_create(name);  // use name as title of document....
    StrTempl *st = str_templ_new(tpl,"@()");    
    char *res = str_templ_subst_values(st,data);
    char *html = str_fmt("%s.html",name);
    FILE *out = fopen(html,"w");
    fputs(res,out);
    fclose(out);
    printf("output written to %s\n",html);    
}

char *flot_rgba(int r, int g, int b,int a) {
    return str_fmt("rgba(%d,%d,%d,%d)",r,g,b,a);
}

PValue flot_gradient(str_t start, str_t finish) {
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


PValue flot_region(str_t axis, double x1, double x2, str_t colour) {
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

