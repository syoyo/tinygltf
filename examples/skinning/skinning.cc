#include "skinning.h"

#include "../common/matrix.h"
#include "../common/trackball.h"  // for quaternion

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <cassert>
#include <cstring>

namespace example {

struct Node {

  float translation[3] = {0.0f, 0.0f, 0.0f};
  float scale[4] = {1.0f, 1.0f, 1.0f};
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};


  void update() {
  }
};

static inline vec4 mix(vec4 x, vec4 y, float a) {
  vec4 v;
  v.f[0] = (1.0f - a) * x.f[0] + a * y.f[0];
  v.f[1] = (1.0f - a) * x.f[1] + a * y.f[1];
  v.f[2] = (1.0f - a) * x.f[2] + a * y.f[2];
  v.f[3] = (1.0f - a) * x.f[3] + a * y.f[3];

  return v;
}

void BuildTransofrmMatrix(const float translate[3],
                          const float rotation[4],  // as quaternion in glTF
                          const float scale[3], mat4 *transform_matrix) {
  float T[4][4];
  T[0][0] = 1.0f;
  T[0][1] = 0.0f;
  T[0][2] = 0.0f;
  T[0][3] = 0.0f;

  T[1][0] = 0.0f;
  T[1][1] = 1.0f;
  T[1][2] = 0.0f;
  T[1][3] = 0.0f;

  T[2][0] = 0.0f;
  T[2][1] = 0.0f;
  T[2][2] = 1.0f;
  T[2][3] = 0.0f;

  T[3][0] = translate[0];
  T[3][1] = translate[1];
  T[3][2] = translate[2];
  T[3][3] = 1.0f;

  float R[4][4];

  build_rotmatrix(R, rotation);

  float S[4][4];
  S[0][0] = scale[0];
  S[0][1] = 0.0f;
  S[0][2] = 0.0f;
  S[0][3] = 0.0f;

  S[1][0] = 0.0f;
  S[1][1] = scale[1];
  S[1][2] = 0.0f;
  S[1][3] = 0.0f;

  S[2][0] = 0.0f;
  S[2][1] = 0.0f;
  S[2][2] = scale[2];
  S[2][3] = 0.0f;

  S[3][0] = 0.0f;
  S[3][1] = 0.0f;
  S[3][2] = 0.0f;
  S[3][3] = 1.0f;

  float RS[4][4];

  Matrix::Mult(RS, R, S);

  Matrix::Mult(transform_matrix->m, T, RS);
}

void ComputeJointMatrices(
    const std::vector<mat4> global_transform_of_nodes,
    const std::vector<mat4> global_transform_of_joint_nodes,
    const std::vector<mat4> inverse_bind_matrix_for_joints,
    std::vector<mat4> *output_joint_matrices) {
  const size_t n = global_transform_of_nodes.size();

  output_joint_matrices->resize(n);

  for (size_t i = 0; i < n; i++) {
    mat4 g_inv = global_transform_of_nodes[i];
    Matrix::Inverse(g_inv.m);

    mat4 g_joint = global_transform_of_joint_nodes[i];
    mat4 inverse_bind_matrix = inverse_bind_matrix_for_joints[i];

    float a[4][4];  // temp matrix
    Matrix::Mult(a, g_joint.m, inverse_bind_matrix.m);

    Matrix::Mult((*output_joint_matrices)[i].m, g_inv.m, a);
  }
}

void Skining(const std::vector<float> vertices,
             const std::vector<float> weights, const std::vector<size_t> joints,
             const size_t num_skinning_weights,
             const std::vector<mat4> joint_matrices, const float t,
             std::vector<float> *skinned_vertices) {
  assert((vertices.size() % 4) == 0);
  const size_t num_vertices = vertices.size() / 4;

  skinned_vertices->resize(vertices.size());

  // TODO(syoyo): Ensure sum(weights) = 1.0;

  for (size_t v = 0; v < num_vertices; v++) {
    const float *w_p = weights.data() + v * num_skinning_weights;
    const size_t *j_p = joints.data() + v * num_skinning_weights;

    mat4 skin_mat;
    memset(skin_mat.m, 0, sizeof(float) * 4 * 4);

    for (size_t k = 0; k < num_skinning_weights; k++) {
      const float w = w_p[k];
      const mat4 &m = joint_matrices[j_p[k]];

      for (size_t j = 0; j < 4; j++) {
        for (size_t i = 0; i < 4; i++) {
          skin_mat.m[j][i] += w * m.m[j][i];
        }
      }
    }

    // M = lerp I and skin_mat
    mat4 M;

    mat4 I;
    Matrix::Identity(I.m);

    for (size_t j = 0; j < 4; j++) {
      for (size_t i = 0; i < 4; i++) {
        M.m[j][i] = I.m[j][i] * t + (1.0f - t) * skin_mat.m[j][i];
      }
    }

    float vtx[4];
    vtx[0] = vertices[4 * v + 0];
    vtx[1] = vertices[4 * v + 1];
    vtx[2] = vertices[4 * v + 2];
    vtx[3] = vertices[4 * v + 3];

    float ret[4];
    Matrix::MultV4(ret, M.m, vtx);
  }
}

void UpdateAnimation(std::vector<Animation> &animations, uint32_t index,
                     float time, std::vector<Node*> &nodes) {
  if (index > uint32_t(animations.size() - 1)) {
    return;
  }

  const Animation &animation = animations[index];

  bool updated = false;
  for (auto &channel : animation.channels) {
    const AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
    if (sampler.inputs.size() > sampler.outputsVec4.size()) {
      continue;
    }

    // TODO(LTE): support interpolation other than LINEAR
    for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
      if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
        float u = std::max(0.0f, time - sampler.inputs[i]) /
                  (sampler.inputs[i + 1] - sampler.inputs[i]);
        if (u <= 1.0f) {
          switch (channel.path) {
            case AnimationChannel::PathType::TRANSLATION: {
              example::vec4 trans =
                  mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
              channel.node->translation[0] = trans.f[0];
              channel.node->translation[1] = trans.f[1];
              channel.node->translation[2] = trans.f[2];
              // drop w
              break;
            }
            case AnimationChannel::PathType::SCALE: {
              example::vec4 scale =
                  mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
              channel.node->scale[0] = scale.f[0];
              channel.node->scale[1] = scale.f[1];
              channel.node->scale[2] = scale.f[2];
              break;
            }
            case AnimationChannel::PathType::ROTATION: {

              hmm_quaternion q1 = HMM_Quaternion(
               sampler.outputsVec4[i].f[0],
               sampler.outputsVec4[i].f[1],
               sampler.outputsVec4[i].f[2],
               sampler.outputsVec4[i].f[3]);

              hmm_quaternion q2 = HMM_Quaternion(
               sampler.outputsVec4[i + 1].f[0],
               sampler.outputsVec4[i + 1].f[1],
               sampler.outputsVec4[i + 1].f[2],
               sampler.outputsVec4[i + 1].f[3]);

              hmm_quaternion q = HMM_NormalizeQuaternion(HMM_Slerp(q1, u, q2));

              channel.node->rotation[0] = q.Elements[0];
              channel.node->rotation[1] = q.Elements[1];
              channel.node->rotation[2] = q.Elements[2];
              channel.node->rotation[3] = q.Elements[3];

              break;
            }
          }
          updated = true;
        }
      }
    }
  }

  if (updated) {
    for (auto &node : nodes) {
      node->update();
    }
  }
}

}  // namespace example
