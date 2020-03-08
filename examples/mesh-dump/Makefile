EXTRA_CXXFLAGS := -fsanitize=address -fno-omit-frame-pointer -Wall -Werror -Weverything -Wno-c++11-long-long -Wno-c++98-compat

all:
	clang++ -std=c++11 -I../../ $(EXTRA_CXXFLAGS) -o mesh-dump mesh-dump.cc
