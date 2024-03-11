clang := clang++
includes := -I -Iglm -Iglfw/include -I../../wasmtime/include -I../../
options := -std=c++17 -stdlib=libc++ -fms-extensions \
           -Wno-error=extra-tokens -Wno-quoted-include-in-framework-header -O3
link := -L/pk -Wl,-rpath,/../../wasmtime/lib ../../wasmtime/lib/libwasmtime.so

build:
	$(clang) $(includes) $(options) $(link) \
	   -o opengl demo.cpp gl3w.c ../../wander.cpp

run: build
	./opengl