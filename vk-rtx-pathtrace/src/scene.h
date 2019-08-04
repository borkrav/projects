#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

struct SphereBinding {
	glm::vec3 centre;
	float radius;
};


class Sphere {

private:
	glm::vec3 m_centre;
	float m_radius;
	glm::mat4 m_transform;


public:

	Sphere() = delete;

	Sphere(const glm::vec3 &centre, const float &radius) :m_centre(centre), m_radius(radius) {
		m_transform = glm::mat4(1.0f);
		m_transform = glm::translate(m_transform, centre);
		m_transform = glm::scale(m_transform, glm::vec3(radius*2));

	}


	glm::vec3 getCentre() { return m_centre; }
	float getRadius() { return m_radius; }
	glm::mat4 getTransform() { return m_transform; }

};


class Scene {

private:
	std::vector<Sphere> spheres;

public:
	
	Scene() {
		spheres = std::vector<Sphere>();
	}


	void addSphere(const Sphere &s) { spheres.push_back(s); }

	friend class Renderer;

};