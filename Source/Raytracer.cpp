/*#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#undef size_t
#include <SDL.h>
#undef main
#include "../Include/SDLauxiliary.h"

#include "../Include/TestModel.h"
#include "../Include/Raytracer.h"
#include "../Include/ModelLoader.h"
#include "../Include/bitmap_image.hpp"
#include "../Include/PhotonMap.h"

//PARAMETERS
#define PHOTON_GATHER_RANGE 0.1


#define _DOF_ENABLE_ false
#define _AA_ENABLE true
#define _TEXTURE_ENABLE_ false
#define _PHOTON_MAPPING_ENABLE_ true

//VS14 FIX
#ifdef _WIN32
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) {
	return _iob;
}
#endif

/*
using namespace std;
using glm::vec3;
using glm::mat3;*/

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */

/*const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
bitmap_image *texture;
bitmap_image *normal_texture;
int t;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

/*void Update();
void Draw(model::Scene scene, photonmap::PhotonMap& photon_map);
glm::vec3 Trace(std::vector<Triangle>& triangles, glm::vec3 cameraPos, glm::vec3 direction, photonmap::PhotonMap& photon_map, model::Scene& scene);

int main(int argc, char** argv) {
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	std::vector<Triangle> model = std::vector<Triangle>();
	LoadTestModel(model);

	model::Scene scene;
	scene.addTriangles(model); //For testing, actual use should involve type Model
	model::LightSource basic_light(glm::vec3(0.0, -0.85, 0.0), glm::vec3(0, 1.0, 0), 8);
	scene.addLight(basic_light);

	//model::Model cornell_box = model::Model("Bench_WoodMetal.obj");
	//model::Model cornell_box = model::Model("../model.txt");
	//scene.addModel(&cornell_box);
	//std::vector<Triangle>* model = cornell_box.getFaces();

	//printf("%d\n", model.size());
	//LoadTestModel(model, cornell_box);

	
	
	texture = new bitmap_image("Resources/bench_woodmetal_a.bmp");
	normal_texture = new bitmap_image("Resources/N1.bmp");

	
		photonmap::PhotonMapper photon_mapper(scene, 10000, 10); //Number of photons, number of bounces
		photonmap::PhotonMap photon_map(&photon_mapper);
		scene.removeFront();
		glm::vec3 campos(0.0, 0.0, -2.0);
	/*while (NoQuitMessageSDL()) {

		Uint8* keystate = SDL_GetKeyState(0);
		if (keystate[SDLK_UP]) {
			// Move camera forward
			campos.z += 0.025;
		}
		if (keystate[SDLK_DOWN]) {
			// Move camera backward
			campos.z -= 0.025;
		}
		if (keystate[SDLK_LEFT]) {
			// Move camera to the left
			campos.x -= 0.025;
		}
		if (keystate[SDLK_RIGHT]) {
			// Move 
			campos.x += 0.025;
		}

		// Render photons
		if (SDL_MUSTLOCK(screen))
			SDL_LockSurface(screen);
		SDL_FillRect(screen, NULL, 0x000000);

		// TEMP DEBUG RENDER PHOTONS

		std::vector<photonmap::PhotonInfo>& g_photoninfo = photon_mapper.getDirectPhotons();
		std::vector<photonmap::PhotonInfo>& photoninfo = photon_mapper.getIndirectPhotons();
		//std::vector<photonmap::PhotonInfo>& photoninfo = photon_mapper.getShadowPhotons(); SDL_FillRect(screen, NULL, 0xFFFFFF);

		//printf("\n \nPHOTONS: %d \n", photoninfo.size());
		int count = 0;
		for (auto pi : photoninfo) {
			glm::vec3 pos = pi.position;
			// Project position to 2D:
			float xScr = (pos.x- campos.x) / (pos.z-campos.z);
			float yScr = (pos.y-campos.y) / (pos.z-campos.z);

			// Scale and bias
			xScr += 1;
			yScr += 1;
			xScr *= 0.5;
			yScr *= 0.5;
			xScr *= 500;
			yScr *= 500;

			// Draw point (only if the point isn't behind the camera, otherwise we get weird wrapping)
			if ((pos.z - campos.z) > 0.0) {
				PutPixelSDL(screen, xScr, yScr, pi.color*255.0f);
			}
			count++;
		}

		if (SDL_MUSTLOCK(screen))
			SDL_UnlockSurface(screen);

		SDL_UpdateRect(screen, 0, 0, 0, 0);
	}

		int a;
		std::cin >> a;
        return 0;*/

	/*while( NoQuitMessageSDL() )
	{
		Update();
		//Draw(cornell_box.getFaces());
		Draw(scene, photon_map);
	}
	std::cout << "Render Complete\n";
	while (NoQuitMessageSDL());
	delete texture;
	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update() {
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	//std::cout << "Render time: " << dt << " ms." << std::endl;
}

void Draw(model::Scene scene, photonmap::PhotonMap& photon_map) {
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	float DOF_focus_length = 2.750f;

	glm::vec3 color;

	glm::vec3 cameraPos(0.0, 0.0, -2.0);
	glm::vec3 cameraDirection(0.0f, 0.0f, 0.0f);
	//glm::vec3 cameraPos(8.0, -8.0, -10.0);
	//glm::vec3 cameraDirection(-(float)M_PI / 5.0f, -(float)M_PI / 6.0f, 0.0f);

	float fov = 80;
	float aspect_ratio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	float focalLength = 1.0f;
	/*
	- Loosely based off of https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays
	- Same principle as Pinhole, except re-scaled such that the xScr, yScr are in camera-space.
	*/
	/*float fovFactor = tan(M_PI * (fov*0.5) / 180);

	float x_rotation = 0.5f;
	float y_rotation = 0.0f;


	#pragma omp parallel for
	for (int y = 0; y<SCREEN_HEIGHT; ++y) {
		for (int x = 0; x<SCREEN_WIDTH; ++x) {

			float xScr = (2 * (x - SCREEN_WIDTH / 2) / (float)(SCREEN_WIDTH)) * aspect_ratio * fovFactor;
			float yScr = (2 * (y - SCREEN_HEIGHT / 2) / (float)(SCREEN_HEIGHT)) * fovFactor;
			glm::vec3 direction(xScr, yScr, focalLength);
			direction = glm::normalize(direction);

			direction = glm::rotateX(direction, cameraDirection.x);
			direction = glm::rotateY(direction, cameraDirection.y);
			direction = glm::rotateY(direction, cameraDirection.z);
			//glm::vec3 color( 1.0, 0.0, 0.0 );
			if (_DOF_ENABLE_) {
				glm::vec3 color_buffer(0.0f, 0.0f, 0.0f);
				//Take multiple samples with camera rotated about focus point for DOF. TODO: modify Trace() to accept camera transformations (as mat4)
				int samples = 0;
				for (float xr = -0.02f; xr <= 0.02f; xr += 0.01f) {
					for (float yr = -0.02f; yr <= 0.02f; yr += 0.01f) {
						samples++;
						glm::vec3 focus_point = cameraPos + glm::normalize(glm::vec3(0, 0, focalLength)) * glm::vec3(DOF_focus_length);
						glm::vec3 cameraClone(cameraPos);
						cameraClone -= focus_point; //translate such that focus_point is origin
						cameraClone = glm::rotateX(cameraClone, xr); //rotation in X
						cameraClone = glm::rotateY(cameraClone, yr); //rotation in Y
																	 //For direction vector: give same rotation as camera
						direction = glm::rotateX(direction, xr);
						direction = glm::rotateY(direction, yr);

						cameraClone += focus_point; //undo translation, effect is camera has rotated about focus_point
						color_buffer += Trace(scene.getTrianglesRef(), cameraClone, direction, photon_map, scene);
					}
				}

				color = color_buffer / glm::vec3((float)samples);
			}
			else {

				if (_AA_ENABLE) {
					// Anti-aliasing:
					glm::vec3 colorAA(0.0, 0.0, 0.0);

					for (float xAA = -0.5f / (float)SCREEN_WIDTH; xAA <= 0.5f / (float)SCREEN_WIDTH; xAA += 0.50f / (float)SCREEN_WIDTH) {
						for (float yAA = -0.5f / (float)SCREEN_HEIGHT; yAA <= 0.5f / (float)SCREEN_HEIGHT; yAA += 0.50f / (float)SCREEN_HEIGHT) {

							float xScr = (2 * (x - SCREEN_WIDTH / 2) / (float)(SCREEN_WIDTH)) * aspect_ratio * fovFactor;
							float yScr = (2 * (y - SCREEN_HEIGHT / 2) / (float)(SCREEN_HEIGHT)) * fovFactor;

							glm::vec3 direction(xScr + xAA, yScr + yAA, focalLength);
							direction = glm::normalize(direction);

							direction = glm::rotateX(direction, cameraDirection.x);
							direction = glm::rotateY(direction, cameraDirection.y);
							direction = glm::rotateY(direction, cameraDirection.z);

							colorAA += Trace(scene.getTrianglesRef(), cameraPos, direction, photon_map, scene);
						}
					}


					//color = Trace(xScr, yScr, model, cameraPos, direction);
					color = colorAA / 9.0f;
				}
				else {
					color = Trace(scene.getTrianglesRef(), cameraPos, direction, photon_map, scene);
				}

			}
			PutPixelSDL(screen, x, y, color);
		}
		printf("Progress: %f \n", ((float)y / (float)SCREEN_HEIGHT) * 100.0f);
	}


	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

//Returns colour of nearest intersecting triangle
glm::vec3 Trace( std::vector<Triangle>& triangles, glm::vec3 cameraPos, glm::vec3 direction, photonmap::PhotonMap& photon_map, model::Scene& scene) {
	Intersection closest_intersect;
	glm::vec3 color_buffer = glm::vec3(0.0f, 0.0f, 0.0f);

	Ray ray(cameraPos, direction);
	if (ray.closestIntersection(triangles, closest_intersect)) {
		glm::vec3 baseColour = closest_intersect.color;
		

		// Calculate interpolated colour:
		/*
			TEMP (explanation of barycentric coordinates):
				- Barycentric coordinates are an alternative way to represent a triangle as the combination of different weights.
				How it boils down is that a coordinate on the surface of a triangle plane can be given a 3D barycentric coordinate (a, b, c). 

				The position of that coordinate can then be converted to euclidean space by taking  pos = v0*a + v1*b + v2*c
				- Conveniently, this allows us to use it to interpolate values. We can give each vertex a colour, and interpolate 
				between them, we can also give each vertex a uv coordinate (mapping to texture space) and use this same
				math to map each point within a triangle to the correct point in uv-space.
		*/
		/*Triangle t = triangles[closest_intersect.index];
		glm::vec3 barycentric_coords = t.calculateBarycentricCoordinates(closest_intersect.position);
		
		glm::vec2 baseColourUV = t.uv0*barycentric_coords.x + t.uv1*barycentric_coords.y + t.uv2*barycentric_coords.z;
		int tw = texture->width();
		int th = texture->height();

		int tx = (int)((float)(tw)*baseColourUV.x);
		int ty = (int)((float)(th)*baseColourUV.y);


		rgb_t colour;
		//std::cout << "TX: " << tx << " TY: " << ty << std::endl;
		if (tx >= 0 && tx < tw && ty >= 0 && ty <= th && _TEXTURE_ENABLE_) {
			texture->get_pixel(tx, ty, colour);

			baseColour.r = (float)colour.red/255.0f;
			baseColour.g = (float)colour.green/225.0f;
			baseColour.b = (float)colour.blue/255.0f;
		}

		// Get normal
		glm::vec3 surface_normal = -(t.n0*barycentric_coords.x + t.n1*barycentric_coords.y + t.n2*barycentric_coords.z);
		glm::vec3 texture_normal;

		// Get normal map data:
		tw = normal_texture->width();
		th = normal_texture->height();

		tx = (int)((float)(tw)*baseColourUV.x);
		ty = (int)((float)(th)*baseColourUV.y);


		rgb_t norm;
		//std::cout << "TX: " << tx << " TY: " << ty << std::endl;
		if (tx >= 0 && tx < tw && ty >= 0 && ty <= th) {
			normal_texture->get_pixel(tx, ty, norm);

			texture_normal.r = (float)norm.red / 255.0f;
			texture_normal.g = (float)norm.green / 225.0f;
			texture_normal.b = (float)norm.blue / 255.0f;
		}

		// Combine normal with texture normal:
		glm::vec3 combined_normal = texture_normal*2.0f - 1.0f;

		//glm::vec3 tangent   = glm::normalize(t.v1 - t.v0);
		glm::vec3 edge1 = t.v1 - t.v0;
		glm::vec3 edge2 = t.v2 - t.v0;

		float du1 = t.uv1.x - t.uv0.x;
		float dv1 = t.uv1.y - t.uv1.y;
		float du2 = t.uv2.x - t.uv0.x;
		float dv2 = t.uv2.y - t.uv0.y;

		glm::vec3 dp2perp = glm::cross(edge2, surface_normal);
		glm::vec3 dp1perp = glm::cross(surface_normal, edge1);
		glm::vec3 tangent = dp2perp * du1 + dp1perp * du2;
		glm::vec3 bitangent = dp2perp * dv1 + dp1perp * dv2;

		tangent = glm::cross(surface_normal, bitangent);
		bitangent = glm::cross(surface_normal, tangent);

		float invmax = glm::inversesqrt(glm::max(glm::dot(tangent, tangent), glm::dot(bitangent, bitangent)));

		glm::mat3x3 TBN = glm::mat3x3(tangent*invmax, bitangent*invmax, surface_normal);
		combined_normal = glm::normalize(TBN*texture_normal);

		//PHOTON MAPPING RENDERER SECTION
		glm::vec3 photon_radiance(0.0f);
		if (_PHOTON_MAPPING_ENABLE_) {
			//photon_radiance = photon_map.gatherPhotons(closest_intersect.position, triangles[closest_intersect.index].getNormal());
			//baseColour = photon_map.gatherPhotons(closest_intersect.position, ray.direction);
			std::vector<std::pair<size_t, float>> direct_photons_in_range, shadow_photons_in_range;
			photon_map.getIndirectPhotonsRadius(closest_intersect.position, PHOTON_GATHER_RANGE, direct_photons_in_range);
			photon_map.getShadowPhotonsRadius(closest_intersect.position, PHOTON_GATHER_RANGE, shadow_photons_in_range);
			const glm::vec3 light_pos = scene.light_sources[0]->position;
			const glm::vec3 light_dir = glm::normalize(light_pos - closest_intersect.position);
			const glm::vec3 light_normal = scene.light_sources[0]->direction;
			float light_factor = glm::dot(-light_dir, -ray.direction);
			if (light_factor >= FLT_EPSILON) {
				//const glm::vec3 radiance = light_factor * closest_intersect.color;
				//colorAccumulator
                photon_radiance = 
			}
		}

		// SIMPLE LIGHTING
		//const glm::vec3 light_position(-30.0, -30, 0.0);
		const glm::vec3 light_position(0.0, -0.98, 0.0);
		glm::vec3 dir_to_light = light_position - closest_intersect.position;
		float light_distance = glm::length(dir_to_light);
		float light_factor = 1.0 - glm::clamp((light_distance / 100.0), 0.0, 1.0);
		light_factor = glm::clamp((double)light_factor, 0.25, 1.0)*2.0f;

		// Send a ray between the point on the surface and the light. (The *0.01 is because we need to step a little bit off the surface to avoid self-intersection)
		Ray lightRay(closest_intersect.position + dir_to_light*0.01f, glm::normalize(dir_to_light));
		Intersection closest_intersect2;

		// Specularity
		glm::vec3 LightReflect = glm::normalize(glm::reflect(-dir_to_light, surface_normal));
		float SpecularFactor   = glm::dot(-ray.direction, LightReflect);
		if (SpecularFactor > 0) {
			SpecularFactor = pow(SpecularFactor, 32.0f);
		}

		// If the ray intersects with something, and the distance to the intersecting object is closer than 
		if (lightRay.closestIntersection(triangles, closest_intersect2)) {
			if (closest_intersect2.distance < light_distance) {
				light_factor = 0.0;
			}
		} 
		
		

		//return (surface_normal + glm::vec3(1.0)) / 2.0f;
		//return (combined_normal +glm::vec3(1.0))/2.0f;
		return baseColour * (photon_radiance + light_factor + SpecularFactor) ;
	}
	return color_buffer;
}

/*bool LoadBMP(const char *fileName) {
	FILE *file;
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int size;
	unsigned int width, height;
	unsigned char *data;


	file = fopen(fileName, "rb");

	if (file == NULL) {
		//MessageBox(NULL, L"Error: Invaild file path!", L"Error", MB_OK);
		return false;
	}

	if (fread(header, 1, 54, file) != 54) {
		//MessageBox(NULL, L"Error: Invaild file!", L"Error", MB_OK);
		return false;
	}

	if (header[0] != 'B' || header[1] != 'M') {
		//MessageBox(NULL, L"Error: Invaild file!", L"Error", MB_OK);
		return false;
	}

	dataPos = *(int*)&(header[0x0A]);
	size = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	if (size == NULL)
		size = width * height * 3;
	if (dataPos == NULL)
		dataPos = 54;

	data = new unsigned char[size];

	fread(data, 1, size, file);

	fclose(file);
}*/
