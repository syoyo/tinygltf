# concat mesh to glTF
# support multiple source glTF to me concatenated to target.gltf

import json
import sys, os

g_prefix = "added/"

def concat(source, target, offsets, prefix, extraInfo):
    # Args:
    # source: dict(modified), target: dict(modified),
    # offsets: dict(modified). contains offset table.
    # prefix: str
    # extraInfo: dict(inout)

    num_source_meshes = len(source["meshes"])
    num_source_buffers = len(source["buffers"])
    num_source_bufferViews = len(source["bufferViews"])
    num_source_accessors = len(source["accessors"])
    num_source_materials = len(source["materials"]) if "materials" in source else 0
    print("src =====")
    print("num_src_meshes: ", num_source_meshes)
    print("num_src_buffers: ", num_source_buffers)
    print("num_src_bufferViews: ", num_source_bufferViews)
    print("num_src_accessors: ", num_source_accessors)
    print("num_src_materials: ", num_source_materials)


    #
    # Adjust name and index offset
    #
    for i in range(len(source["buffers"])):
        if "name" in source["buffers"][i]:
            source["buffers"][i]["name"] = prefix + source["buffers"][i]["name"]

    for i in range(len(source["bufferViews"])):
        if "name" in source["bufferViews"][i]:
            source["bufferViews"][i]["name"] = prefix + source["bufferViews"][i]["name"]

        source["bufferViews"][i]["buffer"] += offsets["buffers"]

    for i in range(len(source["accessors"])):
        if "name" in source["accessors"][i]:
            source["accessors"][i]["name"] = prefix + source["accessors"][i]["name"]

        source["accessors"][i]["bufferView"] += offsets["bufferViews"]

    for i in range(len(source["meshes"])):
        mesh = source["meshes"][i]

        if "name" in mesh:
            source["meshes"][i]["name"] = prefix + source["meshes"][i]["name"]

        for primid in range(len(mesh["primitives"])):
            for attrib in mesh["primitives"][primid]["attributes"]:
                #print(source["meshes"][i]["primitives"][primid]["attributes"][attrib])
                source["meshes"][i]["primitives"][primid]["attributes"][attrib] += offsets["accessors"]


            source["meshes"][i]["primitives"][primid]["indices"] += offsets["accessors"]
            if "material" in source["meshes"][i]["primitives"][primid]:
                source["meshes"][i]["primitives"][primid]["material"] += offsets["materials"]


    #
    # Append mesh info
    #
    target["buffers"] += source["buffers"]
    target["bufferViews"] += source["bufferViews"]
    target["meshes"] += source["meshes"]
    target["accessors"] += source["accessors"]
    if "materials" in source:
        target["materials"] += source["materials"]

    # assume `prefix` is unique
    extraInfo["num_{}_meshes".format(prefix)] = num_source_meshes
    extraInfo["num_{}_buffers".format(prefix)] = num_source_buffers
    extraInfo["num_{}_bufferViews".format(prefix)] = num_source_bufferViews
    extraInfo["num_{}_accessors".format(prefix)] = num_source_accessors
    extraInfo["num_{}_materials".format(prefix)] = num_source_materials

    # update offsets
    offsets["meshes"] += num_source_meshes
    offsets["buffers"] += num_source_buffers
    offsets["bufferViews"] += num_source_bufferViews
    offsets["accessors"] += num_source_accessors
    offsets["materials"] += num_source_materials


def main():
    if len(sys.argv) < 4:
        print("Needs source0.gltf <source1.gltf, ...> target.gltf output.gltf")
        sys.exit(-1)

    num_args = len(sys.argv)
    print("num_args = ", num_args)

    source_filenames = []

    num_srcs = num_args - 3

    for i in range(num_srcs):
        source_filenames.append(sys.argv[1+i])
        print("source[{}] = {}".format(i, sys.argv[1+i]))

    target_filename = sys.argv[num_args - 2]
    output_filename = sys.argv[num_args - 1]
    print("target = ", target_filename)
    print("output = ", output_filename)

    sources = []
    for i in range(num_srcs):
        sources.append(json.loads(open(source_filenames[i]).read()))
    target = json.loads(open(target_filename).read())

    num_target_meshes = len(target["meshes"])
    num_target_buffers = len(target["buffers"])
    num_target_bufferViews = len(target["bufferViews"])
    num_target_accessors = len(target["accessors"])
    num_target_materials = 0
    if "materials" in target:
        num_target_materials = len(target["materials"])

    print("num_target_meshes: ", num_target_meshes)
    print("num_target_buffers: ", num_target_buffers)
    print("num_target_bufferViews: ", num_target_bufferViews)
    print("num_target_accessors: ", num_target_accessors)
    print("num_target_materials: ", num_target_accessors)

    #
    # add some info to asset.extras
    #
    extraInfo = {}
    extraInfo["num_target_meshes"] = num_target_meshes
    extraInfo["num_target_buffers"] = num_target_buffers
    extraInfo["num_target_bufferViews"] = num_target_bufferViews
    extraInfo["num_target_accessors"] = num_target_accessors
    extraInfo["num_target_materials"] = num_target_materials

    offsets = {}
    offsets["meshes"] = num_target_meshes
    offsets["buffers"] = num_target_buffers
    offsets["bufferViews"] = num_target_bufferViews
    offsets["accessors"] = num_target_accessors
    offsets["materials"] = num_target_materials

    for i in range(num_srcs):
        source = sources[i]

        prefix = g_prefix + "{}/".format(source_filenames[i])
        concat(source, target, offsets, prefix, extraInfo)

    extraInfo["num_total_meshes"] = offsets["meshes"]
    extraInfo["num_total_buffers"] = offsets["buffers"]
    extraInfo["num_total_bufferViews"] = offsets["bufferViews"]
    extraInfo["num_total_accessors"] = offsets["accessors"]
    extraInfo["num_total_materials"] = offsets["materials"]

    target["asset"]["extras"] = extraInfo

    with open(output_filename, "w") as f:
        f.write(json.dumps(target, indent=2))

    print("Merged glTF was exported to : ", output_filename)
main()
