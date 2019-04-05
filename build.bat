@echo off

set gccbase=G:\p_files\rtdk\mingw32-gcc5
set fbcbase=G:\p_files\rtdk\FBC
set PATH=%PATH%;%gccbase%\bin;%fbcbase%

set opts=-std=c99 -mconsole -Os -s -Wall -Wextra
set link=

set compiles=src\main.c
set outname=ESS

del %outname%.exe
gcc -o %outname%.exe %compiles% %opts% %link% 2> %outname%_err.log
IF %ERRORLEVEL% NEQ 0 (
    echo oops %outname%!
    pause
)