#!/bin/zsh

JAR=${HOME}/ProjectsExtras/WIBL/closure-compiler/closure-compiler-v20231112.jar

for file in js/*.js
do
    echo Compiling $file ...
    java -jar $JAR --js $file --js_output_file ${file%.js}_compiled.js
done

python inline_flatten.py

rm js/*_compiled.js

exit 0
