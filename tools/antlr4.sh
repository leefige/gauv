#!/bin/bash
ANTLR_JAR="antlr-4.9.2-complete.jar"
PROJ_ROOT=`pwd`/..
java -Xmx500M -cp "$PROJ_ROOT/lib/$ANTLR_JAR:$CLASSPATH" org.antlr.v4.Tool $*
