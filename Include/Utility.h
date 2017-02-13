#include <glm/glm.hpp>

namespace MATH {
#define PI 3.1415926535897932384626433832795
	inline glm::vec3 CosineWeightedHemisphereDirection(glm::vec3 normal) {
		//Algorithm: https://pathtracing.wordpress.com/2011/03/03/cosine-weighted-hemisphere/
		float x1 = (float)rand()/(float)RAND_MAX;
		float x2 = (float)rand() / (float)RAND_MAX;

		float theta = acos(sqrt(1.0-x1));
		float phi = 2.0 * PI * x2;

		float xs = sinf(theta) * cosf(phi);
		float ys = cosf(theta);
		float zs = sinf(theta) * sinf(phi);

		glm::vec3 y(normal.x, normal.y, normal.z);
		glm::vec3 h(y);

		if (fabs(h.x) <= fabs(h.y) && fabs(h.y) <= fabs(h.z))
			h.x = 1.0;
		else if (fabs(h.y) <= fabs(h.x) && fabs(h.y) <= fabs(h.z))
			h.y = 1.0;
		else h.z = 1.0;

		glm::vec3 x = glm::normalize(glm::cross(h, y));
		glm::vec3 z = glm::normalize(glm::cross(x, y));

		glm::vec3 direction = xs * x + ys * y + zs * z;
		return glm::normalize(direction);
	}
}