//#define DEBUG
#include "../Include/ModelLoader.h"
#define EQUAL(s1, s2) strcmp(s1, s2) == 0

std::string trim(const std::string& str, const std::string& whitespace = " \t\n\r") {
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

namespace model {
	void Scene::getTriangles(std::vector<Triangle>& triangles) {
		for (auto model : models) {
			for (auto triangle : *(model->getFaces())) {
				triangles.push_back(triangle);
			}
		}
	}

	Model::Model(std::string file_name) {
		std::string token;
		
		std::ifstream file;
		file.open(file_name);

		if (file.is_open()) {
			std::string line;
			
			while (std::getline(file, line)) {
				std::istringstream stream(line);
				stream >> token;
				
				if (token[0] == '#') continue; //Skip comments
				else if (token == "v") parseVertex(stream);
				else if (token == "vt") parseVertexTexture(stream);
				else if (token == "vn") parseVertexNormal(stream);
				else if (token == "f") parseFace(stream);
				//else if (token == "mtllib") parseMaterialLib(stream);
				//else if (token == "usemtl") parseUseMaterial(stream);
				else std::cout << "Token not supported: '" << token << "'." << std::endl;

			}
			file.close();
		}
		else {
			std::cerr << "Unable to open file: " << file_name << std::endl;
		}
	}

	//Tested. Functioning correctly.
	void Model::parseVertex(std::istringstream& vertex) {
		float x, y, z, w;
		vertex >> x >> y >> z;
		//if (!(vertex >> w)) w = 1.0f;
		vertices.emplace_back(x, y, z);
#ifdef DEBUG
		printf("Added vertex: (%f, %f, %f) \n", x, y, z);
#endif
	}

	void Model::parseVertexTexture(std::istringstream& texture) {
		float u, v, w;
		texture >> u >> v;
		//if (!(texture >> w)) w = 0.0f;
		vertex_textures.emplace_back(u, v);
#ifdef DEBUG
		printf("Added vertex_texture: (%f, %f) \n", u, v);
#endif
	}

	void Model::parseVertexNormal(std::istringstream& normal) {
		//May not be normalized
		float x, y, z;
		normal >> x >> y >> z;
		vertex_normals.emplace_back(x, y, z);
#ifdef DEBUG
		printf("Added vertex_normal: (%f, %f, %f) \n", x, y, z);
#endif
	}

	void Model::parseFace(std::istringstream& face) {
		// vertex/texture/normal
		std::string v[3];
		face >> v[0] >> v[1] >> v[2];
		//glm::umat3 oface = glm::umat3();
		for (int i = 0; i < 3; i++) {
			int vertex, texture, normal;
			std::istringstream section(v[i]);
			section >> vertex;
			
			if (section.peek() == '/') { 
				section.ignore();
				if (section.peek() != '/') section >> texture;
			}
			else continue;

			if (section.peek() == '/') {
				section.ignore();
				section >> normal;
			}
			else continue;
			//Assume vertex/texture/normal are all available
			vertexIndices.push_back(vertex-1);
			textureIndices.push_back(texture-1);
			normalIndices.push_back(normal-1);
#ifdef DEBUG
			printf("Added face: (%d, %d, %d) \n", vertex, texture, normal);
#endif
		}
	}

	void Model::parseMaterialLib(std::istringstream& lib){
		std::string file_name;
		lib >> file_name;
		std::ifstream file;
		file.open(file_name);

		if(file.is_open()){
			std::string line, token;
			while (std::getline(file, line)) {
				std::istringstream stream(line);
				stream >> token;

				Material m;
				if (materials.size()) { m = getActiveMaterial(); }
				if(token[0] == '#') continue; //skip comments
				else if (token == "newmtl") {
					std::string name;
					stream >> name;
					materials.push_back(Material());
					material_map.insert(std::pair<std::string, unsigned int>(name, materials.size()-1));
				}
				else if (token == "Ka") stream >> m.ambient_color.x >> m.ambient_color.y >> m.ambient_color.z; //ambient color
				else if (token == "Kd") stream >> m.diffuse_color.x >> m.diffuse_color.y >> m.diffuse_color.z; //diffuse color
				else if (token == "Ks") stream >> m.specular_color.x >> m.specular_color.y >> m.specular_color.z; //specular color
				else if (token == "Ns") stream >> m.specular_exponent;
				else if (token == "Tr") stream >> m.transparency;
				//else if (token == "map_Ka") parseAmbientMap(stream);
				//else if (token == "map_Kd") parseDiffuseMap(stream);
				//else if (token == "map_Ks") parseSpecularMap(stream);
				//else if (token == "map_bump") parseBumpMap(stream);
			}
		}else{
			std::cerr << "Failed to open material: " << file_name << std::endl;
		}
	}

	void Model::parseUseMaterial(std::istringstream& material){
		std::string material_name;
		material >> material_name;
		active_material = material_map[material_name];
	}
}