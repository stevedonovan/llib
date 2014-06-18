project='llib'
title='llib Documentation'
description='llib: A compact general-purpose C library'
full_description='Available at [Github](https://github.com/stevedonovan/llib)'
file={'obj.c',  'str.c', 'smap.c','scan.c', 'template.c', 'list.c', 'map.c', 'file.c', 
    'value.c', 'json.c','xml.c','farr.c','array.h','table.c','config.c','arg.c'}
parse_extra={C=true}
local tst, ex = '../tests/test-', '../examples/'
examples={
tst..'config.c',tst..'file.c',tst..'json.c',tst..'list.c',tst..'map.c',
tst..'obj.c',tst..'scan.c', tst..'seq.c',tst..'str.c',tst..'table.c',
tst..'sqlite3-table.c',tst..'template.c',tst..'xml.c',
ex..'cmd.c',ex..'testa.c',ex..'config.c',ex..'words.c'
}
format='discount'
new_type("macro","Macros")

