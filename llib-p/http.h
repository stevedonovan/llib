#ifndef ___HTTP_H
#define ___HTTP_H
#include <llib/str.h>

typedef struct HttpRequest_ {
    // input
    char **vars;  // url-encoded variables
    char **headers_in;  // headers of request
    str_t path; // path relative to route
    str_t local_path;  // usually NULL; not rc
    // output
    str_t status; // default '200' (not rc)
    str_t type;  // default 'text/html' (not rc)
    char ***headers_out;
} HttpRequest;

typedef struct HttpResponse_ {
    str_t body;
    str_t status;
    str_t type;
    char **headers; // headers of response
} HttpResponse;

typedef str_t (*HttpHandler)(HttpRequest *web);

str_t http_var_get (HttpRequest *req, str_t name);
char *http_url_encode (str_t val);
void http_url_decode (str_t url, HttpRequest *req);
HttpResponse *http_request(int c, str_t path, char **vars);
bool http_var_set(char** substs, str_t name, str_t value);
void http_add_route(str_t path, HttpHandler handler);
void http_add_static (str_t route, str_t path);
void http_handle_request (int fd);
void http_add_out_header(HttpRequest *req, str_t name, str_t value);
str_t http_redirect(HttpRequest *req, str_t url);

//#define http_server(addr)  http_handle_request(socket

#endif
