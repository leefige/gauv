@echo off
set ANTLR_JAR=antlr-4.9.2-complete.jar
set PROJ_ROOT=%~dp0..
@echo on
java -cp %PROJ_ROOT%\lib\%ANTLR_JAR% org.antlr.v4.Tool %PROJ_ROOT%\src\grammar\*.g4 -o %PROJ_ROOT%\src\frontend -Dlanguage=Python3
