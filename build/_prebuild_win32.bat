@echo off

REM This file takes two arguments
REM 1 = "app" or "meta"
REM 2 = "debug" or "release"
REM 3 = "msvc" or "clang"

set PrebuildCalled=1

set ProjectDevFolder=%~dp0
set ProjectDevPath=%ProjectDevFolder:~0,-7%

set SourceCodePath=%ProjectDevPath%\src\%1

set MetaProgramPath=%ProjectDevPath%\meta_run_tree\win32_%3\%2

set ProjectRunTree=%ProjectDevPath%\%1_run_tree\win32_%3
set BuildPath=%ProjectRunTree%\%2
set StatsPath=%ProjectRunTree%\stats
set StatsFile=%1_win32_%3_%2_build_time.ctm

IF NOT EXIST %BuildPath% mkdir %BuildPath%
IF NOT EXIST %StatsPath% mkdir %StatsPath%

C:\apps\ctime\ctime.exe -begin %StatsPath%\%StatsFile%
echo.
echo BUILDING TO %BuildPath%
echo STATS IN %StatsPath%\%StatsFile%
echo.
