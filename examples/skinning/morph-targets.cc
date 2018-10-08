#include <vector>
#include <cstdlib>
#include <cassert>

void MorthTargets(std::vector<float> &weights,
  std::vector<std::vector<float>> &targets,
  std::vector<float> *output)
{
  assert(weights.size() > 0);
  assert(targets.size() > 0);
  assert(weights.size() == targets.size());

  // Assume all position has same number of vertices;

  // TODO(parallelize)
  for (size_t v = 0; v < targets[0].size(); v++) { // for each vertex
    (*output)[v] = 0.0f;  
    for (size_t i = 0; i < weights.size(); i++) {
      (*output)[v] += weights[i] * targets[i][v];
    }
  }
}
