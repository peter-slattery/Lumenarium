@echo off

IF NOT "%PrebuildCalled%"=="1" GOTO error

C:\apps\ctime\ctime.exe -end %StatsPath%\%StatsFile% %LastError%
C:\apps\ctime\ctime.exe -stats %StatsPath%\%StatsFile%
set PrebuildCalled=0
GOTO:eof

:error
echo ERROR: _prebuild_win32.bat was not called before _postbuild_win32.bat.