all: clean dependencies64 binaries-linux64 binaries-windows64 build-pk3-crosscompiled64

dependencies:
	cd .. && bash fetch-dependencies.sh
	mkdir -p ../build
	cp ../deps/libjpeg-turbo/build-win/libjpeg-62.dll ../build
	cp ../deps/curl-win/curl/bin/libcurl.dll ../build

dependencies64:
	cd .. && bash fetch-dependencies64.sh
	mkdir -p ../build64
	cp ../deps64/libjpeg-turbo/build-win/libjpeg-62.dll ../build64
	cp ../deps64/curl-win/curl/bin/libcurl-x64.dll ../build64

binaries-linux:
	mkdir -p ../build
	cd ../build && CC=clang CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 cmake -G Ninja -DCMAKE_LIBRARY_PATH=/usr/lib/i386-linux-gnu -DCMAKE_PREFIX_PATH=/usr/lib/i386-linux-gnu/cmake/ -DBUILD_CLIENT=OFF  .. && ninja
	cd ../build && rm -rf CMakeCache.txt  CMakeFiles build.ninja cmake_install.cmake  librenderer.a

binaries-linux64:
	mkdir -p ../build64
	cd ../build64 && cmake -G Ninja -DBUILD_CLIENT=OFF  .. && ninja
	cd ../build64 && rm -rf CMakeCache.txt  CMakeFiles build.ninja cmake_install.cmake  librenderer.a

binaries-windows:
	mkdir -p ../build
	cd ../build && \
	cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="../cmake/clang-cl-msvc.cmake" -DHOST_ARCH=x86 -DLLVM_NATIVE_TOOLCHAIN=/usr/ -DMSVC_BASE=../deps/xwin/crt -DWINSDK_BASE=../deps/xwin/sdk -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Release  && ninja
	cd ../build && rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake *.lib build.ninja *.user

binaries-windows64:
	mkdir -p ../build64
	cd ../build64 && \
	cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="../cmake/clang-cl-msvc.cmake" -DHOST_ARCH=x64 -DLLVM_NATIVE_TOOLCHAIN=/usr/ -DMSVC_BASE=../deps64/xwin/crt -DWINSDK_BASE=../deps64/xwin/sdk -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Release  && ninja
	cd ../build && rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake *.lib build.ninja *.user

build-pk3-crosscompiled:
	cd ../build/wolfpro && zip -r wolfpro_bin.pk3 *.dll *.so

build-pk3-crosscompiled64:
	cd ../build64/wolfpro && zip -r wolfpro_bin.pk3 *.dll *.so


clean:
	rm -rf ../build
	rm -rf ../build64

clean-deps:
	rm -rf ../deps
	rm -rf ../deps64