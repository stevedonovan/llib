#include <stdio.h>
#include <llib/table.h>
#include "flot.c"

void read_dates_and_values(const char *file, farr_t *dates, farr_t *values) {
    Table *t = table_new_from_file(file, TableCsv | TableAll);
    if (t->error) {
        fprintf(stderr,"%s %s\n",file,t->error);
        exit(1);        
    }
    table_convert_cols(t,0,TableInt,1,TableFloat,-1);
    table_generate_columns(t);
    
    *dates = farr_sample_int((int*)t->cols[0],0,-1,1);
    *values = farr_sample_float((float*)t->cols[1],0,-1,1);    
    
}

int main()
{
    farr_t oil_dates, oil_prices, exchange_dates, exchange_rates;
    
    read_dates_and_values("oilprices.csv",&oil_dates, &oil_prices);
    read_dates_and_values("exchangerates.csv",&exchange_dates, &exchange_rates);        
    
    // Flot wants milliseconds (JavaScript), not seconds (C) since epoch
    farr_scale(oil_dates,1000,0);
    farr_scale(exchange_dates,1000,0);    

    Flot *P = flot_new("caption", "Exchange Rates and Oil Price in 2007","xaxis.mode","time",
        "yaxes",VA(VMS("min",VF(0)),VMS("position","right"))
    );  
    
    flot_series_new(P,oil_dates, oil_prices,FlotLines,"label","Oil price ($)");
    flot_series_new(P,exchange_dates, exchange_rates,FlotLines,"label","USD/EUR exchange rate",
        "yaxis",VF(2)
    );
    
    flot_render("prices");
    return 0;
}

    //~ FOR(i,array_len(oil_dates)) {
        //~ printf("%f %f\n",oil_dates[i],oil_prices[i]);
    //~ }    
    
    //~ FOR(i,array_len(exchange_dates)) {
        //~ printf("%f %f\n",exchange_dates[i],exchange_rates[i]);
    //~ }    
    
