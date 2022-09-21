WASI_VERSION=16
WASI_VERSION_FULL=${WASI_VERSION}.0

WASI_SDK_PATH=$(HOME)/local/wasi-sdk-${WASI_VERSION_FULL}

CC=${WASI_SDK_PATH}/bin/clang
CXX=${WASI_SDK_PATH}/bin/clang++
CXXFLAGS=-fno-rtti -fno-exceptions -g -Os

all:
	$(CXX) ../loader_example.cc $(CXXFLAGS) -I../
