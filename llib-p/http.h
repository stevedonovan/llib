#ifndef ___HTTP_H
#define ___HTTP_H

#ifndef __LLIB_STR_H
typedef const char *str_t;
#endif

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

typedef str_t (*HttpHandler)(HttpRequest *web, void *user_data);

typedef struct {
    HttpHandler handler;
    FILE *in;
    HttpRequest *req;
    void *data;
} HttpContinuation;

void http_set_verbose(bool v);
str_t http_var_get (HttpRequest *req, str_t name);
bool http_var_set(char** substs, str_t name, str_t value);
char *http_url_encode (str_t val);
void http_url_decode (str_t url, HttpRequest *req);

void http_request_start(int c, str_t path, char **vars, bool post, str_t body);
HttpResponse *http_request_read(int c);

void http_add_route(str_t path, HttpHandler handler, void *user_data);
void http_add_static (str_t route, str_t path, int cache);
HttpContinuation *http_handle_request (int fd);
str_t http_continuation_new(void *data);
void http_continuation_end(HttpContinuation *cc, str_t body);
void http_continuation_handle (HttpHandler h, str_t body);
void http_add_out_header(HttpRequest *req, str_t name, str_t value);
str_t http_redirect(HttpRequest *req, str_t url);
str_t http_continuation_new(void *data);
void http_continuation_end(HttpContinuation *cc, str_t body);


//#define http_server(addr)  http_handle_request(socket

#endif
