#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>
#include "Triangle.h"
#include "ModelLoader.h"

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel(std::vector<Triangle>& triangles) {
	using glm::vec3;
	using glm::vec2;

	// Defines colors:
	vec3 red(0.75f, 0.15f, 0.15f);
	vec3 yellow(0.75f, 0.75f, 0.15f);
	vec3 green(0.15f, 0.75f, 0.15f);
	vec3 cyan(0.15f, 0.75f, 0.75f);
	vec3 blue(0.15f, 0.15f, 0.75f);
	vec3 purple(0.75f, 0.15f, 0.75f);
	vec3 white(0.75f, 0.75f, 0.75f);

	triangles.clear();
	triangles.reserve(5 * 2 * 3);

	// ---------------------------------------------------------------------------
	// Room

	float L = 555;			// Length of Cornell Box side.


	vec2 uv1_0(1.0, 0.0);
	vec2 uv0_0(0.0, 0.0);
	vec2 uv1_1(1.0, 1.0);
	vec2 uv0_1(0.0, 1.0);

	vec2 uvA(0.0, 0.0); vec2 uvB(0.0, 0.0); vec2 uvC(0.0, 0.0); vec2 uvD(0.0, 0.0); vec2 uvE(0.0, 0.0);
	vec2 uvF(0.0, 0.0); vec2 uvG(0.0, 0.0); vec2 uvH(0.0, 0.0);

	vec3 A(L, 0, 0);
	vec3 B(0, 0, 0);
	vec3 C(L, 0, L);
	vec3 D(0, 0, L);

	vec3 E(L, L, 0);
	vec3 F(0, L, 0);
	vec3 G(L, L, L);
	vec3 H(0, L, L);

	//Front Wall
	triangles.push_back(Triangle(E, B, A, white, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(E, F, B, white, uv0_0, uv1_1, uv0_1));

	//Floor
	triangles.push_back(Triangle(C, B, A, green, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(C, D, B, green, uv0_0, uv1_0, uv1_1));

	// Left wall
	triangles.push_back(Triangle(C, A, E, purple, uv1_1, uv0_1, uv0_0));
	triangles.push_back(Triangle(C, E, G, purple, uv1_1, uv0_0, uv1_0));

	// Right wall
	triangles.push_back(Triangle(F, B, D, yellow, uv1_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(H, F, D, yellow, uv0_0, uv1_0, uv0_1));

	// Ceiling
	triangles.push_back(Triangle(E, F, G, cyan, uv0_0, uv1_0, uv0_1));
	triangles.push_back(Triangle(F, H, G, cyan, uv1_0, uv1_1, uv0_1));

	// Back wall
	triangles.push_back(Triangle(G, D, C, white, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(G, H, D, white, uv0_0, uv1_0, uv1_1));

	

	// ---------------------------------------------------------------------------
	// Short block

	A = vec3(290, 0, 114);
	B = vec3(130, 0, 65);
	C = vec3(240, 0, 272);
	D = vec3(82, 0, 225);

	E = vec3(290, 165, 114);
	F = vec3(130, 165, 65);
	G = vec3(240, 165, 272);
	H = vec3(82, 165, 225);
	

	// Front
	triangles.push_back(Triangle(E, B, A, red, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(E, F, B, red, uv0_0, uv1_0, uv1_1));

	// Front
	triangles.push_back(Triangle(F, D, B, red, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(F, H, D, red, uv0_0, uv1_0, uv1_1));

	// BACK
	triangles.push_back(Triangle(H, C, D, red, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(H, G, C, red, uv0_0, uv1_0, uv1_1));

	// LEFT
	triangles.push_back(Triangle(G, E, C, red, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(E, A, C, red, uv0_0, uv1_0, uv1_1));

	// TOP
	triangles.push_back(Triangle(G, F, E, red, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(G, H, F, red, uv0_0, uv1_0, uv1_1));

	// ---------------------------------------------------------------------------
	// Tall block

	A = vec3(423, 0, 247);
	B = vec3(265, 0, 296);
	C = vec3(472, 0, 406);
	D = vec3(314, 0, 456);

	E = vec3(423, 330, 247);
	F = vec3(265, 330, 296);
	G = vec3(472, 330, 406);
	H = vec3(314, 330, 456);


	// Front
	triangles.push_back(Triangle(E, B, A, blue, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(E, F, B, blue, uv0_0, uv1_0, uv1_1));

	// Front
	triangles.push_back(Triangle(F, D, B, blue, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(F, H, D, blue, uv0_0, uv1_0, uv1_1));

	// BACK
	triangles.push_back(Triangle(H, C, D, blue, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(H, G, C, blue, uv0_0, uv1_0, uv1_1));

	// LEFT
	triangles.push_back(Triangle(G, E, C, blue, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(E, A, C, blue, uv0_0, uv1_0, uv1_1));

	// TOP
	triangles.push_back(Triangle(G, F, E, blue, uv0_0, uv1_1, uv0_1));
	triangles.push_back(Triangle(G, H, F, blue, uv0_0, uv1_0, uv1_1));


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for (size_t i = 0; i<triangles.size(); ++i) {
		triangles[i].v0 *= 2 / L;
		triangles[i].v1 *= 2 / L;
		triangles[i].v2 *= 2 / L;

		triangles[i].v0 -= vec3(1, 1, 1);
		triangles[i].v1 -= vec3(1, 1, 1);
		triangles[i].v2 -= vec3(1, 1, 1);

		triangles[i].v0.x *= -1;
		triangles[i].v1.x *= -1;
		triangles[i].v2.x *= -1;

		triangles[i].v0.y *= -1;
		triangles[i].v1.y *= -1;
		triangles[i].v2.y *= -1;

		triangles[i].ComputeNormal();
	}
}

#endif
