cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   obj.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   sort.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   list.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   file.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   scan.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   map.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   str.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   template.c
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   value.c
lib /nologo obj.obj sort.obj list.obj file.obj scan.obj map.obj str.obj template.obj value.obj /OUT:llib_static.lib
