/******************************************************************************
 * Copyright 1998-2018 NVIDIA Corp. All Rights Reserved.
 *****************************************************************************/

#include "renderer.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "manipulator.h"
#include "vk_context.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

 //--------------------------------------------------------------------------------------------------
 // Uniform buffer to store all matrices
 //
struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 modelIT;
	// #VKRay
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

//--------------------------------------------------------------------------------------------------
// Convenient function to load the shaders (SPIR-V)
//
std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	auto              fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

//--------------------------------------------------------------------------------------------------
// Vertex binding description used in the graphic pipeline
//
auto Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

//--------------------------------------------------------------------------------------------------
// Vertex attributes description used in the graphic pipeline
//
auto Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, nrm);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, color);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32_SINT;
	attributeDescriptions[4].offset = offsetof(Vertex, matID);

	return attributeDescriptions;
}

//--------------------------------------------------------------------------------------------------
// Called at each frame to update the camera matrix
//
void Renderer::updateUniformBuffer()
{
	UniformBufferObject ubo = {};
	ubo.model = glm::mat4(1);
	ubo.modelIT = glm::inverseTranspose(ubo.model);

	ubo.view = CameraManip.getMatrix();

	const float aspectRatio = m_framebufferSize.width / static_cast<float>(m_framebufferSize.height);
	ubo.proj = glm::perspective(glm::radians(65.0f), aspectRatio, 0.1f, 1000.0f);
	ubo.proj[1][1] *= -1;  // Inverting Y for Vulkan

	// #VKRay
	ubo.viewInverse = glm::inverse(ubo.view);
	ubo.projInverse = glm::inverse(ubo.proj);

	void* data;
	vkMapMemory(VkCtx.getDevice(), m_uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(VkCtx.getDevice(), m_uniformBufferMemory);
}


//--------------------------------------------------------------------------------------------------
// Loading the OBJ file and setting up all buffers
//
void Renderer::loadModel(const std::string& filename)
{
	ObjLoader<Vertex> loader;
	loader.loadModel(filename);

	m_nbIndices = static_cast<uint32_t>(loader.m_indices.size());
	m_nbVertices = static_cast<uint32_t>(loader.m_vertices.size());
	createVertexBuffer(loader.m_vertices);
	createIndexBuffer(loader.m_indices);
	createMaterialBuffer(loader.m_materials);
	createTextureImages(loader.m_textures);
}

//--------------------------------------------------------------------------------------------------
// Create procedural geometry
//
void Renderer::createProcGeometry()
{

	m_AABB = std::vector<AABB>{ AABB{glm::vec3{-0.5, -0.5, -0.5}, glm::vec3{0.5, 0.5, 0.5}} };
	createAABBBuffer(m_AABB);
	
	std::vector<Sphere> spheres = std::vector<Sphere>{ Sphere{glm::vec3{0,1.001,0}, 0.5},
													   Sphere{glm::vec3{0,-99.5,0}, 100}};
	createSphereBuffer(spheres);

}


//--------------------------------------------------------------------------------------------------
// Create a buffer holding sphere centres and radiuses
//
void Renderer::createSphereBuffer(const std::vector<Sphere>& spheres)
{
	{
		VkDeviceSize bufferSize = spheres.size() * sizeof(Sphere);
		VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_sphereBuffer, m_sphereBufferMemory);
		void* data;
		vkMapMemory(VkCtx.getDevice(), m_sphereBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, spheres.data(), bufferSize);
		vkUnmapMemory(VkCtx.getDevice(), m_sphereBufferMemory);
	}
}

void Renderer::createAccumulationBuffer(int width, int height) {
	VkDeviceSize bufferSize = width * height * sizeof(glm::vec4);
	VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_accumulationBuffer, m_accumulationBufferMemory);
	void* data;
	/*std::vector<glm::vec4>buff{ bufferSize, glm::vec4(0,0,0,0) };
	vkMapMemory(VkCtx.getDevice(), m_sphereBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, buff.data(), bufferSize);
	vkUnmapMemory(VkCtx.getDevice(), m_sphereBufferMemory);*/
}

void Renderer::createInvalidationBuffer(int width, int height)
{
	{
		VkDeviceSize bufferSize = width * height * sizeof(int);
		VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_invalidationBuffer, m_invalidationBufferMemory);
		void* data;
		std::vector<int> vals(bufferSize, 0);
		vkMapMemory(VkCtx.getDevice(), m_invalidationBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vals.data(), bufferSize);
		vkUnmapMemory(VkCtx.getDevice(), m_invalidationBufferMemory);
	}
}

void Renderer::clearBuffers(int width, int height) {
	VkDeviceSize bufferSize = width * height * sizeof(int);
	void* data;
	std::vector<int> vals(bufferSize, 0);
	vkMapMemory(VkCtx.getDevice(), m_invalidationBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vals.data(), bufferSize);
	vkUnmapMemory(VkCtx.getDevice(), m_invalidationBufferMemory);
}


//--------------------------------------------------------------------------------------------------
// Create a buffer holding all materials
//
void Renderer::createMaterialBuffer(const std::vector<MatrialObj>& materials)
{
	{
		VkDeviceSize bufferSize = materials.size() * sizeof(MatrialObj);
		VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_matColorBuffer, m_matColorBufferMemory);
		void* data;
		vkMapMemory(VkCtx.getDevice(), m_matColorBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, materials.data(), bufferSize);
		vkUnmapMemory(VkCtx.getDevice(), m_matColorBufferMemory);
	}
}

//--------------------------------------------------------------------------------------------------
// Create a buffer with all vertices
//
void Renderer::createVertexBuffer(const std::vector<Vertex>& vertex)
{
	VkDeviceSize bufferSize = sizeof(vertex[0]) * vertex.size();

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VkCtx.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertex.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(VkCtx.getDevice(), stagingBufferMemory);

	VkCtx.createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

	VkCtx.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(VkCtx.getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), stagingBufferMemory, nullptr);
}


void Renderer::createAABBBuffer(const std::vector<AABB>& vertex)
{
	VkDeviceSize bufferSize = sizeof(vertex[0]) * vertex.size();

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VkCtx.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertex.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(VkCtx.getDevice(), stagingBufferMemory);

	VkCtx.createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_procBuffer, m_procBufferMemory);

	VkCtx.copyBuffer(stagingBuffer, m_procBuffer, bufferSize);

	vkDestroyBuffer(VkCtx.getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), stagingBufferMemory, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Create a buffer of the triangle indices
//
void Renderer::createIndexBuffer(const std::vector<uint32_t>& indices)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VkCtx.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(VkCtx.getDevice(), stagingBufferMemory);

	VkCtx.createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	VkCtx.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(VkCtx.getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), stagingBufferMemory, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Creating the uniform buffer holding the matrices
//
void Renderer::createUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_uniformBuffer, m_uniformBufferMemory);
}

//--------------------------------------------------------------------------------------------------
// Creating all textures and samplers
//
void Renderer::createTextureImages(const std::vector<std::string>& textures)
{
	// If no textures are present, create a dummy one to accommodate the pipeline layout
	if (textures.empty())
	{
		int          texWidth = 1, texHeight = 1;
		int          texChannels = 4;
		glm::u8vec4* color = new glm::u8vec4(255, 0, 255, 255);
		stbi_uc* pixels = reinterpret_cast<stbi_uc*>(color);

		VkImage        textureImage;
		VkDeviceMemory textureImageMemory;
		VkCtx.createTextureImage(pixels, texWidth, texHeight, texChannels, textureImage,
			textureImageMemory);
		m_textureImage.push_back(textureImage);
		m_textureImageMemory.push_back(textureImageMemory);
		m_textureImageView.push_back(
			VkCtx.createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT));
		m_textureSampler.push_back(createTextureSampler());
		return;
	}


	for (const auto& texture : textures)
	{
		std::stringstream o;
		int               texWidth, texHeight, texChannels;
		o << "../media/textures/" << texture;

		stbi_uc* pixels =
			stbi_load(o.str().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels)
		{
			texWidth = texHeight = 1;
			texChannels = 4;
			glm::u8vec4* color = new glm::u8vec4(255, 0, 255, 255);
			pixels = reinterpret_cast<stbi_uc*>(color);
		}

		VkImage        textureImage;
		VkDeviceMemory textureImageMemory;
		VkCtx.createTextureImage(pixels, texWidth, texHeight, texChannels, textureImage,
			textureImageMemory);
		m_textureImage.push_back(textureImage);
		m_textureImageMemory.push_back(textureImageMemory);
		stbi_image_free(pixels);

		m_textureImageView.push_back(
			VkCtx.createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT));
		m_textureSampler.push_back(createTextureSampler());
	}
}

//--------------------------------------------------------------------------------------------------
// Return standard sampler
//
VkSampler Renderer::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VkSampler textureSampler;
	if (vkCreateSampler(VkCtx.getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	return textureSampler;
}


void Renderer::destroyResources()
{

	vkDestroyDescriptorSetLayout(VkCtx.getDevice(), m_descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(VkCtx.getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(VkCtx.getDevice(), m_graphicsPipeline, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_vertexBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_procBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_procBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_indexBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_indexBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_uniformBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_uniformBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_matColorBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_matColorBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_sphereBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_sphereBufferMemory, nullptr);


	vkDestroyBuffer(VkCtx.getDevice(), m_accumulationBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_accumulationBufferMemory, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_invalidationBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_invalidationBufferMemory, nullptr);

	for (size_t i = 0; i < m_textureImage.size(); i++)
	{
		vkDestroySampler(VkCtx.getDevice(), m_textureSampler[i], nullptr);
		vkDestroyImageView(VkCtx.getDevice(), m_textureImageView[i], nullptr);
		vkDestroyImage(VkCtx.getDevice(), m_textureImage[i], nullptr);
		vkFreeMemory(VkCtx.getDevice(), m_textureImageMemory[i], nullptr);
	}

	// #VKRay
	destroyAccelerationStructure(m_topLevelAS);

	for (auto& as : m_bottomLevelAS)
		destroyAccelerationStructure(as);

	vkDestroyDescriptorSetLayout(VkCtx.getDevice(), m_rtDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(VkCtx.getDevice(), m_rtDescriptorPool, nullptr);

	vkDestroyPipelineLayout(VkCtx.getDevice(), m_rtPipelineLayout, nullptr);
	vkDestroyPipeline(VkCtx.getDevice(), m_rtPipeline, nullptr);

	vkDestroyBuffer(VkCtx.getDevice(), m_shaderBindingTableBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), m_shaderBindingTableMem, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Initialize Vulkan ray tracing
// #VKRay
void Renderer::initRayTracing()
{
	// Query the values of shaderHeaderSize and maxRecursionDepth in current implementation
	m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	m_raytracingProperties.pNext = nullptr;
	m_raytracingProperties.maxRecursionDepth = 0;
	m_raytracingProperties.shaderGroupHandleSize = 0;
	VkPhysicalDeviceProperties2 props;
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &m_raytracingProperties;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(VkCtx.getPhysicalDevice(), &props);
}

//--------------------------------------------------------------------------------------------------
// Create the instances from the scene data
// #VKRay
void Renderer::createGeometryInstances()
{
	// The importer always imports the geometry as a single instance, without a
	// transform. Using a more complex importer, this should be adapted.
	glm::mat4x4 mat1 = glm::mat4x4(	1, 0, 0, 0, 
									0, 1, 0, 0, 
									0, 0, 1, 0, 
									0, 0, 0, 1);
	glm::mat4x4 mat2 = glm::mat4x4(	1, 0, 0, 0, 
									0, 1, 0, 0, 
									0, 0, 1, 0, 
									0, 0, 0, 1);


	m_geometryInstances.push_back(
		{ m_vertexBuffer, m_nbVertices, 0, m_indexBuffer, m_nbIndices, 0, false, mat1 });

	m_geometryInstances.push_back({ m_procBuffer, NULL, 0, NULL, NULL, 0, true, mat2 });
}


//--------------------------------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS #VKRay
Renderer::AccelerationStructure Renderer::createBottomLevelAS(
	VkCommandBuffer               commandBuffer,
	std::vector<GeometryInstance> vVertexBuffers)
{
	nv_helpers_vk::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	for (const auto& buffer : vVertexBuffers)
	{
		if (!buffer.isAABB)
		{

			if (buffer.indexBuffer == VK_NULL_HANDLE)
			{
				// No indices
				bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset, buffer.vertexCount,
					sizeof(Vertex), VK_NULL_HANDLE, 0);
			}
			else
			{
				// Indexed geometry
				bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset, buffer.vertexCount,
					sizeof(Vertex), buffer.indexBuffer, buffer.indexOffset,
					buffer.indexCount, VK_NULL_HANDLE, 0);
			}
		}
		else
		{
			bottomLevelAS.AddProcBuffer(buffer.vertexBuffer, 0);
		}
	}


	AccelerationStructure buffers;

	// Once the overall size of the geometry is known, we can create the handle
	// for the acceleration structure
	buffers.structure = bottomLevelAS.CreateAccelerationStructure(VkCtx.getDevice(), VK_FALSE);

	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	VkDeviceSize scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	VkDeviceSize resultSizeInBytes = 0;
	bottomLevelAS.ComputeASBufferSizes(VkCtx.getDevice(), buffers.structure, &scratchSizeInBytes,
		&resultSizeInBytes);

	// Once the sizes are obtained, the application is responsible for allocating
	// the necessary buffers. Since the entire generation will be done on the GPU,
	// we can directly allocate those in device local mem
	nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(), scratchSizeInBytes,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &buffers.scratchBuffer,
		&buffers.scratchMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(), resultSizeInBytes,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &buffers.resultBuffer,
		&buffers.resultMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	bottomLevelAS.Generate(VkCtx.getDevice(), commandBuffer, buffers.structure, buffers.scratchBuffer,
		0, buffers.resultBuffer, buffers.resultMem, false, VK_NULL_HANDLE);

	return buffers;
}

//--------------------------------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself #VKRay
void Renderer::createTopLevelAS(
	VkCommandBuffer commandBuffer,
	const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances,
	// pair of bottom level AS and matrix of the instance
	const std::vector<int>& hitGroups,
	VkBool32 updateOnly)
{
	if (!updateOnly)
	{
		// Gather all the instances into the builder helper
		for (size_t i = 0; i < instances.size(); i++)
		{
			// For each instance we set its instance index to its index i in the
			// instance vector, and set its hit group index to 2*i. The hit group
			// index defines which entry of the shader binding table will contain the
			// hit group to be executed when hitting this instance. We set this index
			// to i due to the use of 1 type of rays in the scene: the camera rays
			m_topLevelASGenerator.AddInstance(instances[i].first, instances[i].second,
				static_cast<uint32_t>(i), static_cast<uint32_t>(hitGroups[i]));
		}

		// Once all instances have been added, we can create the handle for the TLAS
		m_topLevelAS.structure =
			m_topLevelASGenerator.CreateAccelerationStructure(VkCtx.getDevice(), VK_TRUE);

		// As for the bottom-level AS, the building the AS requires some scratch
		// space to store temporary data in addition to the actual AS. In the case
		// of the top-level AS, the instance descriptors also need to be stored in
		// GPU memory. This call outputs the memory requirements for each (scratch,
		// results, instance descriptors) so that the application can allocate the
		// corresponding memory
		VkDeviceSize scratchSizeInBytes, resultSizeInBytes, instanceDescsSizeInBytes;
		m_topLevelASGenerator.ComputeASBufferSizes(VkCtx.getDevice(), m_topLevelAS.structure,
			&scratchSizeInBytes, &resultSizeInBytes,
			&instanceDescsSizeInBytes);

		// Create the scratch and result buffers. Since the build is all done on
		// GPU, those can be allocated in device local memory
		nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(), scratchSizeInBytes,
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &m_topLevelAS.scratchBuffer,
			&m_topLevelAS.scratchMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(), resultSizeInBytes,
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &m_topLevelAS.resultBuffer,
			&m_topLevelAS.resultMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// The buffer describing the instances: ID, shader binding information,
		// matrices ... Those will be copied into the buffer by the helper through
		// mapping, so the buffer has to be allocated in host visible memory.

		nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(),
			instanceDescsSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
			&m_topLevelAS.instancesBuffer, &m_topLevelAS.instancesMem,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place. Build the acceleration structure. Note that this call
	// integrates a barrier on the generated AS, so that it can be used to compute
	// a top-level AS right after this method.
	m_topLevelASGenerator.Generate(VkCtx.getDevice(), commandBuffer, m_topLevelAS.structure,
		m_topLevelAS.scratchBuffer, 0, m_topLevelAS.resultBuffer,
		m_topLevelAS.resultMem, m_topLevelAS.instancesBuffer,
		m_topLevelAS.instancesMem, updateOnly,
		updateOnly ? m_topLevelAS.structure : VK_NULL_HANDLE);
}


//--------------------------------------------------------------------------------------------------
// Create the bottom-level and top-level acceleration structures
// #VKRay
void Renderer::createAccelerationStructures()
{

	// Create a one-time command buffer in which the AS build commands will be
	// issued
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = VkCtx.getCommandPool()[VkCtx.getFrameIndex()];
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkResult        code =
		vkAllocateCommandBuffers(VkCtx.getDevice(), &commandBufferAllocateInfo, &commandBuffer);
	if (code != VK_SUCCESS)
	{
		throw std::logic_error("rt vkAllocateCommandBuffers failed");
	}

	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	// For each geometric object, we compute the corresponding bottom-level
	// acceleration structure (BLAS)
	m_bottomLevelAS.resize(m_geometryInstances.size());

	std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>> instances;

	for (size_t i = 0; i < m_geometryInstances.size(); i++)
	{
		m_bottomLevelAS[i] = createBottomLevelAS(
			commandBuffer, {
							   {m_geometryInstances[i].vertexBuffer, m_geometryInstances[i].vertexCount,
								m_geometryInstances[i].vertexOffset, m_geometryInstances[i].indexBuffer,
								m_geometryInstances[i].indexCount, m_geometryInstances[i].indexOffset,
								m_geometryInstances[i].isAABB},
			});
		instances.push_back({ m_bottomLevelAS[i].structure, m_geometryInstances[i].transform });
	}

	//these are the various geometric instances
	instances[0].second = glm::translate(instances[0].second, glm::vec3(0, 30, 0));
	instances[0].second = glm::scale(instances[0].second, glm::vec3(0.2, 0.2, 0.2));
	instances[1].second = glm::translate(instances[1].second, glm::vec3(0, 1.001, 0));

	//put 2 more spheres
	instances.push_back({ m_bottomLevelAS[1].structure, m_geometryInstances[1].transform });
	//instances.push_back({ m_bottomLevelAS[1].structure, m_geometryInstances[1].transform });

	instances[2].second = glm::translate(instances[2].second, glm::vec3(0, -99.5, 0));
	instances[2].second = glm::scale(instances[2].second, glm::vec3(200, 200, 200));
	//instances[3].second = glm::translate(instances[3].second, glm::vec3(1.5, 1, 0));

	std::vector<int> hitGroups{ 0,1,1 };

	// Create the top-level AS from the previously computed BLAS
	createTopLevelAS(commandBuffer, instances, hitGroups, VK_FALSE);

	// End the command buffer and submit it
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(VkCtx.getQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VkCtx.getQueue());
	vkFreeCommandBuffers(VkCtx.getDevice(), VkCtx.getCommandPool()[VkCtx.getFrameIndex()], 1,
		&commandBuffer);
}

//--------------------------------------------------------------------------------------------------
// Destroys an acceleration structure and all the resources associated to it
void Renderer::destroyAccelerationStructure(const AccelerationStructure& as)
{
	vkDestroyBuffer(VkCtx.getDevice(), as.scratchBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), as.scratchMem, nullptr);
	vkDestroyBuffer(VkCtx.getDevice(), as.resultBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), as.resultMem, nullptr);
	vkDestroyBuffer(VkCtx.getDevice(), as.instancesBuffer, nullptr);
	vkFreeMemory(VkCtx.getDevice(), as.instancesMem, nullptr);
	vkDestroyAccelerationStructureNV(VkCtx.getDevice(), as.structure, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Create the descriptor set used by the raytracing shaders: note that all
// shaders will access the same descriptor set, and therefore the set needs to
// contain all the resources used by the shaders. For example, it will contain
// all the textures used in the scene.
void Renderer::createRaytracingDescriptorSet()
{
	// We will bind the vertex and index buffers, so we first add a barrier on
	// those buffers to make sure their data is actually present on the GPU
	VkBufferMemoryBarrier bmb = {};
	bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bmb.pNext = nullptr;
	bmb.srcAccessMask = 0;
	bmb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bmb.offset = 0;
	bmb.size = VK_WHOLE_SIZE;

	VkCommandBuffer commandBuffer = VkCtx.beginSingleTimeCommands();

	bmb.buffer = m_vertexBuffer;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bmb, 0, nullptr);
	bmb.buffer = m_indexBuffer;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bmb, 0, nullptr);
	VkCtx.endSingleTimeCommands(commandBuffer);

	// Add the bindings to the resources
	// Top-level acceleration structure, usable by both the ray generation and the
	// closest hit (to shoot shadow rays)
	m_rtDSG.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
		VK_SHADER_STAGE_RAYGEN_BIT_NV);
	// Raytracing output
	m_rtDSG.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV);
	// Camera information
	m_rtDSG.AddBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);
	// Scene data
	// Vertex buffer
	m_rtDSG.AddBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	// Index buffer
	m_rtDSG.AddBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	// Material buffer
	m_rtDSG.AddBinding(5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	// Textures
	m_rtDSG.AddBinding(6, static_cast<uint32_t>(m_textureSampler.size()),
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	// Spheres
	m_rtDSG.AddBinding(7, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_INTERSECTION_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	// Accumulation buffer
	m_rtDSG.AddBinding(8, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);
	// Invalidation buffer
	m_rtDSG.AddBinding(9, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);

	// Create the descriptor pool and layout
	m_rtDescriptorPool = m_rtDSG.GeneratePool(VkCtx.getDevice());
	m_rtDescriptorSetLayout = m_rtDSG.GenerateLayout(VkCtx.getDevice());

	// Generate the descriptor set
	m_rtDescriptorSet =
		m_rtDSG.GenerateSet(VkCtx.getDevice(), m_rtDescriptorPool, m_rtDescriptorSetLayout);

	// Bind the actual resources into the descriptor set
	// Top-level acceleration structure
	VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo;
	descriptorAccelerationStructureInfo.sType =
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptorAccelerationStructureInfo.pNext = nullptr;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &m_topLevelAS.structure;

	m_rtDSG.Bind(m_rtDescriptorSet, 0, { descriptorAccelerationStructureInfo });

	// Camera matrices
	VkDescriptorBufferInfo camInfo = {};
	camInfo.buffer = m_uniformBuffer;
	camInfo.offset = 0;
	camInfo.range = sizeof(UniformBufferObject);

	m_rtDSG.Bind(m_rtDescriptorSet, 2, { camInfo });

	// Vertex buffer
	VkDescriptorBufferInfo vertexInfo = {};
	vertexInfo.buffer = m_vertexBuffer;
	vertexInfo.offset = 0;
	vertexInfo.range = VK_WHOLE_SIZE;

	m_rtDSG.Bind(m_rtDescriptorSet, 3, { vertexInfo });

	// Index buffer
	VkDescriptorBufferInfo indexInfo = {};
	indexInfo.buffer = m_indexBuffer;
	indexInfo.offset = 0;
	indexInfo.range = VK_WHOLE_SIZE;

	m_rtDSG.Bind(m_rtDescriptorSet, 4, { indexInfo });

	// Material buffer
	VkDescriptorBufferInfo materialInfo = {};
	materialInfo.buffer = m_matColorBuffer;
	materialInfo.offset = 0;
	materialInfo.range = VK_WHOLE_SIZE;
	m_rtDSG.Bind(m_rtDescriptorSet, 5, { materialInfo });

	// Spheres buffer
	VkDescriptorBufferInfo sphereInfo = {};
	sphereInfo.buffer = m_sphereBuffer;
	sphereInfo.offset = 0;
	sphereInfo.range = VK_WHOLE_SIZE;
	m_rtDSG.Bind(m_rtDescriptorSet, 7, { sphereInfo });
	
	// accumulation buffer
	VkDescriptorBufferInfo accumulationInfo = {};
	accumulationInfo.buffer = m_accumulationBuffer;
	accumulationInfo.offset = 0;
	accumulationInfo.range = VK_WHOLE_SIZE;
	m_rtDSG.Bind(m_rtDescriptorSet, 8, { accumulationInfo });

	// invalidation buffer
	VkDescriptorBufferInfo invalidationInfo = {};
	invalidationInfo.buffer = m_invalidationBuffer;
	invalidationInfo.offset = 0;
	invalidationInfo.range = VK_WHOLE_SIZE;
	m_rtDSG.Bind(m_rtDescriptorSet, 9, { invalidationInfo });

	// Textures
	std::vector<VkDescriptorImageInfo> imageInfos;
	for (size_t i = 0; i < m_textureSampler.size(); ++i)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_textureImageView[i];
		imageInfo.sampler = m_textureSampler[i];
		imageInfos.push_back(imageInfo);
	}
	if (!m_textureSampler.empty())
	{
		m_rtDSG.Bind(m_rtDescriptorSet, 6, imageInfos);
	}

	// Copy the bound resource handles into the descriptor set
	m_rtDSG.UpdateSetContents(VkCtx.getDevice(), m_rtDescriptorSet);
}

void Renderer::updateRaytracingRenderTarget(VkImageView target)
{
	// Output buffer
	VkDescriptorImageInfo descriptorOutputImageInfo;
	descriptorOutputImageInfo.sampler = nullptr;
	descriptorOutputImageInfo.imageView = target;
	descriptorOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	m_rtDSG.Bind(m_rtDescriptorSet, 1, { descriptorOutputImageInfo });
	// Copy the bound resource handles into the descriptor set
	m_rtDSG.UpdateSetContents(VkCtx.getDevice(), m_rtDescriptorSet);
}

//--------------------------------------------------------------------------------------------------
// Create the raytracing pipeline, containing the handles and data for each
// raytracing shader For each shader or hit group we retain its index, so that
// they can be bound to the geometry in the shader binding table.
void Renderer::createRaytracingPipeline()
{

	std::string shaderDir = "../shaders/";

	nv_helpers_vk::RayTracingPipelineGenerator pipelineGen;
	// We use only one ray generation, that will implement the camera model
	VkShaderModule rayGenModule = VkCtx.createShaderModule(readFile(shaderDir + "raygen.rgen.spv"));
	m_rayGenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);
	// The first miss shader is used to look-up the environment in case the rays
	// from the camera miss the geometry
	VkShaderModule missModule = VkCtx.createShaderModule(readFile(shaderDir + "miss.rmiss.spv"));
	m_missIndex = pipelineGen.AddMissShaderStage(missModule);

	// The first hit group defines the shaders invoked when a ray shot from the
	// camera hit the geometry. In this case we only specify the closest hit
	// shader, and rely on the build-in triangle intersection and pass-through
	// any-hit shader. However, explicit intersection and any hit shaders could be
	// added as well.
	m_hitGroupIndex = pipelineGen.StartHitGroup();

	VkShaderModule closestHitModule =
		VkCtx.createShaderModule(readFile(shaderDir + "closesthit.rchit.spv"));
	pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

	pipelineGen.EndHitGroup();

	m_hitGroupIndexSphere = pipelineGen.StartHitGroup();

	VkShaderModule closestHitSphereModule = VkCtx.createShaderModule(readFile(shaderDir + "closesthitSphere.rchit.spv"));
	pipelineGen.AddHitShaderStage(closestHitSphereModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

	VkShaderModule intersectModule = VkCtx.createShaderModule(readFile(shaderDir + "intersect.rint.spv"));
	pipelineGen.AddHitShaderStage(intersectModule, VK_SHADER_STAGE_INTERSECTION_BIT_NV);

	pipelineGen.EndHitGroup();

	// The raytracing process now can only shoot rays from the camera, hence a
	// recursion level of 1. This number should be kept as low as possible for
	// performance reasons. Even recursive raytracing should be flattened into a
	// loop in the ray generation to avoid deep recursion.
	pipelineGen.SetMaxRecursionDepth(1);

	// Generate the pipeline
	pipelineGen.Generate(VkCtx.getDevice(), m_rtDescriptorSetLayout, &m_rtPipeline,
		&m_rtPipelineLayout);

	vkDestroyShaderModule(VkCtx.getDevice(), rayGenModule, nullptr);
	vkDestroyShaderModule(VkCtx.getDevice(), missModule, nullptr);
	vkDestroyShaderModule(VkCtx.getDevice(), closestHitModule, nullptr);
	vkDestroyShaderModule(VkCtx.getDevice(), closestHitSphereModule, nullptr);
	vkDestroyShaderModule(VkCtx.getDevice(), intersectModule, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Create the shader binding table, associating the geometry to the indices of the shaders in the
// ray tracing pipeline
void Renderer::createShaderBindingTable()
{
	// Add the entry point, the ray generation program
	m_sbtGen.AddRayGenerationProgram(m_rayGenIndex, {});
	// Add the miss shader for the camera rays
	m_sbtGen.AddMissProgram(m_missIndex, {});

	// For each instance, we will have 1 hit group for the camera rays.
	// When setting the instances in the top-level acceleration structure we indicated the index
	// of the hit group in the shader binding table that will be invoked.

	// Add the hit group defining the behavior upon hitting a surface with a camera ray
	m_sbtGen.AddHitGroup(m_hitGroupIndex, {});

	m_sbtGen.AddHitGroup(m_hitGroupIndexSphere, {});

	// Compute the required size for the SBT
	VkDeviceSize shaderBindingTableSize = m_sbtGen.ComputeSBTSize(m_raytracingProperties);

	// Allocate mappable memory to store the SBT
	nv_helpers_vk::createBuffer(VkCtx.getPhysicalDevice(), VkCtx.getDevice(), shaderBindingTableSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &m_shaderBindingTableBuffer,
		&m_shaderBindingTableMem, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// Generate the SBT using mapping. For further performance a staging buffer should be used, so
	// that the SBT is guaranteed to reside on GPU memory without overheads.
	m_sbtGen.Generate(VkCtx.getDevice(), m_rtPipeline, m_shaderBindingTableBuffer,
		m_shaderBindingTableMem);
}