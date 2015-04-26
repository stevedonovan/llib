/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Generating Pretty Plots with Flot

A llib flot program generates an HTML file containing [flot](http://flotcharts.org) plots,
which you can then open in your browser.  For example, this example will generate a
'norm.html' file.

By default, this file will have Internet references to Flot and jQuery, which makes it
a self-contained document suitable for giving to others.  However, for situations where
one is offline or simply don't enjoy the latency, you can define LLIB_FLOT to be
the location of a local copy of Flot (e.g. 'file:///home/user/stuff/flot')

The official JavaScript API still applies here. Options are passed as name/value pairs,
and the name may be 'dotted', like "series.lines.fill",VF(0.2)" means 'series:{lines:{fill:0.2}}}'.

    #include <llib/flot.h>
    
    double sqr(double x) { return x*x; }
    
    double *make_gaussian(double m, double x, double *xv) {
        double s2 = 2*sqr(x);
        double norm = 1.0/(M_PI*s2);
        int n = array_len(xv);
        double *res = array_new(double,n);
        FOR (i,n) {
            res[i] = norm*exp(-sqr(xv[i]-m)/s2);
        }
        return res;
    }
    
    int main()
    {
        Flot *P = flot_new("caption", "Gaussians",
            "series.lines.fill",VF(0.2));    
        
        double *xv = farr_range(0,10,0.1);
        
        flot_series_new(P,xv,make_gaussian(5,1,xv),
            FlotLines,"label","norm s=1");    
        flot_series_new(P,xv,make_gaussian(4,0.7,xv), 
            FlotLines,"label","norm s=0.7"); 
        
        flot_render("norm");
        return 0;    
    }
    
![llib Flot](http://stevedonovan.github.io/files/llib-flot.png)

@module flot
*/

#include <stdio.h>
#include <stdlib.h>
#include "flot.h"
#include "file.h"
#include "list.h"
#include "map.h"
#include "template.h"

#define FLOT_CDN "http://www.flotcharts.org/flot/"
#define JQUERY_CDN "http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"

static List *plugins = NULL;

static str_t tpl = 
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

static str_t flot_plot = 
"@(if text |@(text)|)@(else |<h2>@(title)</h2>\n"
"<div id='@(div)' style='width:@(width)px;height:@(height)px'></div>\n|)"
;

static str_t flot_headers =
"    <!--[if lte IE 8]><script language='javascript' type='text/javascript' src='@(flot)/excanvas.min.js'></script><![endif]-->\n"
"    <script language='javascript' type='text/javascript' src='@(jquery)'></script>\n"
"    <script language='javascript' type='text/javascript' src='@(flot)/jquery.flot.js'></script>\n"
"    @(for plugins |\n"
"        <script language='javascript' type='text/javascript' src='@(flot)/jquery.flot.@(_).js'></script>\n"
"    |)\n"
;

static str_t flot_scripts = 
"<script type='text/javascript'>\n"
"    function text_marking(plot,id,xp,yp, text) {\n"
" 		var o = plot.pointOffset({ x: xp, y: yp});\n"
"		// Append it to the placeholder that Flot already uses for positioning\n"
"		$('#'+id).append(\"<div style='position:absolute;left:\" + (o.left + 4) + 'px;top:' + o.top + 'px;color:#666;font-size:smaller\">' + text + '</div>');\n"
"    }\n"
"\n"
"$(function () {\n"
"@(for plots | @(if div |\n"
"  var plot_@(div) = $.plot( $('#@(div)'),\n"
"    @(data),\n"
"    @(options)\n"
" )\n"
" @(textmarks)\n"
"|)|)\n"
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
        Map *sub = (Map*)map_gets(map,key);
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
        Map *sub = (Map*)map_gets(map,key);
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

/// new Flot plot.
// @param .... options (as name/value pairs)
// @function flot_new

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
    Map *map = (Map*)P->map;
    update_options(map,options);
    P->caption = (str_t)map_get(map,"caption");
    if (get_submap(map,"xaxis.mode")) {
        list_add_unique(plugins,"time");
    }
    list_add(plots,P);
    return P;
}

void flot_comment (str_t text) {
    list_add(plots,str_new(text));
}

/// set an option for a whole plot.
void flot_option(Flot *P, str_t key, const void* value) {
    put_submap((Map*)P->map,key,value);
}

typedef struct TextMark_ {
    double x,y;
    const char *text;
} TextMark;

/// attach text annotations to a plot.
void flot_text_mark(Flot *P, double x, double y, str_t text) {
    if (! P->text_marks) 
        P->text_marks = list_new_ref();
    TextMark *mark = obj_new(TextMark,NULL);
    mark->x = x;
    mark->y = y;
    mark->text = text;
    list_add((List*)P->text_marks, mark);
}

static void Series_dispose(Series *S) {
    obj_unref_v(S->X, S->Y);
}

/// new Flot series associated with a plot.
// If `Y` is `NULL`, then `X` is interpreted as containing x and y values interleaved.
Series *flot_series_new_(Flot *p, double *X, double *Y, int flags, PValue options) {
    Series *s = obj_new(Series,Series_dispose);
    s->plot = p;
    s->map = map_new_str_ref();
    Map *map = (Map*)s->map;
    if (flags == FlotBars) {
        put_submap(map,"bars.show",True);
    } else {
        if (flags & FlotLines) {
            put_submap(map,"lines.show",True);
            if (flags & FlotFill) {
                put_submap(map,"lines.fill",VF(0.5));
            }
        }
        if (flags & FlotPoints) {
            put_submap(map,"points.show",True);
        }
    }
    update_options(map,options);
    double **xydata = NULL;
    if (! Y) { 
        // might already be an array in the correct form!
        void **D = (void**)X;
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
    map_puts((Map*)s->map,"data",xydata);
    if (get_submap((Map*)s->map,"points.errorbars")) {
        list_add_unique(plugins,"errorbars");
    }
    list_add((List*)p->series,s);
    return s;
}

/// setting a Series option.
void flot_series_option(Series *S, str_t key, const void* value) {
    put_submap((Map*)S->map,key,value);
}

static StrTempl *flot_plot_tpl;

static char *flot_plot_impl(void *arg, StrTempl *stl) {
    str_t title = (str_t)arg;
    Flot *plot = NULL;
    FOR_LIST(iter,plots) {
        Flot *P = (Flot*)iter->data;
        if (str_eq(P->caption,title)) {
            plot = P;
            break;
        }
    }    
    return str_templ_subst_values(flot_plot_tpl, plot->vmap);
}

/// Create template data prior to rendering.
void *flot_create(str_t title) {
    List *plist = list_new_ref();
    FOR_LIST(iter,plots) {
        Map *pd = map_new_str_ref();
        if (obj_is_instance(iter->data,"Flot")) {
            Flot *P = (Flot*)iter->data;
            map_puts(pd,"title",VS(P->caption));
            map_puts(pd,"width",VI(P->width));
            map_puts(pd,"height",VI(P->height));
            map_puts(pd,"div",VS(P->id));
            
            if (P->text_marks) {
                char **out = strbuf_new();
                FOR_LIST(p,(List*)P->text_marks) {
                    TextMark *m = (TextMark*)p->data;
                    strbuf_addf(out,"text_marking(plot_%s,'%s',%f,%f,'%s')\n",P->id,P->id,m->x,m->y,m->text);
                }
                map_puts(pd,"textmarks",strbuf_tostring(out));
            }
        
            // an array of all the data objects of the series
            PValue *series = array_new_ref(PValue,list_size((List*)P->series));
            int i = 0;
            FOR_LIST(p,(List*)P->series) {
                Series *S = (Series*)p->data;
                series[i++] = S->map;
            }
        
            map_puts(pd,"data",json_tostring(series));
            map_puts(pd,"options",json_tostring(P->map));
            P->vmap = pd;
        } else {
            map_puts(pd,"text",iter->data);
        }
        list_add(plist,pd);
    }

    // and write the substitution out to the HTML output...    
    Map *data = map_new_str_ref();
    str_t flot_cdn = FLOT_CDN, jquery_cdn = JQUERY_CDN;
    str_t flot_base = getenv("LLIB_FLOT");
    if (flot_base) {
        flot_cdn = flot_base;
        jquery_cdn = str_fmt("%s/jquery.min.js",flot_cdn);
    }
    map_puts(data,"flot",flot_cdn);
    map_puts(data,"jquery",jquery_cdn);
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

/// Render plots to a file, `name`.html.
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

//// gradient between two colours.
PValue flot_gradient(str_t start, str_t finish) {
    return VMS("colors",VAS(start,finish));
}

/// coloured region between x1 and x2
// @double x1
// @double x2
// @string colour
// @function flot_vert_region

/// coloured region between y1 and y2
// @double y1
// @double y2
// @string colour
// @function flot_horz_region

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
}

