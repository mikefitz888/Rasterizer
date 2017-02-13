#include "../Include/Raytracer.h"
#include "../Include/Triangle.h"

glm::vec3 Ray::getIntersection(Triangle triangle) {
	//Intersection = t, u, v
	// v0 + u*e1 + v*e2 = s + t*d
	glm::mat3 A(-direction, triangle.e1(), triangle.e2());
	glm::vec3 b = origin - triangle.v0;
	
	// Ax = b, we want to find x = (t, u, v)
	//  x = (A^-1)b

	return glm::inverse(A) * b;

	//Consider cramer's rule to get closed form solution to improve performance
	float detA = glm::determinant(A);
}

bool Ray::closestIntersection(const std::vector<Triangle>& triangles, Intersection& closestIntersection) {
	bool result = false;

	float min_dist = std::numeric_limits<float>::max();
	glm::vec3 position;
	int index;

	int i = -1;
	for (auto triangle : triangles) {
		i++;
		glm::mat3 A(-direction, triangle.e1(), triangle.e2());
		glm::vec3 b = origin - triangle.v0;

		float detA = glm::determinant(A); //Calculating determinant of A for use in Cramer's rule

		glm::mat3 A0 = A;
		A0[0] = b;
		float detA0 = glm::determinant(A0);
		float t = detA0 / detA;

		if (t >= 0) { //If t is negative then no intersection will occur
			//If this is a bottle-neck then maybe it's faster to: glm::inverse(A) * b;
			glm::mat3 A1 = A;
			A1[1] = b;
			float detA1 = glm::determinant(A1);
			float u = detA1 / detA;
			if (u < 0 || u > 1) continue;

			glm::mat3 A2 = A;
			A2[2] = b;
			float detA2 = glm::determinant(A2);
			float v = detA2 / detA;
			if (v < 0 || v > 1) continue;

			//Check if intersection with triangle actually occurs
			if (u + v <= 1) {
				glm::vec3 intersect = triangle.v0 + u*triangle.e1() + v*triangle.e2();
				float distance = glm::distance(origin, intersect);
				if (distance < min_dist) {
					result = true;
					min_dist = distance;
					position = intersect;
					index = i;
				}
			}
		}
	}
	if (result) {
		closestIntersection.distance = min_dist;
		closestIntersection.position = position;
		closestIntersection.index = index;
		closestIntersection.color = triangles[index].color;
	}
	return result;
}