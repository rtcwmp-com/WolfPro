all: clean dependencies binaries-linux binaries-windows build-pk3-crosscompiled

dependencies:
	cd .. && bash fetch-dependencies.sh
	mkdir -p ../build
	cp ../deps/libjpeg-turbo/build-win/libjpeg-62.dll ../build
	cp ../deps/curl-win/curl/bin/libcurl.dll ../build

binaries-linux:
	mkdir -p ../build
	cd ../build && CC=clang CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 cmake -G Ninja -DCMAKE_LIBRARY_PATH=/usr/lib/i386-linux-gnu -DCMAKE_PREFIX_PATH=/usr/lib/i386-linux-gnu/cmake/ -DBUILD_CLIENT=OFF  .. && ninja
	cd ../build && rm -rf CMakeCache.txt  CMakeFiles build.ninja cmake_install.cmake  librenderer.a

binaries-windows:
	mkdir -p ../build
	cd ../build && \
	cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="../cmake/clang-cl-msvc.cmake" -DHOST_ARCH=x86 -DLLVM_NATIVE_TOOLCHAIN=/usr/ -DMSVC_BASE=../deps/xwin/crt -DWINSDK_BASE=../deps/xwin/sdk -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Release  && ninja
	cd ../build && rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake *.lib build.ninja *.user

build-pk3-crosscompiled:
	cd ../build/rtcwmod && zip -r rtcwmod_bin.pk3 *.dll *.so

clean:
	rm -rf ../build

clean-deps:
	rm -rf ../deps