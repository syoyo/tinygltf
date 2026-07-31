import json
import struct
import sys

if len(sys.argv) < 2:
    print("Need input.gltf")
    sys.exit(-1)

gltf_file = sys.argv[1]
#bin_file = sys.argv[1]

gltf = json.loads(open(gltf_file, "r", encoding='utf-8').read())
print(gltf)

#data = open(bin_file, "rb").read()
#print(len(data))
#
#out = data[100:]
#print(len(out))
#
#with open("output.bin", "wb") as f:
#    f.write(out)
#
#print("DONE!")
