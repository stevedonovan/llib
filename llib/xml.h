/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_XML_H
#define _LLIB_XML_H

#include "obj.h"

PValue xml_parse_string(const char *str, bool is_data);
PValue xml_parse_file(const char *file, bool is_data);

const char *xml_tag(PValue* doc);
char** xml_attribs(PValue* doc);
PValue *xml_children(PValue* doc, int* plen);

char *xml_tostring(PValue doc, int indent);

#endif
