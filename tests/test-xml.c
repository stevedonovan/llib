#include <stdio.h>
#include <llib/json.h>

#include <llib/xml.h>

const char *file = "test.xml";

int main(int argc, char**argv)
{
    if (argv[1])
        file = argv[1];
    PValue v = xml_parse_file(file,true);
    if (value_is_error(v)) {
        fprintf(stderr,"error reading xml: %s\n",(char*)v);
        return 1;
    }
    //char *s = json_tostring(v); // try this for kicks...
    char *s = xml_tostring(v,2);
    printf("%s\n",s);
    obj_unref_v(v, s);
    
    // the same notation used for building up JSON data works fine for XML docs
    v = VAS("root",
        VAS("item",VMS("name","age","type","int"),"10"),
        VAS("item",VMS("name","name","type","string"),"Bonzo")
    );
    s = xml_tostring(v,0);
    printf("%s\n",s);
    char *s2 = json_tostring(v);
    printf("json %s\n",s2);
    obj_unref_v(v, s, s2);
    printf("kount %d\n",obj_kount());
    return 0;
}
