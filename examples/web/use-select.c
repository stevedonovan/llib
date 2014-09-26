/* llib simple web framework

Using a select loop makes the HTTP server non-blocking.

`select_add_reader` allows us to provide a command-line to the server.

Can of course use llib templates; note that the request contains a `vars` field
which is a simple map suitable for passing directly to the template.
    
*/
#include <stdio.h>
#include <llib/str.h>
#include <llib/template.h>
#include <llib-p/http.h>
#include <llib-p/socket.h>
#include <llib-p/select.h>

str_t templ = 
  "<html><body>\n"
  "<h1>Hello $(name)! How is $(dog)?</h2>\n"
  "vars are $(for _: $(_); ) <br/>\n"
  "Here is the <a href='/static/use-select.c'>code</a>\n"
  "</body></html>\n"; 
  
static str_t index (HttpRequest *web) {
    static StrTempl *tpl = NULL;
    if (! tpl)
        tpl = str_templ_new(templ,NULL);  // use $
    return str_templ_subst_values(tpl,web->vars);
}

// called for each line typed in console
static int on_line(str_t line,...) {
    printf("'%s'\n",line);
    if (str_eq(line,"quit")) 
        return 1; // quit the program!
    return 0;
}

int main(int argc, char **argv) 
{
    http_add_route("/", index);
    
    // so this file will be refered to as /static/use-select.c
    http_add_static("/static",".");
    
    Select *s = select_new();
    
    int srv = socket_server(argv[1] ? argv[1] : "127.0.0.1:8080");
    select_add_read(s,srv);
    
    // when a line from stdin becomes available; close us down on EOF (ctrl-D)
    select_add_reader(s,0,true,on_line,NULL);
    
    while (select_select(s) != -1) {
        if (select_can_read(s,srv)) {
            int c = socket_accept(srv);
            http_handle_request(c);            
        }
    }
    
    return 0;
}
