#include "../Include/PhotonMap.h"
//#include "../Include/SDLauxiliary.h"

namespace photonmap {
	Photon::Photon(glm::vec3 o, glm::vec3 d, unsigned int _depth, float intensity) : beam(o, d), depth(_depth) {
        color *= glm::vec3(intensity);
    }

	void PhotonMapper::mapScene(model::Scene& scene) {
		float total_light_intensity = 0.0f;
		for (auto light : scene.light_sources) { total_light_intensity += light->intensity; }

		unsigned int assigned_photons = 0;
		std::vector<unsigned int> photons_per_light;
		for (int i = 0; i < scene.light_sources.size(); i++) {
			photons_per_light.push_back((unsigned int)(((float)number_of_photons) * (scene.light_sources[i]->intensity / total_light_intensity)));
			assigned_photons += photons_per_light[i];
		}

		photons_per_light[scene.light_sources.size() - 1] += number_of_photons - assigned_photons; //Give any unused photons (due to truncation) to the last light source
		
		/*
        ** http://users.csc.calpoly.edu/~zwood/teaching/csc572/final15/cthomp/index.html
		** At this point the photons have been divided between available light sources, the split being weighted by the intensity of the light source
		** (although I am not sure if this is the best approach. It shouldn't matter for simple examples however.)
		**
		** First step is to generate random directions for each photon based on an appropriate probability density
		** Next step is to simulate the action of the photon throughout the scene, employing russion-roulette approach for randomising photon action
		**   -
		*/

		//Generate photons
		/*for (int i = 0; i < scene.light_sources.size(); i++) {
			printf("Adding %d photons to light #%d\n", photons_per_light[i], i);
            float total_photons = photons_per_light[i];
			while (photons_per_light[i]--) {
				//Light uniformly distributed in all directions, not necessarily the best approach, we probably want to stop upwards directed light
				auto dir = glm::sphericalRand(1.0f);
				photons.emplace_back(scene.light_sources[i]->position, dir, number_of_bounces, std::pow(2.0f, scene.light_sources[i]->intensity)/(total_photons) );
				//auto pos = scene.light_sources[i]->position;
				//printf("Photon info: Dir=(%f, %f, %f), Pos=(%f, %f, %f), Bounces=%d\n", dir.x, dir.y, dir.z, pos.x, pos.y, pos.z, number_of_bounces);
			}
			printf("Added %d photons to light #%d\n", photons.size(), i);
		}*/

		/* 
		** Emulate photons
		** Currently handling:
		**	-Global Illumination
		**  -Shadows
		**
		** Planned:
		**  -Refraction, requires russian-roulette style decision making for efficiency
		*/

		Intersection closest, shade;
		for (int i = 0; i < number_of_photons; i++) {
			glm::vec3 origin = scene.light_sources[0]->position;//glm::vec3(-0.1, -0.85f, -0.1);//glm::linearRand(glm::vec3(-0.1, -0.85f, -0.1), glm::vec3(0.1, -0.99f, 0.1));
			float u = Rand(); float v = 2 * M_PI * Rand();
			glm::vec3 direction = MATH::CosineWeightedHemisphereDirection(scene.light_sources[0]->direction);
			Ray ray(origin, direction);
			glm::vec3 radiance = glm::dot(ray.direction, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec3(1.0f); //Last term is light color (white)

			for (int bounce = 0; bounce < number_of_bounces; bounce++) {
				//bounce == 0 => direct lighting => shoot shadow photons
				if (ray.closestIntersection(scene.getTrianglesRef(), closest)) {
					glm::vec3 intersection = closest.position;
					glm::vec3 normal = scene.getTrianglesRef()[closest.index].getNormal();
					glm::vec3 reflection = MATH::CosineWeightedHemisphereDirection(normal);

					if (bounce > 0) { //Indirect lighting
						indirect_photons.emplace_back(radiance, intersection, ray.direction);

						float p = (radiance.r + radiance.g + radiance.b) * 0.1;
						float rand = (float)std::rand() / (float)RAND_MAX;
						if (rand > p) {
							break;
						}
					}
					else { //Direct lighting
						direct_photons.emplace_back(radiance, intersection, ray.direction);

						//Add shadows
						Ray shadow(intersection + 0.001f * ray.direction, ray.direction);
						while (shadow.closestIntersection(scene.getTrianglesRef(), shade)) {
							shadow_photons.emplace_back(glm::vec3(0), shade.position, ray.direction);
							shadow.origin = shade.position + 0.001f * ray.direction;
						}
					}

					radiance = glm::max(0.0f, glm::dot(-ray.direction, normal)) * (radiance * closest.color);
					ray.origin = intersection + 0.001f*normal;
					ray.direction = reflection;
				}
				else break;
			}
		}

		//Weight photons
		for (auto photon : direct_photons.photons) {
			photon.color /= (direct_photons.size() + indirect_photons.size());
		}
		for (auto photon : indirect_photons.photons) {
			photon.color /= (direct_photons.size() + indirect_photons.size());
		}


		/*for (int i = 0; i < number_of_photons; i++) {
			glm::vec3 intensity(std::pow(2.0f, scene.light_sources[0]->intensity)/photons_per_light[0]);
			glm::vec3 origin = scene.light_sources[0]->position;//glm::vec3(-0.1, -0.85f, -0.1);//glm::linearRand(glm::vec3(-0.1, -0.85f, -0.1), glm::vec3(0.1, -0.99f, 0.1));

			float u = Rand();
			float v = 2 * M_PI * Rand();
			glm::vec3 direction = glm::vec3(std::cos(v)*std::sqrt(u), std::sin(v)*std::sqrt(u), std::sqrt(1-u));

			Ray ray(origin, direction); //Ray is not being updated each iteration
			unsigned int bounce_limit = number_of_bounces;
			while (bounce_limit-- && ray.closestIntersection(scene.getTrianglesRef(), closest)) {
                //bounce_limit = 0;
				glm::vec3 pd = closest.color;// glm::vec3(3); //Pr(Diffuse reflection)
				glm::vec3 ps(0.0); //Pr(Specular reflection)

				float Pr = std::max(pd.r + ps.r, std::max(pd.g + ps.g, pd.b + ps.b));
				float Pd = Pr * (pd.r + pd.g + pd.b) / (pd.r + pd.g + pd.b + ps.r + ps.g + ps.b);
				float Ps = Pr - Pd;

				float r = Rand();
				if (r < Pd) { //Diffuse reflection
					gathered_photons.emplace_back(intensity, closest.position, ray.direction); //Store photon
					ray.direction = glm::normalize(glm::reflect(ray.direction, scene.getTrianglesRef()[closest.index].getNormal())); //Reflect photon
					ray.origin = closest.position + ray.direction * glm::vec3(0.000001); //Slight offset to prevent colliding with current geometry
					intensity *= pd / Pd; //Adjust powers to suit probability of survival
				}
				else if (r < Pd + Ps) { //Specular reflection
					intensity *= ps / Ps;
				}
				else { //Absorb photon (TODO: only store if diffuse surface)
					gathered_photons.emplace_back(intensity, closest.position, ray.direction); //Store photon
					break;
				}
			}
            if(i % 100 == 0){printf("Photon = %d\n", i);}
		}*/
		//printf("Gathered %d photons of data.\n", gathered_photons.size());
		printf("Gathered %d photons of direct data.\n", direct_photons.size());
		printf("Gathered %d photons of indirect data.\n", indirect_photons.size());
		printf("Gathered %d photons of shadow data.\n", shadow_photons.size());
	}
	
	/*void PhotonMapper::render(SDL_Surface* screen){
		for (auto pi : this->gathered_photons) {
			glm::vec3 pos = pi.position;

			// Project position to 2D:
			float xScr = pos.x / pos.z;
			float yScr = pos.y / pos.z;

			// Scale and bias
			xScr += 1;
			yScr += 1;
			xScr *= 0.5;
			yScr *= 0.5;
			xScr *= 500;
			yScr *= 500;

			// Draw point
			//PutPixelSDL(screen, xScr, yScr, pi.color);

		}
	}*/

	PhotonMap::PhotonMap(PhotonMapper* _p) : p(*_p), 
		direct_photon_map(3, (_p->direct_photons), nanoflann::KDTreeSingleIndexAdaptorParams(10)),
		indirect_photon_map(3, (_p->indirect_photons), nanoflann::KDTreeSingleIndexAdaptorParams(10)),
		shadow_photon_map(3, (_p->shadow_photons), nanoflann::KDTreeSingleIndexAdaptorParams(10))
	{
		//photon_map(3, points, nanoflann::KDTreeSingleIndexAdaptorParams(10)), p(points)
		//photon_tree_t photon_map(3, p, nanoflann::KDTreeSingleIndexAdaptorParams(10));
		direct_photon_map.buildIndex();
		indirect_photon_map.buildIndex();
		shadow_photon_map.buildIndex();
		//printf("Size of photon_map = %d", photon_map.size());
	}

	void PhotonMap::getDirectPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices) {
		const float point[3] = { pos.x, pos.y, pos.z };
		nanoflann::SearchParams params;
		params.sorted = false;
		const size_t count = direct_photon_map.radiusSearch(point, radius, indices, params);
	}

	void PhotonMap::getIndirectPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices) {
		const float point[3] = { pos.x, pos.y, pos.z };
		nanoflann::SearchParams params;
		params.sorted = false;
		const size_t count = indirect_photon_map.radiusSearch(point, radius, indices, params);
	}

	void PhotonMap::getShadowPhotonsRadius(const glm::vec3& pos, const float radius, std::vector< std::pair<size_t, float> >& indices) {
		const float point[3] = { pos.x, pos.y, pos.z };
		nanoflann::SearchParams params;
		params.sorted = false;
		const size_t count = shadow_photon_map.radiusSearch(point, radius, indices, params);
	}

	//void PhotonMap::getCausticPhotonsRadius() {}

	bool PhotonMap::nearestDirectPhotonInRange(const glm::vec3& pos, const float radius, size_t& index) {}

}
