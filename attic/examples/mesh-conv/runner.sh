#!/bin/bash

./mesh-conv --op gltf2obj -i ../hairsimtest/sim.gltf

#./mesh-conv --op obj2gltf --verbose -i hair_bfShape_0.obj
./mesh-conv --op obj2gltf -i hair_bfShape_1.obj -o hair_bfShape_1.gltf

