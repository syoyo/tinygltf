EXTRA_CXXFLAGS := -fsanitize=address -fno-omit-frame-pointer -Wall -Werror -Weverything -Wno-c++11-long-long -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded

all:
	clang++ -std=c++11 -g -O1 -I../../ -I../common $(EXTRA_CXXFLAGS) -o mesh-modify mesh-modify.cc mesh-util.cc
