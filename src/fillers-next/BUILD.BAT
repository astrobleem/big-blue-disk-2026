@echo off
set PATH=C:\BIN
set INCLUDE=C:\INCLUDE
set LIB=C:\LIB
cl /nologo /c /G0 /AS /Os /W3 HORSE.C DRUGWAR.C TEST.C > BUILD.LOG
if errorlevel 1 goto fail
link /NOI /MAP /CP:4096 HORSE.OBJ,HORSE.EXE,HORSE.MAP,,; >> BUILD.LOG
if errorlevel 1 goto fail
link /NOI /MAP /CP:4096 DRUGWAR.OBJ,DRUGWAR.EXE,DRUGWAR.MAP,,; >> BUILD.LOG
if errorlevel 1 goto fail
link /NOI /MAP /CP:4096 TEST.OBJ,TEST.EXE,TEST.MAP,,; >> BUILD.LOG
if errorlevel 1 goto fail
TEST > DOSTEST.TXT
if errorlevel 1 goto fail
HORSE /demo > HORSEQA.TXT
DRUGWAR /demo > TRADEQA.TXT
echo PASS >> BUILD.LOG
goto done
:fail
echo FAIL >> BUILD.LOG
:done
exit
