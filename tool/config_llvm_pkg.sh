#!/usr/bin/env bash

version=$(llvm-config --version)
filename=$1

mkdir -p `dirname $filename`
touch $filename

machine="$(uname -s)"
case "${machine}" in
  Darwin*)  libs="-lc++";;
  *)        libs=""
esac

echo Name: LLVM > $filename
echo Description: Low-level Virtual Machine compiler framework >> $filename
echo Version: $(echo ${version} | sed 's/\([0-9.]\+\).*/\1/') >> $filename
echo URL: http://www.llvm.org/ >> $filename
echo Libs: ${libs} -L$(llvm-config --libdir --system-libs --libs analysis bitwriter core native passes target) >> $filename
echo Cflags: -I$(llvm-config --includedir) >> $filename

echo "$filename written:"
cat $filename
