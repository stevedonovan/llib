cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-obj.c /Fotest-obj.obj
link /nologo test-obj.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-obj.exe
test-obj.exe >test-obj-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-list.c /Fotest-list.obj
link /nologo test-list.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-list.exe
test-list.exe >test-list-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-file.c /Fotest-file.obj
link /nologo test-file.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-file.exe
test-file.exe >test-file-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-scan.c /Fotest-scan.obj
link /nologo test-scan.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-scan.exe
test-scan.exe >test-scan-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-seq.c /Fotest-seq.obj
link /nologo test-seq.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-seq.exe
test-seq.exe >test-seq-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-map.c /Fotest-map.obj
link /nologo test-map.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-map.exe
test-map.exe >test-map-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-str.c /Fotest-str.obj
link /nologo test-str.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-str.exe
test-str.exe >test-str-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-template.c /Fotest-template.obj
link /nologo test-template.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-template.exe
test-template.exe >test-template-output
cl /nologo -c /O2 /WX /TP /I..  /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG   test-json.c /Fotest-json.obj
link /nologo test-json.obj  /LIBPATH:../llib  llib_static.lib  /OUT:test-json.exe
test-json.exe >test-json-output
