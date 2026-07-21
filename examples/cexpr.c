/**
This is a little program which takes C expressions, constructs a little
program around them, links against llib, and runs them.
```sh
$ ./cexpr -f %f '10.0*sin(1.2)'
9.320391
```
But any `llib` allocated value is put out in `JSON` format:

```sh
:~/llib/examples$ ./cexpr 'file_files_in_dir("*.c",false)'
["cexpr.c","cmd.c","codex-sessions.c","config.c","getdoc.c","make-ex.c","mapset.c","procs.c","subst.c","tempx.c","test-rx.c","testa.c","testing.c","tmp.c","txt2c.c","words.c"]
```
Can ask for `JSON` pretty printing:

```sh
$ ./cexpr -p 'VM("one",VI(10),"two",VA(VI(10),VI(20)))'
{
  "one":10,
  "two":[
    10,
    20
  ]
}
```
As a convenience, `JSON` files can be read in (using the extra slack
format supported by `json_parse_file_vs5`). Then the special variable
`JS` becomes available:

```sh
~/llib/examples$ ./cexpr -j test.json 'smap_get_pointer(JS,"/config/auto")'
true
```
*/

#include "llib/all.h"
#include "llib/arg.h"
#include <stdio.h>
#include <stdlib.h>

static struct Args {
    char *expr;
    char *format;
    bool pretty;
    char *jfile;
} args;

bool compile_and_run(const char *prog) {
    scoped char *cmd = str_fmt(
        "gcc -std=c2x -I.. %s.c -o %s -Wl,-s -L../llib -lllib", prog, prog);
    // printf("cmd %s", cmd);
    bool ok = system(cmd) == 0;
    if (ok) {
        scoped char *p = str_fmt("./%s", prog);
        return system(p) == 0;
    } else {
        return false;
    }
}

char *make_program(const struct Args *args) {
    scoped char *full_expr;
    bool formatted = strlen(args->format) > 0;
    if (formatted) {
        full_expr = str_fmt("printf(\"%s\\n\",(%s))", args->format, args->expr);
    } else if (args->pretty) {
        full_expr = str_fmt("puts(json_to_pretty_string((%s),2))", args->expr);
    } else {
        full_expr = str_fmt("puts(json_tostring((%s)))", args->expr);
    }
    auto read_first =
        strlen(args->jfile)
            ? str_fmt("PValue JS = json_parse_file_vs5(\"%s\"); \n",
                      args->jfile)
            : "";
    return str_fmt("#include <stdlib.h> \n"
                   "#include <stdio.h> \n"
                   "#include \"llib/all.h\" \n"
                   "int main(int argc, char **argv) { \n"
                   "%s %s; \n return 0; \n }\n",
                   read_first, full_expr);
}

PValue spec[] = {"// cexpr: build & run C expressions",
                 "string format=''; // -f printf format (default to JSON dump)",
                 &args.format,
                 "bool pretty; // -p pretty JSON",
                 &args.pretty,
                 "string json=''; // -j read this JSON5 file as JS",
                 &args.jfile,
                 "string #1; // expression to evaluate",
                 &args.expr,
                 NULL};

int main(int argc, const char **argv) {
    arg_command_line(spec, argv);
    auto prog = make_program(&args);
    file_write_fmt("tmp.c", "%s", prog);
    return compile_and_run("tmp");
}