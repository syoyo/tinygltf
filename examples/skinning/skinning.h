#ifndef EXAMPLE_SKINNING_H_
#define EXAMPLE_SKINNING_H_

#include <vector>
#include <cstddef>
#include <cstdint>
#include <string>
#include <limits>

namespace example {

struct mat4 {
  float m[4][4];
};

struct vec4 {
  float f[4];
};

// glTF node
struct Node;


struct AnimationChannel {
  enum PathType { TRANSLATION, ROTATION, SCALE };
  PathType path;
  Node *node;
  uint32_t samplerIndex;
};

struct AnimationSampler {
  enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
  InterpolationType interpolation;
  std::vector<float> inputs;
  std::vector<example::vec4> outputsVec4;
};

struct Animation {
  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
  float start = std::numeric_limits<float>::max();
  float end = std::numeric_limits<float>::min();
};

///
/// Utility function to build transformation matrix from translate/rotation/scale
///
/// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md
///
/// M = T * R * S
///
void BuildTransofrmMatrix(
  const float translate[3],
  const float rotation[4], // as quaternion in glTF
  const float scale[3],
  mat4 *transform_matrix);
  

///
/// Compute joint matrices.
///
/// jointMatrix(j) =
///   globalTransformOfNodeThatTheMeshIsAttachedTo^-1 *
///   globalTransformOfJointNode(j) *
///   inverseBindMatrixForJoint(j);
///

void ComputeJointMatrices(
  const std::vector<mat4> global_transform_of_nodes,
  const std::vector<mat4> global_transform_of_joint_nodes,
  const std::vector<mat4> inverse_bind_matrix_for_joints,
  std::vector<mat4> output_joint_matrices);


///
///
/// @param[in] vertices Input vertices(# of elements = num_vertices * 4(xyzw))
/// @param[in] weights Linearized weights(# of elements = num_vertices * num_skinning_weights)
/// @param[in] weights Linearized weights(# of elements = num_vertices * num_skinning_weights)
/// @param[in] num_weights Linearized weights(# of elements = num_vertices * 
/// @param[in] joint_matrices Array of joint matricies.
/// @param[in] t Interpolator. [0.0, 1.0]
/// @param[in] skinned_vertices Resulting skinned vertices
///
void Skining(const std::vector<float> vertices,
             const std::vector<float> weights, const std::vector<size_t> joints,
             const size_t num_skinning_weights,
             const std::vector<mat4> joint_matrices,
             const float t,
             std::vector<float> *skinned_vertices);

}  // namespace example

#endif  // EXAMPLE_SKINNING_H_
