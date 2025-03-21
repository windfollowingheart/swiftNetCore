#!/bin/bash

set -e

if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*
rm -rf `pwd`/lib/*

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo so库拷贝到 /usr/lib PATH
if [ ! -d /usr/include/swiftNetCore ]; then
    mkdir /usr/include/swiftNetCore
fi

for header in `ls ./src/*.h`
do
    cp $header /usr/include/swiftNetCore
done

cp `pwd`/lib/libswiftNetCore.so /usr/lib

# ldconfig