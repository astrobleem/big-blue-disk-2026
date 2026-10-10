@echo off
rem Alfredo: After Hours, Microsoft C 6 reference build (8088 small model).
rem Run from src\ALFREDO2 with the src\SOUND directory beside it.
set PATH=C:\BIN
set INCLUDE=C:\INCLUDE
set LIB=C:\LIB
if exist ALFREDO.EXE del ALFREDO.EXE
cl /nologo /c /G0 /AS /Ox /W3 ALFREDO.C > BUILD.LOG
if errorlevel 1 goto fail
cl /nologo /c /G0 /AS /Ox /W3 ..\SOUND\DOSSND.C >> BUILD.LOG
if errorlevel 1 goto fail
link /NOI /MAP /CP:4096 /ST:4096 ALFREDO.OBJ DOSSND.OBJ,ALFREDO.EXE,ALFREDO.MAP,,; >> BUILD.LOG
if errorlevel 1 goto fail
echo PASS > STATUS.TXT
goto end
:fail
echo FAIL > STATUS.TXT
:end
