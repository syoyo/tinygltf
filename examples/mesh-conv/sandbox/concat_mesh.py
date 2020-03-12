# concat mesh to glTF

import json
import sys, os

prefix = "added/"

def main():
    if len(sys.argv) < 4:
        print("Needs source.gltf target.gltf output.gltf")
        sys.exit(-1)

    source_filename = sys.argv[1]
    target_filename = sys.argv[2]
    output_filename = sys.argv[3]

    source = json.loads(open(source_filename).read())
    target = json.loads(open(target_filename).read())

    num_target_meshes = len(target["meshes"])
    num_target_buffers = len(target["buffers"])
    num_target_bufferViews = len(target["bufferViews"])
    num_target_accessors = len(target["accessors"])
    num_target_materials = len(target["materials"])
    print("num_target_meshes: ", num_target_meshes)
    print("num_target_buffers: ", num_target_buffers)
    print("num_target_bufferViews: ", num_target_bufferViews)
    print("num_target_accessors: ", num_target_accessors)
    print("num_target_materials: ", num_target_accessors)

    num_source_meshes = len(source["meshes"])
    num_source_buffers = len(source["buffers"])
    num_source_bufferViews = len(source["bufferViews"])
    num_source_accessors = len(source["accessors"])
    num_source_materials = len(source["materials"]) if "materials" in source else 0
    print("num_source_meshes: ", num_source_meshes)
    print("num_source_buffers: ", num_source_buffers)
    print("num_source_bufferViews: ", num_source_bufferViews)
    print("num_source_accessors: ", num_source_accessors)
    print("num_source_materials: ", num_source_materials)

    #
    # Adjust name and index
    #
    for i in range(len(source["buffers"])):
        if "name" in source["buffers"][i]:
            source["buffers"][i]["name"] = prefix + source["buffers"][i]["name"]

    for i in range(len(source["bufferViews"])):
        if "name" in source["bufferViews"][i]:
            source["bufferViews"][i]["name"] = prefix + source["bufferViews"][i]["name"]

        source["bufferViews"][i]["buffer"] += num_target_buffers

    for i in range(len(source["accessors"])):
        if "name" in source["accessors"][i]:
            source["accessors"][i]["name"] = prefix + source["accessors"][i]["name"]

        source["accessors"][i]["bufferView"] += num_target_bufferViews

    for i in range(len(source["meshes"])):
        mesh = source["meshes"][i]

        if "name" in mesh:
            source["meshes"][i]["name"] = prefix + source["meshes"][i]["name"]

        for primid in range(len(mesh["primitives"])):
            for attrib in mesh["primitives"][primid]["attributes"]:
                #print(source["meshes"][i]["primitives"][primid]["attributes"][attrib])
                source["meshes"][i]["primitives"][primid]["attributes"][attrib] += num_target_accessors


            source["meshes"][i]["primitives"][primid]["indices"] += num_target_accessors
            if "material" in source["meshes"][i]["primitives"][primid]:
                source["meshes"][i]["primitives"][primid]["material"] += num_target_materials


    #
    # Append mesh info
    #
    target["buffers"] += source["buffers"]
    target["bufferViews"] += source["bufferViews"]
    target["meshes"] += source["meshes"]
    target["accessors"] += source["accessors"]
    if "materials" in source:
        target["materials"] += source["materials"]

    #
    # add some info
    #
    extraInfo = {}
    extraInfo["num_target_meshes"] = num_target_meshes
    extraInfo["num_target_buffers"] = num_target_buffers
    extraInfo["num_target_bufferViews"] = num_target_bufferViews
    extraInfo["num_target_accessors"] = num_target_accessors
    extraInfo["num_target_materials"] = num_target_materials

    extraInfo["num_source_meshes"] = num_source_meshes
    extraInfo["num_source_buffers"] = num_source_buffers
    extraInfo["num_source_bufferViews"] = num_source_bufferViews
    extraInfo["num_source_accessors"] = num_source_accessors
    extraInfo["num_source_materials"] = num_source_materials

    target["asset"]["extras"] = extraInfo

    with open(output_filename, "w") as f:
        f.write(json.dumps(target, indent=2))

    print("Merged glTF was exported to : ", output_filename)
main()
