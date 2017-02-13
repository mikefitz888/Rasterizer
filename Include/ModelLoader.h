#ifndef MODELLOADER_H
#define MODELLOADER_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_integer.hpp>
#include "Triangle.h"
#include "bitmap_image.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

namespace model {
	class Model;

	enum LTYPE : unsigned int {POINT=0};
	struct LightSource {
		float intensity;
		glm::vec3 position;
		glm::vec3 direction;
		LTYPE type;
		inline LightSource(glm::vec3 p, glm::vec3 d, float i = 1.0f, LTYPE t = LTYPE::POINT) : position(p), direction(d), intensity(i), type(t) {}
	};

	struct Scene {
	private:
		std::vector<Triangle> triangles;
	public:
		std::vector<Model*> models;
		std::vector<LightSource*> light_sources;
		void getTriangles(std::vector<Triangle>& triangles);
	
		inline std::vector<Triangle>& getTrianglesRef() { return triangles; }
		inline void addModel(Model* model) {
			models.emplace_back(model);
		}

		inline void addLight(LightSource& light) {
			light_sources.emplace_back(&light);
		}

		inline void removeFront() {
			triangles.erase(triangles.begin(), triangles.begin() + 2);
		}

		inline void addTriangles(std::vector<Triangle>& ts) {
			for (auto t : ts) {
				triangles.push_back(t);
			}
		}
	};

	class Material {
	public:
		glm::vec3 ambient_color, diffuse_color, specular_color;
		float specular_exponent, transparency;
		bitmap_image* ambient_map, diffuse_map, specular_map, bump_map; 
		inline Material() {};
	};

	class Model {
		std::vector<Material> materials;
		std::map<std::string, unsigned int> material_map;
		unsigned int active_material;
		bool modified = true;

		std::vector<unsigned int> vertexIndices, textureIndices, normalIndices;
		std::vector<glm::vec3> vertices, vertex_normals;
		std::vector<glm::vec2> vertex_textures; //UVs
		std::vector<Triangle> triangles;
		std::vector<int> mtl_map;

	private:
		void parseVertex(std::istringstream& vertex);
		void parseVertexNormal(std::istringstream& vertex_normal);
		void parseVertexTexture(std::istringstream& vertex_texture);
		void parseFace(std::istringstream& face);
		void parseUseMaterial(std::istringstream& material);
		void parseMaterialLib(std::istringstream& lib);
	public:
		Model(std::string file_name);
		inline Model() {};

		inline Material& getActiveMaterial(){
			return materials[active_material];
		}

		inline void removeFront() {
			triangles.erase(triangles.begin(), triangles.begin() + 2);
		}

		//Convert to Triangles for raytracer
		inline std::vector<Triangle>* getFaces() {
			if (modified) {
				modified = false;
				triangles.clear();
				for (int i = 0; i < vertexIndices.size(); i += 3) {
					triangles.emplace_back(
						vertices[vertexIndices[i]],
						vertices[vertexIndices[i + 1]],
						vertices[vertexIndices[i + 2]],
						vertex_textures[textureIndices[i]],
						vertex_textures[textureIndices[i + 1]],
						vertex_textures[textureIndices[i + 2]],
						vertex_normals[normalIndices[i]],
						vertex_normals[normalIndices[i + 1]],
						vertex_normals[normalIndices[i + 2]]
					);
				}
			}
			return &triangles;
		}
	};
}

#endif // !MODELLOADER_H

