#!/bin/bash

# compile script

# declare default parameters
threads="6"
dlog="ON"
dconsole="ON"
dfile="ON"

while getopts "j:l:c:f" opt; do
  case $opt in
    j)
      threads=$OPTARG
      ;;
    l)
      dlog="OFF"
      ;;
    c)
      dconsole="OFF"
      ;;
    f)
      dfile="OFF"
      ;;
  esac
done

if [ ! -d "build" ]; then
    mkdir build
fi
cd build

# Execute the cmake with the stored parameters
cmake .. -DLOG=$dlog -DCONSOLE=$dconsole -DFILE=$dfile
cmake --build . -j$threads


