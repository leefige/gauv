@echo off
set PROJ_ROOT=%~dp0..
@echo on
%PROJ_ROOT%\tools\antlr4.bat %PROJ_ROOT%\src\grammar\*.g4 -o %PROJ_ROOT%\src\frontend -Dlanguage=Cpp -listener -visitor -package mpc
