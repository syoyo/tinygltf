Currently it causes stack-overflow in runtime.

```
$ ./tinygltf-validate schema_dir ../../models/Cube/Cube.gltf

...

==17033==ERROR: AddressSanitizer: stack-overflow on address 0x7fff55415d68 (pc 0x0000004c62de bp 0x7fff554165c0 sp 0x7fff55415d70 T0)
    #0 0x4c62dd in __asan_memset /tmp/final/llvm.src/projects/compiler-rt/lib/asan/asan_interceptors_memintrinsics.cc:27:3
    #1 0x53a7a7 in std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) /usr/lib/gcc/x86_64-linux-gnu/5.4.0/../../../../include/c++/5.4.0/bits/stl_vector.h:91:37
    #2 0x53a6f1 in std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) /usr/lib/gcc/x86_64-linux-gnu/5.4.0/../../../../include/c++/5.4.0/bits/stl_vector.h:135:9
    #3 0x53a9e2 in std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > const&) /usr/lib/gcc/x86_64-linux-gnu/5.4.0/../../../../include/c++/5.4.0/bits/stl_vector.h:319:9
    #4 0x584313 in valijson::ValidationVisitor<valijson::adapters::Json11Adapter>::ValidationVisitor(valijson::ValidationVisitor<valijson::adapters::Json11Adapter> const&) /home/syoyo/work/tinygltf/examples/validator/valijson/include/valijson/validation_visitor.hpp:26:7
```

