@echo off
set PATH=C:\BIN
set INCLUDE=C:\INCLUDE
set LIB=C:\LIB
cl /nologo /c /G0 /AS /O /W3 FLOW88.C > BUILD.LOG
if errorlevel 1 goto fail
link /NOI /MAP /CP:4096 FLOW88.OBJ,FLOW88.EXE,FLOW88.MAP,,; >> BUILD.LOG
if errorlevel 1 goto fail
echo PASS
goto end
:fail
echo FAIL: see BUILD.LOG
exit /b 1
:end
