#include <llib/flot.h>

int main()
{
    // apply gradient from top to bottom as background, and move legend
    Flot *P = flot_new("caption", "First Test",
        "grid.backgroundColor",flot_gradient("#FFF","#EEE"),
        // 'nw' is short for North West, i.e. top-left
        "legend.position","nw"
    );

    // Series data must be llib arrays of doubles 
    double X[] = {1,2,3,4,5};
    double Y1[] = {10,15,23,29,31};
    double Y2[] = {9.0,9.0,25.0,28.0,32.0};
    double *xv = farr_copy(X);
    double *yv1 = farr_copy(Y1), *yv2 = farr_copy(Y2);

    // fill underneath this series
    flot_series_new(P,xv,yv1, FlotLines,"label","cats",
        "lines.fill",VF(0.2)  // specified as an alpha to be applied to line colour
    );
    
    // override point colour
    flot_series_new(P,xv,yv2, FlotPoints,"label","dogs","color","#F00");
    
    // will generate test.html
    flot_render("test");
    return 0;
    
}