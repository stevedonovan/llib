cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   obj.c /Foobj.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   sort.c /Fosort.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   list.c /Folist.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   file.c /Fofile.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   scan.c /Foscan.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   map.c /Fomap.obj
cl /nologo -c /O2 /WX /TP /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   str.c /Fostr.obj
lib /nologo obj.obj sort.obj list.obj file.obj scan.obj map.obj str.obj /OUT:llib_static.lib
