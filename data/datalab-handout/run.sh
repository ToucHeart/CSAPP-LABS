#!/bin/bash

make clean
make
read name
if [ "$name" == "all" ]
then
  ./btest;
else
  ./btest -f $name;
fi

