/* llib simple web framework
This is a simple blocking server on localhost.

Can associate handlers with different patterns or _routes_;
handlers will receive any vars encoded in the full URL
*/
#include <stdio.h>
#include <llib/str.h>
#include <llib-p/http.h>
#include <llib-p/socket.h>

static str_t index (HttpRequest *web) {
    str_t name = http_var_get(web,"name");
    if (! name)
        name = "Joe";
    return str_fmt("<html><body><h1>Hello %s!</h1></body></html>",name);
}

int main() 
{
    http_add_route("/", index);
    
    int s = socket_server("127.0.0.1:8080");
    
    for(;;) {
        int c = socket_accept(s);
        http_handle_request(c);
    }
    
    return 0;
}