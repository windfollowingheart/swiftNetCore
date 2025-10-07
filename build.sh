#!/bin/bash

set -e

SOURCE_DIR=.
echo $SOURCE_DIR

if [ ! -d $SOURCE_DIR/build ]; then
    mkdir ./build
fi

rm -rf $SOURCE_DIR/build/*
rm -rf $SOURCE_DIR/lib/*

cd $SOURCE_DIR/build &&
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
    sudo cp $header /usr/include/swiftNetCore
done

for header in `ls ./src/http/*.h`
do
    sudo cp $header /usr/include/swiftNetCore/http
done

sudo cp $SOURCE_DIR/lib/libswiftNetCore.so /usr/lib

# ldconfig