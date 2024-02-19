@echo off

del *.pdb

set COMMON_FLAGS=/Od /W3 /Z7 /DUNICODE_X /EHsc /wd4996 /nologo /MT
set BUILD_FLAGS=%COMMON_FLAGS%  /link opengl32.lib gdi32.lib user32.lib Dxgi.lib D3D11.lib

cl main.cpp /Femain.exe %BUILD_FLAGS% 
REM cl test_win_api_directx_research.cpp /Fecapture.exe %BUILD_FLAGS% 

del *.ilk
del *.obj
