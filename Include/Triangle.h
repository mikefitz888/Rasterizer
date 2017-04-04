#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

//These are set and are important for RenderUtil, they can be changed as long as they are different and both multiple of coarse_render_length in RenderUtil.cpp
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 480;

// Used to describe a triangular surface:
class Triangle {
public:
	//Vertices
	glm::vec3 v0, v1, v2;

	//Texture Coordinates
	glm::vec2 uv0, uv1, uv2;

	//Normals
	glm::vec3 n0, n1, n2;

	//Optional Parameters

	glm::vec3 normal;
	glm::vec3 color;
    float transparency = 1.0;

    inline Triangle() {};

	inline Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
					glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
					glm::vec3 n0, glm::vec3 n1, glm::vec3 n2) :
					v0(v0), v1(v1), v2(v2), uv0(uv0), uv1(uv1), uv2(uv2), n0(n0), n1(n1), n2(n2) {
		ComputeNormal();
	}

	Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 color, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2)
		: v0(v0), v1(v1), v2(v2), color(color), uv0(uv0), uv1(uv1), uv2(uv2) {
		ComputeNormal();
	}

	inline glm::vec3 e1() { return v1 - v0; }
	inline glm::vec3 e2() { return v2 - v0; }
    inline float maxY() const { return std::max(v0.y, std::max(v1.y, v2.y)); }

	inline bool checkIntersection(glm::vec3 intersect) {
		return (0 < intersect.y) && (0 < intersect.z) && (intersect.y + intersect.z < 1);
	}

	inline glm::vec3 getNormal() {
		return normal;
	}

    inline glm::vec3 getInterpolatedNormal(float u, float v){
         // compute "smooth" normal using Gouraud's technique (interpolate vertex normals)
    }

	inline void ComputeNormal() {
		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;
		/*
		**  Points in triangle's plane: r = v0 + u*e1 + v*e2
		**	Points in triangle: 0 < u, 0 < v, u+v < 1
		*/
		normal = glm::normalize(glm::cross(e2, e1));
	}

	/*
		Fills the internal triangle data with the barycentric coordinates for a given point of intersection.
		These can then be used for sampling interpolated uv coordinates/colours at any point on the triangle.

		RETURNS:
			- Vec3 where each factor corresponds to the contribution from each vertex of the triangle (0,1,2).

		Source:
			http://answers.unity3d.com/questions/383804/calculate-uv-coordinates-of-3d-point-on-plane-of-m.html
	*/
	inline glm::vec3 calculateBarycentricCoordinates(glm::vec3 intersection_point) {
		glm::vec3 factorA = v0 - intersection_point;
		glm::vec3 factorB = v1 - intersection_point;
		glm::vec3 factorC = v2 - intersection_point;

		float area = glm::length( glm::cross(v0 - v1, v0 - v2));
		glm::vec3 barycentric_coordinates;
		barycentric_coordinates.x = glm::length(glm::cross(factorB, factorC)) / area;
		barycentric_coordinates.y = glm::length(glm::cross(factorC, factorA)) / area;
		barycentric_coordinates.z = glm::length(glm::cross(factorA, factorB)) / area;
		return barycentric_coordinates;
	}
};

struct Intersection {
	glm::vec3 position;
	glm::vec3 color;
	float distance;
	int index;
	Intersection(glm::vec3 pos, float dis, int ind) : position(pos), distance(dis), index(ind) {}
	Intersection(){}
};

#endif // !TRIANGLE_H
