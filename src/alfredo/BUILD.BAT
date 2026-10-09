@echo off
set PATH=C:\BIN
set INCLUDE=C:\INCLUDE
set LIB=C:\LIB
if exist ALFREDO.EXE del ALFREDO.EXE
cl /nologo /c /G0 /AS /O /W3 src\ALFREDO.C > BUILD.LOG
if errorlevel 1 goto done
link /NOI /MAP /CP:4096 ALFREDO.OBJ,ALFREDO.EXE,ALFREDO.MAP,,; >> BUILD.LOG
:done
exit
