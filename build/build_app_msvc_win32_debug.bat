@echo off

SET MyPath=%~dp0
SET MyPath=%MyPath:~0,-1%

call %MyPath%\_prebuild_win32.bat app debug msvc
call %MyPath%\setup_cl.bat

SET CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except- -IC:\programs-dev\gs_libs\src

SET CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -I%CommonLibs%  -Od %CommonCompilerFlags%

SET CommonLinkerFlags= -opt:ref -incremental:no

SET DLLExports=/EXPORT:InitializeApplication /EXPORT:UpdateAndRender /EXPORT:CleanupApplication /EXPORT:ReloadStaticData

pushd %BuildPath%

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB TO WRITE > lock.tmp

cl %CommonCompilerFlags% %SourceCodePath%\foldhaus_app.cpp /Fefoldhaus.dll /LD /link %CommonLinkerFlags% %DLLExports%
SET LastError=%ERRORLEVEL%

del lock.tmp

cl %CommonCompilerFlags% %SourceCodePath%\platform_win32\win32_foldhaus.cpp /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib  opengl32.lib dsound.lib Ws2_32.lib Comdlg32.lib Winspool.lib


REM COMPILE UTILITY EXES

cl %CommonCompilerFlags% %ProjectDevPath%\src\serial_monitor\first.cpp /Feserial_monitor.exe /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib Winspool.lib

cl %CommonCompilerFlags% %ProjectDevPath%\src\sculpture_gen\gen_blumen_lumen.cpp /Fegen_blumen_lumen.exe /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib

REM COMPILE AND RUN TESTS
cl %CommonCompilerFlags% %ProjectDevPath%\src\tests\sanity_tests.cpp /Fesanity_tests.exe /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib

ECHO SANITY TESTS BEGIN
sanity_tests.exe
ECHO SANITY TESTS END

popd

call %MyPath%\_postbuild_win32.bat