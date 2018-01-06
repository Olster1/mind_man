@echo off
set CommonCompilerFlags= -Od -MTd -nologo -GR- -Gm- -EHa- -Oi -WX -W4 -wd4189 -wd4201 -wd4100 -wd4505 -FC -Z7
set CommonCompilerFlags= %CommonCompilerFlags% -DINTERNAL_BUILD=1 -DWIN32_BUILD=1 -DSDL_PORT=1
set CommonLinkerFlags= -incremental:no -opt:ref SDL2main.lib SDL2.lib user32.lib gdi32.lib Winmm.lib opengl32.lib  

IF NOT EXIST ..\..\..\build_extra_projects mkdir ..\..\..\build_extra_projects
pushd ..\..\..\build_extra_projects
REM 32-bit build

REM cl %CommonCompilerFlags% ..\mind_man\code\meta_generator.cpp -Fmcalm_win32.map /link %CommonLinkerFlags%

pushd ..\mind_man\code
REM ..\..\..\build_extra_projects\meta_generator.exe
popd

cl %CommonCompilerFlags% ..\mind_man\code\calm_sdl.cpp -Fmcalm_sdl.map /link %CommonLinkerFlags% 
popd


