#ifndef EXAMPLE_MATRIX_H_
#define EXAMPLE_MATRIX_H_

class Matrix {
public:
  Matrix();
  ~Matrix();

  static void Print(float m[4][4]);
  static void LookAt(float m[4][4], float eye[3], float lookat[3],
                     float up[3]);
  static void Inverse(float m[4][4]);
  static void Identity(float m[4][4]);
  static void Add(float dst[4][4], const float m0[4][4], const float m1[4][4]);
  static void Mult(float dst[4][4], const float m0[4][4], const float m1[4][4]);
  static void MultV(float dst[3], const float m[4][4], const float v[3]);
  static void MultV4(float dst[4], const float m[4][4], const float v[4]);
};

#endif  // 
