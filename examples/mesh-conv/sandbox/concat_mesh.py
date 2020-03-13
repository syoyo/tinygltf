# concat mesh to glTF
# support multiple source glTF to me concatenated to target.gltf

# TODO: skins, nodes, scenes

import json
import sys, os

g_prefix = "added/"

def adjustMaterialTextureIndex(mat, texture_index_offset):
    # Args: mat(dict)
    # texture_index_offset(int) Index offset

    if "normalTexture" in mat:
        if "index" in mat["normalTexture"]:
            mat["normalTexture"]["index"] += texture_index_offset

    if "occlusionTexture" in mat:
        if "index" in mat["occlusionTexture"]:
            mat["occlusionTexture"]["index"] += texture_index_offset

    if "emissiveTexture" in mat:
        if "index" in mat["emissiveTexture"]:
            mat["emissiveTexture"]["index"] += texture_index_offset

    if "pbrMetallicRoughness" in mat:
        if "baseColorTexture" in mat["pbrMetallicRoughness"]:
            if "index" in mat["pbrMetallicRoughness"]["baseColorTexture"]:
                mat["pbrMetallicRoughness"]["baseColorTexture"]["index"] += texture_index_offset

        if "metallicRoughnessTexture" in mat["pbrMetallicRoughness"]:
            if "index" in mat["pbrMetallicRoughness"]["metallicRoughnessTexture"]:
                mat["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"] += texture_index_offset

    return mat


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
    num_source_samplers = len(source["samplers"]) if "samplers" in source else 0
    num_source_images = len(source["images"]) if "images" in source else 0
    num_source_textures = len(source["textures"]) if "textures" in source else 0
    print("src =====")
    print("num_src_meshes: ", num_source_meshes)
    print("num_src_buffers: ", num_source_buffers)
    print("num_src_bufferViews: ", num_source_bufferViews)
    print("num_src_accessors: ", num_source_accessors)
    print("num_src_materials: ", num_source_materials)
    print("num_src_textures: ", num_source_textures)
    print("num_src_images: ", num_source_images)
    print("num_src_samplers: ", num_source_samplers)


    #
    # Modify name and adjust index offset
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


    if "materials" in source:
        for i in range(len(source["materials"])):
            if "name" in source["materials"][i]:
                source["materials"][i]["name"] = prefix + source["materials"][i]["name"]
            source["materials"][i] = adjustMaterialTextureIndex(source["materials"][i], offsets["textures"])

    if "images" in source:
        for i in range(len(source["images"])):
            if "name" in source["images"][i]:
                source["images"][i]["name"] = prefix + source["images"][i]["name"]

    if "samplers" in source:
        for i in range(len(source["samplers"])):
            if "name" in source["samplers"][i]:
                source["samplers"][i]["name"] = prefix + source["samplers"][i]["name"]

    if "textures" in source:
        for i in range(len(source["textures"])):
            if "name" in source["textures"][i]:
                source["textures"][i]["name"] = prefix + source["textures"][i]["name"]
            source["textures"][i]["sampler"] += offsets["samplers"]
            source["textures"][i]["source"] += offsets["images"]

    #
    # Append mesh info
    #
    target["buffers"] += source["buffers"]
    target["bufferViews"] += source["bufferViews"]
    target["meshes"] += source["meshes"]
    target["accessors"] += source["accessors"]
    if "materials" in source:
        if "materials" in target:
            target["materials"] += source["materials"]
        else:
            target["materials"] = source["materials"]
    if "images" in source:
        if "images" in target:
            target["images"] += source["images"]
        else:
            target["images"] = source["images"]
    if "textures" in source:
        if "textures" in target:
            target["textures"] += source["textures"]
        else:
            target["textures"] = source["textures"]
    if "samplers" in source:
        if "samplers" in target:
            target["samplers"] += source["samplers"]
        else:
            target["samplers"] = source["samplers"]

    # assume `prefix` is unique
    extraInfo["num_{}_meshes".format(prefix)] = num_source_meshes
    extraInfo["num_{}_buffers".format(prefix)] = num_source_buffers
    extraInfo["num_{}_bufferViews".format(prefix)] = num_source_bufferViews
    extraInfo["num_{}_accessors".format(prefix)] = num_source_accessors
    extraInfo["num_{}_materials".format(prefix)] = num_source_materials
    extraInfo["num_{}_images".format(prefix)] = num_source_images
    extraInfo["num_{}_samplers".format(prefix)] = num_source_samplers
    extraInfo["num_{}_textures".format(prefix)] = num_source_textures

    # update offsets
    offsets["meshes"] += num_source_meshes
    offsets["buffers"] += num_source_buffers
    offsets["bufferViews"] += num_source_bufferViews
    offsets["accessors"] += num_source_accessors
    offsets["materials"] += num_source_materials
    offsets["textures"] += num_source_textures
    offsets["images"] += num_source_images
    offsets["samplers"] += num_source_samplers


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

    num_target_images = 0
    if "images" in target:
        num_target_images = len(target["images"])

    num_target_samplers = 0
    if "samplers" in target:
        num_target_samplers = len(target["samplers"])

    num_target_textures = 0
    if "textures" in target:
        num_target_textures = len(target["textures"])

    print("num_target_meshes: ", num_target_meshes)
    print("num_target_buffers: ", num_target_buffers)
    print("num_target_bufferViews: ", num_target_bufferViews)
    print("num_target_accessors: ", num_target_accessors)
    print("num_target_materials: ", num_target_materials)
    print("num_target_textures: ", num_target_textures)
    print("num_target_images: ", num_target_images)
    print("num_target_samplers: ", num_target_samplers)

    #
    # add some info to asset.extras
    #
    extraInfo = {}
    extraInfo["num_target_meshes"] = num_target_meshes
    extraInfo["num_target_buffers"] = num_target_buffers
    extraInfo["num_target_bufferViews"] = num_target_bufferViews
    extraInfo["num_target_accessors"] = num_target_accessors
    extraInfo["num_target_materials"] = num_target_materials
    extraInfo["num_target_textures"] = num_target_textures
    extraInfo["num_target_images"] = num_target_images
    extraInfo["num_target_samplers"] = num_target_samplers

    offsets = {}
    offsets["meshes"] = num_target_meshes
    offsets["buffers"] = num_target_buffers
    offsets["bufferViews"] = num_target_bufferViews
    offsets["accessors"] = num_target_accessors
    offsets["materials"] = num_target_materials
    offsets["textures"] = num_target_textures
    offsets["samplers"] = num_target_samplers
    offsets["images"] = num_target_images

    for i in range(num_srcs):
        source = sources[i]

        prefix = g_prefix + "{}/".format(source_filenames[i])
        concat(source, target, offsets, prefix, extraInfo)

    extraInfo["num_total_meshes"] = offsets["meshes"]
    extraInfo["num_total_buffers"] = offsets["buffers"]
    extraInfo["num_total_bufferViews"] = offsets["bufferViews"]
    extraInfo["num_total_accessors"] = offsets["accessors"]
    extraInfo["num_total_materials"] = offsets["materials"]
    extraInfo["num_total_textures"] = offsets["textures"]
    extraInfo["num_total_images"] = offsets["images"]
    extraInfo["num_total_samplers"] = offsets["samplers"]

    target["asset"]["extras"] = extraInfo

    with open(output_filename, "w") as f:
        f.write(json.dumps(target, indent=2))

    print("Merged glTF was exported to : ", output_filename)
main()
