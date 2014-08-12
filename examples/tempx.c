/* Useful little test rig for playing with templates;
 * you provide a JSON file defining the data, and a template file;
 * test.js and test.ctp are provided in this directory
*/
#include <stdio.h>
#include <llib/template.h>
#include <llib/file.h>
#include <llib/json.h>
#include <llib/str.h>

static char *test_impl (void *arg, StrTempl *stl) {
    return str_fmt("<h2>%s</h2>\n",(char*)arg);
}

int main(int argc, char **argv)
{
    PValue v = NULL;
    StrTempl *st = NULL;
    char *res = NULL;
    char *jfile, *tfile;
    if (argc != 3) {
        jfile = "test.js";
        tfile = "test.ctp";
    } else {
        jfile = argv[1];
        tfile = argv[2];
    }
    printf("file '%s' '%s'\n",jfile,tfile);
    v = json_parse_file(jfile);
    if (value_is_error(v)) {
        printf("json error: %s\n",value_as_string(v));
        goto err;
    }
    res = file_read_all(tfile,true);
    if (! res) {
        printf("cannot read '%s'\n",tfile);
        goto err;
    }

    st = str_templ_new(res,NULL);
    unref(res);
    
    str_templ_add_builtin("test",test_impl);

    res = str_templ_subst_values(st,v);
    printf("%s\n",res);

err:
    dispose(v,st,res);
    printf("kount = %d\n",obj_kount());
    return 0;
}
