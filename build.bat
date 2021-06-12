@echo off
set VERY_IMPORTANT_SAFETY=-DENABLE_HARDWARE_ACCESS

call getcomp rosbe

::must not contain spaces!
set buttiolocation=C:\p_files\prog\_proj\CodeCocks\buttio

set opts=-std=c99 -mwindows -Os -s -Wall -Wextra %VERY_IMPORTANT_SAFETY%
set linkinc=-I%buttiolocation%\src\ -L%buttiolocation%\bin\
set linkinc=%linkinc% -lbuttio -lcomdlg32 -lcomctl32
set compiles=src\maingui.c src\logplayer.c src\file.c
set outname=ESS

del %outname%.exe

xcopy "%buttiolocation%\bin\buttio.sys" .\ /c /Y
gcc -o %outname%.exe %compiles% %opts% %linkinc% 2> %outname%_err.log
IF %ERRORLEVEL% NEQ 0 (
    echo oops %outname%!
    pause
)