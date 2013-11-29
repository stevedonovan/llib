/* Useful little test rig for playing with templates;
 * you provide a JSON file defining the data, and a template file;
 * test.js and test.ctp are provided in this directory
*/
#include <stdio.h>
#include <llib/template.h>
#include <llib/file.h>
#include <llib/json.h>

int main(int argc, char **argv)
{
    PValue v = NULL;
    StrTempl st = NULL;
    char *res = NULL;
    if (argc != 3) {
        printf("usage: tempx <js-file> <tmpl-file>\n");
        return 1;
    }
    v = json_parse_file(argv[1]);
    if (value_is_error(v)) {
        printf("json error: %s\n",v);
        goto err;
    }
    res = file_read_all(argv[2],true);
    if (! res) {
        printf("cannot read '%s'\n",argv[2]);
        goto err;
    }

    st = str_templ_new(res,NULL);
    unref(res);

    res = str_templ_subst_values(st,v);
    printf("%s\n",res);

err:
    dispose(v,st,res);
    printf("kount = %d\n",obj_kount());
    return 0;
}
