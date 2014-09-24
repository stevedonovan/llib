#include <llib/flot.h>

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
    double Y2[] = {9.0,9.0,25.0,28.0,32.0};
    double *xv = array_new_copy(double,X,5);
    double *yv1 = array_new_copy(double,Y1,5);
    double *yv2 = farr_copy(Y2);
    
    const char *gray = "#f6f6f6";

    flot_series_new(P,xv,yv1, FlotLines,"label","cats",
        "lines.lineWidth",VF(1),
        "lines.fill",VF(0.2)  // specified as an alpha to be applied to line colour
    );
    flot_series_new(P,xv,yv2, FlotPoints,"label","dogs","color","#F00");
    flot_series_new(P,farr_4(2,12,4,25),NULL,FlotLines,"label","lizards");

    // this is based on the Flot annotations example
    
    Flot *P2 = flot_new("caption","Second Test",
        "legend.show",False, // no legend
        // and explicitly give ourselves some more vertical room
        "yaxis.min",VF(-2),"yaxis.max",VF(2),
        flot_markings (
            flot_horz_region(1,FlotMax,gray),
            flot_horz_region(FlotMin,-1,gray),
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

    flot_render("markings");

    return 0;
}
