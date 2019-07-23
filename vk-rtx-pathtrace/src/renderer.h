#pragma once
//////////////////////////////////////////////////////////////////////////

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "obj_loader.h"

// #VKRay
#include "nv_helpers_vk/BottomLevelASGenerator.h"
#include "nv_helpers_vk/DescriptorSetGenerator.h"
#include "nv_helpers_vk/RaytracingPipelineGenerator.h"
#include "nv_helpers_vk/ShaderBindingTableGenerator.h"
#include "nv_helpers_vk/TopLevelASGenerator.h"
#include "nv_helpers_vk/VKHelpers.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 color;
	glm::vec2 texCoord;
	int       matID = 0;

	static auto getBindingDescription();
	static auto getAttributeDescriptions();
};

struct AABB
{
	glm::vec3 v1;
	glm::vec3 v2;
};

//--------------------------------------------------------------------------------------------------
//
//
class Renderer
{
public:
	void loadModel(const std::string& filename);
	void createMaterialBuffer(const std::vector<MatrialObj>& materials);
	void createVertexBuffer(const std::vector<Vertex>& vertex);

	void createAABBBuffer(const std::vector<AABB>& vertex);

	void createAccumulationBuffer(int width, int height);
	void createInvalidationBuffer(int width, int height);


	void clearBuffers(int width, int height);

	void      createIndexBuffer(const std::vector<uint32_t>& indices);
	void      createUniformBuffer();
	void      createTextureImages(const std::vector<std::string>& textures);
	VkSampler createTextureSampler();
	void      updateUniformBuffer();

	//AABB
	void createProcGeometry();

	void destroyResources();

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout      m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline            m_graphicsPipeline = VK_NULL_HANDLE;
	VkDescriptorSet       m_descriptorSet;

	uint32_t m_nbIndices;
	uint32_t m_nbVertices;

	//AABB
	std::vector<AABB> m_AABB;

	VkBuffer       m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	VkBuffer       m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	VkBuffer       m_uniformBuffer;
	VkDeviceMemory m_uniformBufferMemory;

	VkBuffer       m_matColorBuffer;
	VkDeviceMemory m_matColorBufferMemory;

	VkBuffer       m_sphereBuffer;
	VkDeviceMemory m_sphereBufferMemory;

	VkBuffer       m_accumulationBuffer;
	VkDeviceMemory m_accumulationBufferMemory;

	VkBuffer       m_invalidationBuffer;
	VkDeviceMemory m_invalidationBufferMemory;

	VkExtent2D m_framebufferSize;

	//Procedural geometry implementation (AABB mode)
	VkBuffer       m_procBuffer;
	VkDeviceMemory m_procBufferMemory;


	std::vector<VkImage>        m_textureImage;
	std::vector<VkDeviceMemory> m_textureImageMemory;
	std::vector<VkImageView>    m_textureImageView;
	std::vector<VkSampler>      m_textureSampler;

	// #VKRay
	void                                   initRayTracing();
	VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties = {};


	struct GeometryInstance
	{
		VkBuffer     vertexBuffer;
		uint32_t     vertexCount;
		VkDeviceSize vertexOffset;
		VkBuffer     indexBuffer;
		uint32_t     indexCount;
		VkDeviceSize indexOffset;
		bool         isAABB;
		glm::mat4x4  transform;
	};

	struct Sphere {
		glm::vec3 centre;
		float radius;
	};

	void createSphereBuffer(const std::vector<Sphere>& spheres);

	void                          createGeometryInstances();
	std::vector<GeometryInstance> m_geometryInstances;


	struct AccelerationStructure
	{
		VkBuffer                  scratchBuffer = VK_NULL_HANDLE;
		VkDeviceMemory            scratchMem = VK_NULL_HANDLE;
		VkBuffer                  resultBuffer = VK_NULL_HANDLE;
		VkDeviceMemory            resultMem = VK_NULL_HANDLE;
		VkBuffer                  instancesBuffer = VK_NULL_HANDLE;
		VkDeviceMemory            instancesMem = VK_NULL_HANDLE;
		VkAccelerationStructureNV structure = VK_NULL_HANDLE;
	};

	AccelerationStructure createBottomLevelAS(VkCommandBuffer               commandBuffer,
		std::vector<GeometryInstance> vVertexBuffers);

	void                  createTopLevelAS(
		VkCommandBuffer                                                       commandBuffer,
		const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances,
		const std::vector<int>& hitGroups,
		/* pair of bottom level AS and matrix of the instance */
		VkBool32 updateOnly);

	void createAccelerationStructures();
	void destroyAccelerationStructure(const AccelerationStructure& as);

	nv_helpers_vk::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructure              m_topLevelAS;
	std::vector<AccelerationStructure> m_bottomLevelAS;


	void                                  createRaytracingDescriptorSet();
	void                                  updateRaytracingRenderTarget(VkImageView target);
	nv_helpers_vk::DescriptorSetGenerator m_rtDSG;
	VkDescriptorPool                      m_rtDescriptorPool;
	VkDescriptorSetLayout                 m_rtDescriptorSetLayout;
	VkDescriptorSet                       m_rtDescriptorSet;

	void             createRaytracingPipeline();
	VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
	VkPipeline       m_rtPipeline = VK_NULL_HANDLE;

	uint32_t m_rayGenIndex;
	uint32_t m_hitGroupIndex;
	uint32_t m_missIndex;

	uint32_t m_hitGroupIndexSphere;

	void                                       createShaderBindingTable();
	nv_helpers_vk::ShaderBindingTableGenerator m_sbtGen;
	VkBuffer                                   m_shaderBindingTableBuffer;
	VkDeviceMemory                             m_shaderBindingTableMem;
};
