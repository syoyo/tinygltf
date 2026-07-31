#ifndef EXAMPLE_RENDER_H_
#define EXAMPLE_RENDER_H_

#include <atomic>  // C++11

//mode definitions now here 

#define SHOW_BUFFER_COLOR (0)
#define SHOW_BUFFER_NORMAL (1)
#define SHOW_BUFFER_POSITION (2)
#define SHOW_BUFFER_DEPTH (3)
#define SHOW_BUFFER_TEXCOORD (4)
#define SHOW_BUFFER_VARYCOORD (5)

#include "render-config.h"
#include "nanosg.h"
#include "mesh.h"
#include "material.h"

namespace example {

struct Asset {
  std::vector<Mesh<float> > meshes;
  std::vector<Material> materials;
  
  //tigra: add default material
  Material default_material;
  std::vector<Texture> textures;
};

class Renderer {
 public:
  Renderer() {}
  ~Renderer() {}

  /// Returns false when the rendering was canceled.
  static bool Render(float* rgba, float* aux_rgba, int *sample_counts, float quat[4],
              const nanosg::Scene<float, Mesh<float>> &scene, const Asset &asset, const RenderConfig& config,
                     std::atomic<bool>& cancel_flag,
                     int& _showBufferMode
                    );
};
};

#endif  // EXAMPLE_RENDER_H_
