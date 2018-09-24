#include "skinning.h"

#include "../common/matrix.h"
#include "../common/trackball.h" // for quaternion

#include <cstring>
#include <cassert>

namespace example {

void BuildTransofrmMatrix(
  const float translate[3],
  const float rotation[4], // as quaternion in glTF
  const float scale[3],
  mat4 *transform_matrix) {

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

    float a[4][4]; // temp matrix
    Matrix::Mult(a, g_joint.m, inverse_bind_matrix.m);

    Matrix::Mult((*output_joint_matrices)[i].m, g_inv.m, a);
  }
}


void Skining(const std::vector<float> vertices,
             const std::vector<float> weights, const std::vector<size_t> joints,
             const size_t num_skinning_weights,
             const std::vector<mat4> joint_matrices,
             const float t,
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


} // namespace example
