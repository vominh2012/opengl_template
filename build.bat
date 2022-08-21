@echo off

set COMMON_FLAGS=/Od /W3 /Z7 /DUNICODE /EHsc /wd4996 /nologo /MT
set BUILD_FLAGS=%COMMON_FLAGS%  /link opengl32.lib gdi32.lib user32.lib

cl main.cpp /Femain.exe %BUILD_FLAGS% 

del *.ilk
del *.obj
