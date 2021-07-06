all:
	cmake --build build
	cd build/test && ./tester
lcov:
	cd build && lcov -c -d . -b src -o all.info
	cd build && lcov -e all.info */c2pa/src/* -o lcov.info
