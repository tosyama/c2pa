#!/bin/bash
# Coverage check tool

target_bin="cparser"
target_src="clexer"

rm build/src/CMakeFiles/cparser.dir/*.gcda
rm build/src/CMakeFiles/c2pa.dir/*.gcda

cd build
cmake -DOUTPUT_COVERAGE=ON ..
cd ..
make
gcov build/src/CMakeFiles/${target_bin}.dir/${target_src}.cpp.gcda | grep -A 1 -E ${target_src}
rm $(ls ./*.gcov | grep -E -v ${target_src})
rm build/CMakeCache.txt

