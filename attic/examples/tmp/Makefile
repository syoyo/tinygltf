
# Use this for strict compilation check(will work on clang 3.8+)
EXTRA_CXXFLAGS := -Wall -Werror -Weverything -Wno-c++11-long-long -Wno-c++98-compat

all:
	clang++  $(EXTRA_CXXFLAGS) -I../../ -std=c++11 -g -O0 -o main main.cpp

