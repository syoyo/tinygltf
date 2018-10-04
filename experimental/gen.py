#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, os
import subprocess
import json
from pprint import pprint
import jsonref

# glTF 2.0
schema_files = [
    "glTF.schema.json"
]

def main():
    if len(sys.argv) < 2:
        print("Requires path to glTF scheme directory.")
        sys.exit(-1)
    
    gltf_schema_dir = sys.argv[1]

    gltf_schema_filepath = os.path.join(gltf_schema_dir, schema_files[0])
    if not os.path.exists(gltf_schema_filepath):
        print("File not found: {}".format(gltf_schema_filepath))
        sys.exit(-1)

    gltf_schema_uri = 'file://{}/'.format(gltf_schema_dir)
    with open(gltf_schema_filepath) as schema_file:
        j = jsonref.loads(schema_file.read(), base_uri=gltf_schema_uri, jsonschema=True)
        pprint(j)

    

main()
