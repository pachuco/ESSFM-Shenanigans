@echo off

set gccbase=G:\p_files\rtdk\i686-8.1.0-win32-dwarf-rt_v6-rev0\mingw32
set fbcbase=G:\p_files\rtdk\FBC
set PATH=%PATH%;%gccbase%\bin;%fbcbase%

set opts=-std=c99 -mconsole -Os -s -Wall -Wextra
set link=-lcomdlg32

set compiles=src\main.c
set outname=ESS

del %outname%.exe
gcc -o %outname%.exe %compiles% %opts% %link% 2> %outname%_err.log
IF %ERRORLEVEL% NEQ 0 (
    echo oops %outname%!
    pause
)