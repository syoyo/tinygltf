/*
The MIT License (MIT)

Copyright (c) 2016 Light Transport Entertainment, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Simple Cyhair loader.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <iostream>

#include "cyhair_loader.h"

namespace example {

class real3 {
 public:
  real3() : x(0.0f), y(0.0f), z(0.0f) {}
  real3(float v) : x(v), y(v), z(v) {}
  real3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
  //~real3() {}

  real3 operator+(const real3 &f2) const {
    return real3(x + f2.x, y + f2.y, z + f2.z);
  }
  real3 operator*(const real3 &f2) const {
    return real3(x * f2.x, y * f2.y, z * f2.z);
  }
  real3 operator/(const real3 &f2) const {
    return real3(x / f2.x, y / f2.y, z / f2.z);
  }
  real3 operator/(const float f) const { return real3(x / f, y / f, z / f); }

  float x, y, z;
};

inline real3 operator*(float f, const real3 &v) {
  return real3(v.x * f, v.y * f, v.z * f);
}

static const float toC2B[4][4] = {
    {0.0f, 6.0f / 6.0f, 0.0f, 0.0f},
    {-1.0f / 6.0f, 6.0f / 6.0f, 1.0f / 6.0f, 0.0f},
    {0.0f, 1.0f / 6.0f, 6.0f / 6.0f, -1.0f / 6.0f},
    {0.0f, 0.0, 6.0f / 6.0f, 0.0f}};

static const float toC2B0[4][4] = {
    {0.0f, 6.0f / 6.0f, 0.0f, 0.0f},
    {0.0f, 3.0f / 6.0f, 4.0f / 6.0f, -1.0f / 6.0f},
    {0.0f, 1.0f / 6.0f, 6.0f / 6.0f, -1.0f / 6.0f},
    {0.0f, 0.0f, 6.0f / 6.0f, 0.0f}};

static const float toC2B1[4][4] = {
    {0.0f, 6.0f / 6.0f, 0.0f, 0.0f},
    {-1.0f / 6.0f, 6.0f / 6.0f, 1.0f / 6.0f, 0.0f},
    {-1.0f / 6.0f, 4.0f / 6.0f, 3.0f / 6.0f, 0.0f},
    {0.0f, 0.0f, 6.0f / 6.0f, 0.0f}};

static void mul_matrix(real3 out[4], const float mat[4][4], const real3 pt[4]) {
  for (int i = 0; i < 4; i++) {
    out[i] = mat[i][0] * pt[0] + mat[i][1] * pt[1] + mat[i][2] * pt[2] +
             mat[i][3] * pt[3];
  }
}

static void CamullRomToCubicBezier(real3 Q[4], const real3 *cps, int cps_size,
                                   int seg_idx) {
  size_t sz = static_cast<size_t>(cps_size);
  if (sz == 2) {
    Q[0] = cps[seg_idx];
    Q[1] = cps[seg_idx] * 2.0f / 3.0f + cps[seg_idx + 1] * 1.0f / 3.0f;
    Q[2] = cps[seg_idx] * 1.0f / 3.0f + cps[seg_idx + 1] * 2.0f / 3.0f;
    Q[3] = cps[seg_idx + 1];
  } else {
    real3 P[4];
    if (seg_idx == 0) {
      P[0] = real3(0.0f);
      P[1] = cps[seg_idx + 0];
      P[2] = cps[seg_idx + 1];
      P[3] = cps[seg_idx + 2];
      mul_matrix(Q, toC2B0, P);
    } else if (seg_idx == static_cast<int>(sz - 2)) {
      P[0] = cps[seg_idx - 1];
      P[1] = cps[seg_idx + 0];
      P[2] = cps[seg_idx + 1];
      P[3] = real3(0.0f);
      mul_matrix(Q, toC2B1, P);
    } else {
      P[0] = cps[seg_idx - 1];
      P[1] = cps[seg_idx + 0];
      P[2] = cps[seg_idx + 1];
      P[3] = cps[seg_idx + 2];
      mul_matrix(Q, toC2B, P);
    }
  }
}

bool CyHair::Load(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  assert(sizeof(CyHairHeader) == 128);
  CyHairHeader header;

  if (1 != fread(&header, 128, 1, fp)) {
    fclose(fp);
    return false;
  }
  if (memcmp(header.magic, "HAIR", 4) != 0) {
    fclose(fp);
    return false;
  }

  flags_ = header.flags;
  default_thickness_ = header.default_thickness;
  default_transparency_ = header.default_transparency;
  default_segments_ = static_cast<int>(header.default_segments);
  default_color_[0] = header.default_color[0];
  default_color_[1] = header.default_color[1];
  default_color_[2] = header.default_color[2];

  const bool has_segments = flags_ & 0x1;
  const bool has_points = flags_ & 0x2;
  const bool has_thickness = flags_ & 0x4;
  const bool has_transparency = flags_ & 0x8;
  const bool has_color = flags_ & 0x10;

  num_strands_ = header.num_strands;
  total_points_ = header.total_points;

  if (!has_points) {
    std::cout << "No point data in CyHair." << std::endl;
    return false;
  }

  if ((default_segments_ < 1) && (!has_segments)) {
    std::cout << "No valid segment information in CyHair." << std::endl;
    return false;
  }

  // First read all strand data from a file.
  if (has_segments) {
    segments_.resize(num_strands_);
    if (1 !=
        fread(&segments_[0], sizeof(unsigned short) * num_strands_, 1, fp)) {
      std::cout << "Failed to read CyHair segments data." << std::endl;
      fclose(fp);
      return false;
    }
  }

  if (has_points) {
    points_.resize(3 * total_points_);
    size_t n = fread(&points_[0], total_points_ * sizeof(float) * 3, 1, fp);
    if (1 != n) {
      std::cout << "Failed to read CyHair points data." << std::endl;
      fclose(fp);
      return false;
    }
  }
  if (has_thickness) {
    thicknesses_.resize(total_points_);
    if (1 != fread(&thicknesses_[0], total_points_ * sizeof(float), 1, fp)) {
      std::cout << "Failed to read CyHair thickness data." << std::endl;
      fclose(fp);
      return false;
    }
  }

  if (has_transparency) {
    transparencies_.resize(total_points_);
    if (1 != fread(&transparencies_[0], total_points_ * sizeof(float), 1, fp)) {
      std::cout << "Failed to read CyHair transparencies data." << std::endl;
      fclose(fp);
      return false;
    }
  }

  if (has_color) {
    colors_.resize(3 * total_points_);
    if (1 != fread(&colors_[0], total_points_ * sizeof(float) * 3, 1, fp)) {
      std::cout << "Failed to read CyHair colors data." << std::endl;
      fclose(fp);
      return false;
    }
  }

  // Build strand offset table.
  strand_offsets_.resize(num_strands_);
  strand_offsets_[0] = 0;
  for (size_t i = 1; i < num_strands_; i++) {
    int num_segments = segments_.empty() ? default_segments_ : segments_[i - 1];
    strand_offsets_[i] =
        strand_offsets_[i - 1] + static_cast<unsigned int>(num_segments + 1);
  }

  return true;
}

bool CyHair::ToCubicBezierCurves(std::vector<float> *vertices,
                                 std::vector<float> *radiuss,
                                 const float vertex_scale[3],
                                 const float vertex_translate[3],
                                 const int max_strands, const float user_thickness) {
  if (points_.empty() || strand_offsets_.empty()) {
    return false;
  }

  vertices->clear();
  radiuss->clear();

  int num_strands = static_cast<int>(num_strands_);

  if ((max_strands > 0) && (max_strands < num_strands)) {
    num_strands = max_strands;
  }

  std::cout << "[Hair] Convert first " << num_strands << " strands from "
            << max_strands << " strands in the original hair data."
            << std::endl;

  // Assume input points are CatmullRom spline.
  for (size_t i = 0; i < static_cast<size_t>(num_strands); i++) {
    if ((i % 1000) == 0) {
      std::cout << i << " / " << num_strands_ << std::endl;
    }

    int num_segments = segments_.empty() ? default_segments_ : segments_[i];
    if (num_segments < 2) {
      continue;
    }

    std::vector<real3> segment_points;
    for (size_t k = 0; k < static_cast<size_t>(num_segments); k++) {
      // Zup -> Yup
      real3 p(points_[3 * (strand_offsets_[i] + k) + 0],
              points_[3 * (strand_offsets_[i] + k) + 2],
              points_[3 * (strand_offsets_[i] + k) + 1]);
      segment_points.push_back(p);
    }

    // Skip both endpoints
    for (int s = 1; s < num_segments - 1; s++) {
      int seg_idx = s - 1;
      real3 q[4];
      CamullRomToCubicBezier(q, segment_points.data(), num_segments, seg_idx);

      vertices->push_back(vertex_scale[0] * q[0].x + vertex_translate[0]);
      vertices->push_back(vertex_scale[1] * q[0].y + vertex_translate[1]);
      vertices->push_back(vertex_scale[2] * q[0].z + vertex_translate[2]);
      vertices->push_back(vertex_scale[0] * q[1].x + vertex_translate[0]);
      vertices->push_back(vertex_scale[1] * q[1].y + vertex_translate[1]);
      vertices->push_back(vertex_scale[2] * q[1].z + vertex_translate[2]);
      vertices->push_back(vertex_scale[0] * q[2].x + vertex_translate[0]);
      vertices->push_back(vertex_scale[1] * q[2].y + vertex_translate[1]);
      vertices->push_back(vertex_scale[2] * q[2].z + vertex_translate[2]);
      vertices->push_back(vertex_scale[0] * q[3].x + vertex_translate[0]);
      vertices->push_back(vertex_scale[1] * q[3].y + vertex_translate[1]);
      vertices->push_back(vertex_scale[2] * q[3].z + vertex_translate[2]);

      if (user_thickness > 0) {
        // Use user supplied thickness.
        radiuss->push_back(user_thickness);
        radiuss->push_back(user_thickness);
        radiuss->push_back(user_thickness);
        radiuss->push_back(user_thickness);
      } else {
        // TODO(syoyo) Support per point/segment thickness
        radiuss->push_back(default_thickness_);
        radiuss->push_back(default_thickness_);
        radiuss->push_back(default_thickness_);
        radiuss->push_back(default_thickness_);
      }
    }
  }

  return true;
}

}  // namespace example
