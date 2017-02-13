
#ifndef PHOTONMAP_H
#define PHOTONMAP_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "nanoflann.hpp"
#include "ModelLoader.h"
#include "Raytracer.h"
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <random>
#include <functional>
#include "Utility.h"

static std::default_random_engine generator(time(nullptr));
static std::uniform_real_distribution<float> distribution(0, 1);
static auto Rand = std::bind(distribution, generator);

namespace model {
	class Scene;
}

//class SDL_Surface;

namespace photonmap {
	class PhotonMapper;
	class PhotonInfoWrapper;

	typedef nanoflann::KDTreeSingleIndexAdaptor<
		nanoflann::L2_Simple_Adaptor<float, PhotonInfoWrapper>,
		PhotonInfoWrapper, 3> photon_tree_t;

	class PhotonMap {
		//photon_tree_t photon_map;
		photon_tree_t direct_photon_map;
		photon_tree_t indirect_photon_map;
		photon_tree_t shadow_photon_map;

		PhotonMapper& p;
	public:
		PhotonMap(PhotonMapper* _p);
		void getDirectPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices);
		void getIndirectPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices);
		void getShadowPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices);
		//void getCausticPhotonsRadius();
		bool nearestDirectPhotonInRange(const glm::vec3& pos, const float radius, size_t& index);
	};

	struct PhotonInfo {
		glm::vec3 color;
		glm::vec3 position;
		glm::vec3 direction;
		inline PhotonInfo() {}
		inline PhotonInfo(glm::vec3 c, glm::vec3 p, glm::vec3 d) : color(c), position(p), direction(d) {}
	};

	class PhotonInfoWrapper {
	public:
		std::vector<PhotonInfo> photons;
		inline PhotonInfoWrapper() {};
		
		inline void emplace_back(glm::vec3 c, glm::vec3 p, glm::vec3 d) {
			photons.emplace_back(c, p, d);
		}
		inline unsigned int size() {
			return photons.size();
		}
		// Must return the number of data points
		inline size_t kdtree_get_point_count() const { return photons.size(); }

		// Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
		inline float kdtree_distance(const float* p1, const size_t idx_p2, size_t /*size*/) const {
			const float d0 = p1[0] - photons[idx_p2].position.x;
			const float d1 = p1[1] - photons[idx_p2].position.y;
			const float d2 = p1[2] - photons[idx_p2].position.z;
			return d0*d0 + d1*d1 + d2*d2;
		}

		// Returns the dim'th component of the idx'th point in the class:
		// Since this is inlined and the "dim" argument is typically an immediate value, the
		//  "if/else's" are actually solved at compile time.
		inline float kdtree_get_pt(const size_t idx, int dim) const {
			if (dim == 0) return photons[idx].position.x;
			else if (dim == 1) return photons[idx].position.y;
			else return photons[idx].position.z;
		}

		// Optional bounding-box computation: return false to default to a standard bbox computation loop.
		//   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
		//   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
		template <class BBOX>
		bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
	};

	struct Photon {
		Ray beam;
		//Need to accumulate color
		unsigned int depth;
		bool absorbed = false;
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); //Photon is initially white by default
		Photon(glm::vec3 o, glm::vec3 d, unsigned int depth = 3, float intensity = 1);
	};

	class PhotonMapper {
		unsigned int number_of_photons;
		unsigned int number_of_bounces;

		std::vector<Photon> photons;
		std::vector<PhotonInfo> gathered_photons;
	public:
		PhotonInfoWrapper direct_photons;
		PhotonInfoWrapper indirect_photons;
		PhotonInfoWrapper shadow_photons;

	
		inline std::vector<PhotonInfo>& getGatheredPhotons() { return gathered_photons; }
		inline std::vector<PhotonInfo>& getDirectPhotons() { return direct_photons.photons; }
		inline std::vector<PhotonInfo>& getIndirectPhotons() { return indirect_photons.photons; }
		inline std::vector<PhotonInfo>& getShadowPhotons() { return shadow_photons.photons; }

		inline PhotonMapper(model::Scene& scene, unsigned int photon_count = 1000, unsigned int bounces = 3) : number_of_photons(photon_count), number_of_bounces(bounces) {
			photons.reserve(number_of_photons);
			mapScene(scene);
		}
		//void render(SDL_Surface* screen);
		void mapScene(model::Scene& scene); //Generate photons for each light source, distributed by light intensity

		inline glm::vec3 getDirection(size_t i) { return gathered_photons[i].direction; }
		inline glm::vec3 getEnergy(size_t i) { return gathered_photons[i].color; }
	};
}

#endif // PHOTONMAP_H

