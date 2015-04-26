project='llib'
title='llib Documentation'
description='llib: A compact general-purpose C library'
full_description='Available at [Github](https://github.com/stevedonovan/llib)'
file={'obj.c', 'str.c', 'smap.c','scan.c', 'template.c', 'list.c', 'map.c', 'file.c', 'file_fmt.c',
    'value.c', 'interface.c', 'json.c','json-parse.c', 'xml.c','farr.c','array.h','table.c','config.c',
    'arg.c','flot.c'}
parse_extra={C=true}
-- dont_escape_underscore=true
-- global_lookup=true
local function ex(f) return '../examples/'..f..'.c' end
examples={
'../tests', ex'cmd',ex'testa',ex'config',ex'words'
}
readme = '../readme.md'
format='discount'
new_type("macro","Macros")
--new_type("typename","Types")
tparam_alias('double','double')
style = '!fixed'


