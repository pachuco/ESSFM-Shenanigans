@echo off
::set VERY_IMPORTANT_SAFETY=-DENABLE_HARDWARE_ACCESS

set gccbase=G:\p_files\rtdk\i686-8.1.0-win32-dwarf-rt_v6-rev0\mingw32\bin
set fbcbase=G:\p_files\rtdk\FBC
set PATH=%PATH%;%gccbase%;%fbcbase%

set opts=-std=c99 -mwindows -Os -s -Wall -Wextra %VERY_IMPORTANT_SAFETY%
set link=-lcomdlg32 -lcomctl32
set compiles=src\maingui.c src\logplayer.c src\file.c src\sup_inout.c
set outname=ESS

del %outname%.exe
gcc -o %outname%.exe %compiles% %opts% %link% 2> %outname%_err.log
IF %ERRORLEVEL% NEQ 0 (
    echo oops %outname%!
    pause
)