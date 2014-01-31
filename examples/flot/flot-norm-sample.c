// Simple use of llib flot using the defaults.

#include "flot.c"

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

double frand() {
    return (rand() % 100)/100.0;
}

int main()
{
    Flot *P = flot_new("caption", "Gaussians","yaxis.min",VF(-0.1));    
    
    farr_t xv = farr_range(0,10,0.1);
    farr_t yv = make_gaussian(5,0.7,xv);    
    
    // let's make some fake data...
    farr_t xs = farr_sample(xv,0,-1,3);
    farr_t ys = farr_sample(yv,0,-1,3);
    
    FOR(i,array_len(ys))
        ys[i] += 0.04*frand() - 0.02;
    
    flot_series_new(P,xv,yv, FlotLines,"label","norm s=0.7"); 
    flot_series_new(P,xs,ys, FlotPoints,"label","data"); 
    
    flot_render("norm-sample");
    return 0;
    
}