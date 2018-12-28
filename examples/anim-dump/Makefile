# Use this for strict compilation check(will work on clang 3.8+)
#EXTRA_CXXFLAGS := -fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long -Wno-c++98-compat

all:
	clang++ -I../../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o anim-dump anim-dump.cc

