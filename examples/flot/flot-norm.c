// Simple use of llib flot using the defaults.

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
    Flot *P = flot_new("caption", "Gaussians","series.lines.fill",VF(0.2));    
    
    double *xv = farr_range(0,10,0.1);
    
    flot_series_new(P,xv,make_gaussian(5,1,xv), FlotLines,"label","norm s=1");    
    flot_series_new(P,xv,make_gaussian(4,0.7,xv), FlotLines,"label","norm s=0.7"); 
    
    flot_comment("default fill colour is line colour");
    flot_render("norm");
    return 0;
    
}