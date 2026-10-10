@echo off

FOR /F "tokens=* USEBACKQ" %%g IN (`git describe --dirty`) do (SET "TAG=%%g")

powershell -Command "(gc ..\pokketsx-qt\pokketsx-qt.rc) -replace '1,0,0,1', '"%TAG:~0,1%","%TAG:~2,1%","%TAG:~4,4%",0' | Out-File -encoding ASCII ..\pokketsx-qt\pokketsx-qt.rc"
powershell -Command "(gc ..\pokketsx-qt\pokketsx-qt.rc) -replace '1.0.0.1', '"%TAG:~0,1%"."%TAG:~2,1%"."%TAG:~4,4%"' | Out-File -encoding ASCII ..\pokketsx-qt\pokketsx-qt.rc"
