#include <stdio.h>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"


#ifdef __clang__
#pragma clang diagnostic ignored "-Weverything"
#endif


static int _allocs = 0;

void* operator new( size_t size )
{
    ++_allocs;
    return malloc( size );
}

void* operator new[]( size_t size )
{
    ++_allocs;
    return malloc( size );
}

void operator delete( void* memory )
{
    --_allocs;
    free( memory );
}

void operator delete( void* memory, size_t size )
{
    --_allocs;
    free( memory );
}

void operator delete[]( void* memory )
{
    --_allocs;
    free( memory );
}

void operator delete[]( void* memory, size_t size )
{
    --_allocs;
    free( memory );
}

void test()
{
    tinygltf::TinyGLTF  loader;
    tinygltf::Model     model;
    
    bool status = loader.LoadASCIIFromFile( &model, NULL, NULL, "box.gltf" );
    printf( "status: %d\n", status );
}

int main()
{
    printf( "before: %d\n", _allocs );
    test();
    printf( "after:  %d\n", _allocs );
    return 0;
}

