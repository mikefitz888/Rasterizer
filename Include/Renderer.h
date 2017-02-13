#include <glm/glm.hpp>
#include <vector>
#include "PhotonMap.h"
#include "Triangle.h"
#include "Utility.h"

namespace RENDER {
    glm::vec3 getRadianceEstimate(glm::vec3 point, std::vector<Triangle>& triangles, photonmap::PhotonMapper& map, PhotonInfoWrapper photons);
    glm::vec3 finalGather(glm::vec3 point, std::vector<Triangle>& triangles, photonmap::PhotonMapper& map, PhotonInfoWrapper photons);
}
