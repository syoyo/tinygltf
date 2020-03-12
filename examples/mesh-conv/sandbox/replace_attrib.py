# Replace accessor id of attributes for speicified mesh
# Usually called after concat_mesh.py
# Example usecase is to replace UV coordinate of a mesh.

import json
import sys, os

attrib_names = ["TEXCOORD_0"]

def check_accessor(src, target):
    if src["componentType"] != target["componentType"]:
        print("componentType mismatch!")
        return False

    if src["count"] != target["count"]:
        print("`count` mismatch!")
        return False

    if src["type"] != target["type"]:
        print("`type` mismatch!")
        return False

    return True

def main():
    if len(sys.argv) < 5:
        print("Needs input.gltf output.gltf source_mesh_name target_mesh_name <source_primid> <target_primid>")
        sys.exit(-1)

    input_filename = sys.argv[1]
    output_filename = sys.argv[2]
    source_mesh_name = sys.argv[3]
    target_mesh_name = sys.argv[4]

    source_primid = 0
    target_primid = 0

    if len(sys.argv) > 5:
        source_primid = int(sys.argv[5])

    if len(sys.argv) > 6:
        target_primid = int(sys.argv[6])

    gltf = json.loads(open(input_filename).read())

    source_mesh_id = -1
    target_mesh_id = -1

    for i in range(len(gltf["meshes"])):
        mesh = gltf["meshes"][i]
        print("mesh[{}].name = {}".format(i, mesh["name"]))

        if target_mesh_name == mesh["name"]:
            target_mesh_id = i

        if source_mesh_name == mesh["name"]:
            source_mesh_id = i

    if source_mesh_id == -1:
        print("source mesh with name [{}] not found.".format(source_mesh_name))
        sys.exit(-1)

    if target_mesh_id == -1:
        print("target mesh with name [{}] not found.".format(target_mesh_name))
        sys.exit(-1)

    print("target: name = {}, id = {}".format(target_mesh_name, target_mesh_id))
    print("source: name = {}, id = {}".format(source_mesh_name, source_mesh_id))

    source_mesh = gltf["meshes"][source_mesh_id]
    target_mesh = gltf["meshes"][target_mesh_id]

    source_prim = source_mesh["primitives"][source_primid]
    target_prim = target_mesh["primitives"][target_primid]

    for attrib in target_prim["attributes"]:
        print("attrib ", attrib)
        if attrib in attrib_names:
            if attrib in source_prim["attributes"]:
                target_accessor_id = target_prim["attributes"][attrib]
                src_accessor_id = source_prim["attributes"][attrib]

                if check_accessor(gltf["accessors"][src_accessor_id], gltf["accessors"][target_accessor_id]):
                    gltf["meshes"][target_mesh_id]["primitives"][target_primid]["attributes"][attrib] = src_accessor_id
                    print("Replaced accessor id for attrib {} from {} to {}".format(attrib, target_accessor_id, src_accessor_id))
                else:
                    print("Accessor type/format is not identical. Skip replace")
                    print("  attrib {}".format(attrib))

            else:
                print("attribute[{}] not found in source primitive: mesh[{}].primitives[{}]".format(attrib, source_mesh_name, source_primid))

    with open(output_filename, "w") as f:
        f.write(json.dumps(gltf, indent=2))

    print("Merged glTF was exported to : ", output_filename)
main()
