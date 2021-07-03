// An example of how to generate a gltf file from scratch. This example
// was translated from the pygltlib documentation in the pypi project page,
// which in turn is based on the Khronos Sample Models at:
//
// https://github.com/KhronosGroup/glTF-Sample-Models
//
// This example is released under the MIT license. 
//
// 2021-02-25 Thu
// Dov Grobgeld <dov.grobgeld@gmail.com>


// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

int main(int argc, char **argv)
{
  // Create a model with a single mesh and save it as a gltf file
  tinygltf::Model m;
  tinygltf::Scene scene;
  tinygltf::Mesh mesh;
  tinygltf::Primitive primitive;
  tinygltf::Node node;
  tinygltf::Buffer buffer;
  tinygltf::BufferView bufferView1;
  tinygltf::BufferView bufferView2;
  tinygltf::Accessor accessor1;
  tinygltf::Accessor accessor2;
  tinygltf::Asset asset;

  // This is the raw data buffer. 
  buffer.data = {
    // 6 bytes of indices and two bytes of padding
    0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,
    // 36 bytes of floating point numbers
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x00,0x00};

  // "The indices of the vertices (ELEMENT_ARRAY_BUFFER) take up 6 bytes in the
  // start of the buffer.
  bufferView1.buffer = 0;
  bufferView1.byteOffset=0;
  bufferView1.byteLength=6;
  bufferView1.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

  // The vertices take up 36 bytes (3 vertices * 3 floating points * 4 bytes)
  // at position 8 in the buffer and are of type ARRAY_BUFFER
  bufferView2.buffer = 0;
  bufferView2.byteOffset=8;
  bufferView2.byteLength=36;
  bufferView2.target = TINYGLTF_TARGET_ARRAY_BUFFER;

  // Describe the layout of bufferView1, the indices of the vertices
  accessor1.bufferView = 0;
  accessor1.byteOffset = 0;
  accessor1.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
  accessor1.count = 3;
  accessor1.type = TINYGLTF_TYPE_SCALAR;
  accessor1.maxValues.push_back(2);
  accessor1.minValues.push_back(0);

  // Describe the layout of bufferView2, the vertices themself
  accessor2.bufferView = 1;
  accessor2.byteOffset = 0;
  accessor2.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  accessor2.count = 3;
  accessor2.type = TINYGLTF_TYPE_VEC3;
  accessor2.maxValues = {1.0, 1.0, 0.0};
  accessor2.minValues = {0.0, 0.0, 0.0};

  // Build the mesh primitive and add it to the mesh
  primitive.indices = 0;                 // The index of the accessor for the vertex indices
  primitive.attributes["POSITION"] = 1;  // The index of the accessor for positions
  primitive.material = 0;
  primitive.mode = TINYGLTF_MODE_TRIANGLES;
  mesh.primitives.push_back(primitive);

  // Other tie ups
  node.mesh = 0;
  scene.nodes.push_back(0); // Default scene

  // Define the asset. The version is required
  asset.version = "2.0";
  asset.generator = "tinygltf";

  // Now all that remains is to tie back all the loose objects into the
  // our single model.
  m.scenes.push_back(scene);
  m.meshes.push_back(mesh);
  m.nodes.push_back(node);
  m.buffers.push_back(buffer);
  m.bufferViews.push_back(bufferView1);
  m.bufferViews.push_back(bufferView2);
  m.accessors.push_back(accessor1);
  m.accessors.push_back(accessor2);
  m.asset = asset;

  // Create a simple material
  tinygltf::Material mat;
  mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 0.9f, 0.9f, 1.0f};  
  mat.doubleSided = true;
  m.materials.push_back(mat);

  // Save it to a file
  tinygltf::TinyGLTF gltf;
  gltf.WriteGltfSceneToFile(&m, "triangle.gltf",
                           true, // embedImages
                           true, // embedBuffers
                           true, // pretty print
                           false); // write binary
  
  exit(0);
}
