rm -rf build
CXX=clang++ CC=clang meson build -Db_sanitize=address -Db_lundef=false --buildtype=debug
cp Makefile.meson build/Makefile
