#include "flot.c"

int main()
{
    Flot *P = flot_new("caption", "Trig Functions");
    
    double **vsin = seq_new(double);
    double **vcos = seq_new(double);
    for (double x = 0; x < 2*M_PI; x += 0.1)  {
        seq_add2(vsin,x,sin(x));
        seq_add2(vcos,x,cos(x));
    }

    flot_series_new(P,farr_from_seq(vsin), NULL,FlotLines,"label","sine");
    flot_series_new(P,farr_from_seq(vcos), NULL,FlotLines,"label","cosine");
    
    flot_render("sine");
    return 0;
    
}