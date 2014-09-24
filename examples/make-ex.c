/* A useful utilty using templates; it generates a skeleton for llib
 * programs. You pass it the names of the headers you would like to include,
 * and capture the output, e.g. 'make-ex str list > mystr.c'
 *
 * Here the data for the template is a simple NULL-terminated array of strings,
 * which argv is guaranteed to be. We use '_' to select the whole data, without
 * lookup.
 */
#include <stdio.h>
#include <llib/template.h>

const char *tpl =
    "#include <stdio.h>\n"
    "$(for _ |\n"
    "#include <llib/$(_).h>|)\n\n"
    "int main(int argc, char**argv)\n{\n"
    "    return 0;\n}";

int main(int argc, char **argv)
{
    StrTempl *t = str_templ_new(tpl,NULL);
    char *res = str_templ_subst(t,argv+1);
    printf("%s\n",res);
    return 0;
}
