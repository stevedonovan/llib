/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013-2015
*/

/****
### Simple HTTP app server

*/

#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <llib/file.h>
#include "http.h"

static bool s_verbose = false;

/// make the server very chatty.
void http_set_verbose(bool v) {
    s_verbose = v;
}

/// escape any non-alphanumeric characters in an URL.
char *http_url_encode (str_t val) {
    char **res = strbuf_new();
    int len = strlen(val);
    FOR(i,len) {
        char c = val[i];
        if (isalpha(c) || isdigit(c)) {
            strbuf_add(res,c);
        } else
        if (c == ' ') {
            strbuf_add(res,'+');
        } else {
            strbuf_addf(res,"%%%02X",c);
        }
    }
    return strbuf_tostring(res);
}

static char *decode(str_t s) {
    char **res = strbuf_new();
    while (*s) {
        char ch;
        if (*s == '+') {
            ch = ' ';
        } else
        if (*s == '%') {
            int code;
            sscanf(s+1,"%02X",&code);
            s += 2;
            ch = (char)code;
        } else {
            ch = *s;
        }
        strbuf_add(res,ch);
        ++s;        
    }
    return strbuf_tostring(res);
}

static void var_put(char ***ss, str_t name, str_t value) {
    smap_add(ss,str_new(name),str_new(value));
}

static char line[1024];

static char** parse_headers(FILE *in) {
    char ***hds = smap_new(true);
    file_gets(in,line,sizeof(line));
    while (*line != 0) {
        if (s_verbose)
            printf("%s\n",line);
        char *sep = strchr(line,':');
        *sep = '\0';
        var_put(hds,line,sep+2);
        file_gets(in,line,sizeof(line));    
    }
    return smap_close(hds);
}

void HttpResponse_dispose(HttpResponse *resp) {
    obj_unref(resp->body);
    obj_unref(resp->status);
    obj_unref(resp->type);
    obj_unref(resp->headers);
}

static void encode_vars(FILE *f, char **vars) {
    for (int i = 0, n = array_len(vars); i < n; i+=2) {
        char *enc = http_url_encode(vars[i+1]);
        fprintf(f,"%s=%s",vars[i],enc);
        if (i < n-2)
            fprintf(f,"&");
        obj_unref(enc);
    }
}    

/// Send a HTTP request with an URL and optional variables.
// May also say `true` for `post` and provide a body to be sent.
HttpResponse *http_request(int c, str_t path, char **vars, bool post, str_t obody) {
    int nreq, n;
    FILE *f = fdopen(c,"r+");
    fprintf(f,"%s %s",post ? "POST" : "GET",path);
    if (vars) {
        fprintf(f,"?");
        encode_vars(f,vars);
    }
    fprintf(f," HTTP/1.1\r\n");
    fprintf(f,"Host: localhost\r\n\r\n");
    if (obody) {
        fprintf(f,"Content-Length: %d\r\n",(int)strlen(obody));
        fprintf(f,"%s\r\n",obody);        
    }
    fflush(f);
    
    HttpResponse *resp = obj_new(HttpResponse,HttpResponse_dispose);
    
    // get status from first line
    file_gets(f,line,sizeof(line));
    char **parts = str_split(line," ");     
    resp->status = str_ref(parts[1]);
    obj_unref(parts);
    
    // we particularly need to know how big the body data is....
    resp->headers = parse_headers(f);    
    str_t slen = str_lookup(resp->headers,"Content-Length");
    n = atoi(slen);
    
    resp->type = str_ref(str_lookup(resp->headers,"Content-Type"));
    
    if (s_verbose)
        printf("fetching %d bytes\n",n);
    char *body = str_new_size(n);
    nreq = fread(body,1,n,f);
    if (nreq != n) {
        fprintf(stderr,"asked for %d, got %d bytes\n",n,nreq);
    }
    // this is safe even for binary returned values
    body[nreq] = 0;    
    fclose(f);
    
    resp->body = body;
    return resp;
}

void http_url_decode (const char *url, HttpRequest *req) {
    char *parms, **vars;
    char url_copy[512];
    strcpy(url_copy,url);    
    parms = strchr(url_copy,'?');    
    if (parms)
        *parms = '\0';        
    req->path = str_new(url_copy);
    if (parms) {
        char ***ss = smap_new(true);
        vars = str_split(parms+1,"&");
        FOR(i,array_len(vars)) {
            char *name = vars[i], *value;
            // decode values
            char *sep = strchr(name,'=');
            if (! sep) { // pathological. Likely?
                value = str_new("");
            } else {
                *sep = '\0';
                value = decode(sep+1);
            }   
            var_put(ss,name,value);
            obj_unref(value);
        }
        req->vars = smap_close(ss);
        obj_unref(vars);
    } else { // empty var array - don't need to check for existence!
        req->vars = array_new(char*,0);
    }
}

/// set the value of a URL-encoded variable.
// Only works if the variable _already exists_!
bool http_var_set(char** substs, const char *name, const char *value) {
    for (char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0) {
            obj_unref(*(S+1));
            *(S+1) = (char*)str_ref(value);
        }
    }
    return NULL;
}

/// get the value of a URL-encoded variable.
str_t http_var_get (HttpRequest *req, str_t name) {
    return str_lookup(req->vars,name);
}

#define MAX_ROUTES 30

typedef struct {
    const char *path;
    int path_len;
    HttpHandler handler;
    const char *local_path;
    HttpContinuation *cc;
} RouteEntry;

static RouteEntry routes[MAX_ROUTES];
static int last_route = 0;

static char *mime_types[] = {
    "jpeg","image/jpeg","jpg","image/jpg","gif","image/gif","png","image/png",
    "html","text/html","css","text/css","js","text/javascript",
    NULL
};

static str_t static_handler(HttpRequest *web) {
    char file[512];
    str_t contents;
    strcpy(file,web->local_path);
    strcat(file,web->path);
    if (s_verbose)
        printf("trying to open '%s'\n",file);
    contents = file_read_all(file,false); 
    if (! contents) {
        web->status = "404";
        return "<html><body><h1>file does not exist</h1></body></html>";
    } else {
        str_t ext = file_extension(file);
        str_t mime = str_lookup(mime_types,ext);
        if (! mime)
            mime = "text/plain";
        web->type = mime;
        http_add_out_header(web,"Cache-Control","max-age=86400");
        return contents;
    }
}

/// Associate a route with a handler.
// The _longest_ route which matches the path will be chosen.
void http_add_route(const char *path, HttpHandler handler) {
    routes[last_route].path = path;
    routes[last_route].handler = handler;
    routes[last_route].path_len = strlen(path);
    ++last_route;
    routes[last_route].path = NULL;
    if (str_eq(path,"/")) { // special case!
        http_add_route("/index.html",handler);
    }
}

/// Set up a route for statically serving files.
// Multiple such routes may be set for different directories.
void http_add_static (const char *route, const char *path) {
    http_add_route(route,static_handler);
    routes[last_route-1].local_path = path;
}

static bool is_end(str_t path, int sz) {
    char ch = path[sz];
    if (sz == 1) // '/' is a special case...
        return true; 
    return ch=='\0' || ch=='/';
}

// find the longest route which matches the path
static RouteEntry *match_request(str_t path) {
    int max_len = 0;
    RouteEntry *re_match = NULL;
    for (RouteEntry *re = routes; re->path; ++re) {
        if (strncmp(path,re->path,re->path_len) == 0 && is_end(path,re->path_len)) {
            if (s_verbose)
                printf("matching '%s'\n",re->path);
            if (re->path_len > max_len) {
                max_len = re->path_len;
                re_match = re;
            }
        }
    }
    return re_match;
}

static void send_response(FILE *in, str_t body, str_t code, str_t type, char** headers) {
    fprintf(in,"HTTP/1.1 %s\r\n",code);
    fprintf(in,"Content-Type: %s\r\n",type);
    fprintf(in,"Content-Length: %d\r\n",(int)strlen(body));
    if (headers) {
        for (char **h = headers; *h; h += 2) {
            fprintf(in,"%s: %s\r\n",*h,*(h+1));
        }
    }
    fprintf(in,"\r\n");
    fprintf(in,"%s",body); 
}

void HttpRequest_dispose (HttpRequest *req) {
    obj_unref(req->vars);
    obj_unref(req->headers_in);
    obj_unref(req->path);
    obj_unref(req->headers_out);
}

static void finish_response(FILE *in, str_t body, HttpRequest *req);

/// Handle a request by parsing the URL and headers and matching the route.
// The handler is assumed to return a string; if it's one of our strings, it will
// be disposed. This response is written to the file handle which will be closed.
HttpContinuation* http_handle_request (int fd) {
    FILE *in = fdopen(fd,"r+");
    if (in == NULL) {
        perror("fdopen");
        close(fd);
        return NULL;
    }
    file_gets(in,line,sizeof(line));
    if (s_verbose)
        printf("request '%s'\n",line);
    // char **parts = str_split(line," "); bommmbed?
    // pull out path
    char *sp = strchr(line,' ');
    char *raw;
    *sp = '\0';
    raw = sp+1;
    sp = strchr(raw,' ');
    *sp = '\0';
    
    HttpRequest *req = obj_new(HttpRequest,HttpRequest_dispose);
    req->status = "200";
    req->type = "text/html";
    req->headers_out = NULL;
    
    // parse URL and headers
    http_url_decode(raw,req);
    req->headers_in = parse_headers(in);    
  
    // and match the best handler for this route....
    RouteEntry *re_match = match_request(req->path);
    if (! re_match) { // error
        char obuff[1024];
        sprintf(obuff,"<html><head><title>Error</title></head><body><h1>%s</h1></body></html>","no such route or file");
        send_response(in, obuff,"404","text/html",NULL);
    } else {
        // make PATH relative to the route
        str_t path = str_new(req->path + re_match->path_len);
        obj_unref(req->path);
        req->path = path;
        
        // extra data?
        if (re_match->local_path)
            req->local_path = re_match->local_path;
        
        // call the handler
        str_t body = re_match->handler(req);
        if (obj_is_instance(body,"HttpContinuation")) {
            HttpContinuation *cc = (HttpContinuation*)body;
            cc->handler  = re_match->handler;
            cc->in = in; 
            cc->req = req;
            re_match->cc = cc;
            return cc;
        }
        finish_response(in, body, req);
    }        
    obj_unref(req);
    fclose(in);    
    return NULL;
}

static void finish_response(FILE *in, str_t body, HttpRequest *req) {
    char **headers_out = NULL;
    if (req->headers_out)
        headers_out = *req->headers_out;
    send_response(in, body,req->status,req->type,headers_out);
    
    // and clean up the string, if it's one of ours
    if (obj_refcount(body) != -1)
        obj_unref(body);    
}

static void HttpContinuation_dispose(HttpContinuation *cc) {
    obj_unref(cc->req);
    fclose(cc->in);    
}

str_t http_continuation_new(void *data) {
    HttpContinuation *cc = obj_new(HttpContinuation,HttpContinuation_dispose);
    cc->data = data;
    return (str_t)cc;
}

void http_continuation_end(HttpContinuation *cc, str_t body) {
    finish_response(cc->in, body, cc->req);
    obj_unref(cc);
}

void http_continuation_handle (HttpHandler h, str_t body) {
    for (RouteEntry *re = routes; re->path; ++re) {
        if (re->handler == h) {
            http_continuation_end(re->cc,body);
            return;
        }
    }
}

/// Handlers can use this to add extra headers to the response, apart
// from the default content-type and content-length.
void http_add_out_header(HttpRequest *req, str_t name, str_t value) {
    if (! req->headers_out)
        req->headers_out = smap_new(true);
    smap_add(req->headers_out, str_ref(name),str_ref(value));
}

/// Handlers can use this to redirect to a new URL.
str_t http_redirect(HttpRequest *req, str_t url) {
    req->status = "302";
    http_add_out_header(req,"Location",url);
    return "redirect";
}
