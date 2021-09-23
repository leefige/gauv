#!/bin/bash
PROJ_ROOT=`pwd`/..
$PROJ_ROOT/tools/antlr4.sh $PROJ_ROOT/src/grammar/*.g4 -o $PROJ_ROOT/src/frontend -Dlanguage=Cpp -listener -visitor -package mpc
