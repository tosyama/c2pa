all:
	cmake --build build
	cd build/test && ./tester

