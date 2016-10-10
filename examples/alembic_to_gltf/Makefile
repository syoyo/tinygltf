ABC_INC := -I${HOME}/work/alembic-dist/usr/local/include
ABC_LDFLAGS := -L${HOME}/work/alembic-dist/usr/local/lib -lAlembic

ILMBASE_INC := -I/usr/include/OpenEXR

# Suppress some C++ warnings(in clang).
EXTRA_CXXFLAGS := -Wno-deprecated-register


all:
	clang++ -std=c++11 -g -o abc2gltf $(ABC_INC) $(ILMBASE_INC) $(EXTRA_CXXFLAGS) main.cc $(ABC_LDFLAGS)
