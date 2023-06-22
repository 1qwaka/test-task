#!/bin/bash

mkdir -p build
cd build
cmake ..

if cmake --build . 
then
    echo ""
    ./app
fi