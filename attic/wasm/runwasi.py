import wasmtime.loader

# Assuming `your_wasm_file.wasm` is in the python load path...
import loader_example

# Now you're compiled and instantiated and ready to go!
print(loader_example.main())
