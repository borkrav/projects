// ImGui - standalone example application for Glfw + Vulkan, using programmable
// pipeline If you are new to ImGui, see examples/README.txt and documentation
// at the top of imgui.cpp.

#include <array>
#include <ctime>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "lodepng.h"

#include "renderer.h"
#include "manipulator.h"
#include "vk_context.h"

static bool g_ResizeWanted = false;
static int  g_winWidth = 1650, g_winHeight = 1200;

//////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
//////////////////////////////////////////////////////////////////////////

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

static void on_errorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void on_resizeCallback(GLFWwindow* window, int w, int h)
{
	UNUSED(window);
	CameraManip.setWindowSize(w, h);
	g_ResizeWanted = true;
	g_winWidth = w;
	g_winHeight = h;
}

//-----------------------------------------------------------------------------
// Trapping the keyboard
//
static void on_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	if (action == GLFW_RELEASE)  // Don't deal with key up
	{
		return;
	}

	if (key == GLFW_KEY_ESCAPE || key == 'Q')
	{
		glfwSetWindowShouldClose(window, 1);
	}
}

static void on_mouseMoveCallback(GLFWwindow* window, double mouseX, double mouseY)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantCaptureMouse)
	{
		return;
	}

	using nv_helpers_vk::Manipulator;
	Manipulator::Inputs inputs;
	inputs.lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	inputs.mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
	inputs.rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	if (!inputs.lmb && !inputs.rmb && !inputs.mmb)
	{
		return;  // no mouse button pressed
	}

	inputs.ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	inputs.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
	inputs.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

	CameraManip.mouseMove(static_cast<int>(mouseX), static_cast<int>(mouseY), inputs);
}

static void on_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	CameraManip.setMousePosition(static_cast<int>(xpos), static_cast<int>(ypos));
}

static void on_scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}
	CameraManip.wheel(static_cast<int>(yoffset));
}

int main(int argc, char** argv)
{
	UNUSED(argc);
	UNUSED(argv);

	// Setup window
	glfwSetErrorCallback(on_errorCallback);
	if (!glfwInit())
	{
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight,
		"vk-rtx-pathtrace", nullptr, nullptr);

	glfwSetWindowPos(window, 10, 100);

	glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
	glfwSetScrollCallback(window, on_scrollCallback);
	glfwSetKeyCallback(window, on_keyCallback);
	glfwSetCursorPosCallback(window, on_mouseMoveCallback);
	glfwSetMouseButtonCallback(window, on_mouseButtonCallback);
	// glfwSetWindowSizeCallback(window, on_resizeCallback);
	glfwSetFramebufferSizeCallback(window, on_resizeCallback);

	// Setup camera
	CameraManip.setWindowSize(g_winWidth, g_winHeight);
	CameraManip.setLookat(glm::vec3(0, 1, 3.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	//CameraManip.setLookat(glm::vec3(0, 25, -100), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		printf("GLFW: Vulkan Not Supported\n");
		return 1;
	}

	// #VKRay: Activate the ray tracing extension
	VkCtx.setupVulkan(window, true,
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_NV_RAY_TRACING_EXTENSION_NAME,
		 VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME });

	// Setup Dear ImGui binding
	if (1 == 1)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		ImGui_ImplGlfwVulkan_Init_Data init_data = {};
		init_data.allocator = VkCtx.getAllocator();
		init_data.gpu = VkCtx.getPhysicalDevice();
		init_data.device = VkCtx.getDevice();
		init_data.render_pass = VkCtx.getRenderPass();
		init_data.pipeline_cache = VkCtx.getPipelineCache();
		init_data.descriptor_pool = VkCtx.getDescriptorPool();
		init_data.check_vk_result = check_vk_result;

		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard
		// Controls
		ImGui_ImplGlfwVulkan_Init(window, false, &init_data);

		// Setup style
		ImGui::StyleColorsDark();

	}

	// Upload Fonts
	if (1 == 1)
	{
		uint32_t g_FrameIndex = VkCtx.getFrameIndex();
		VkResult err;
		err = vkResetCommandPool(VkCtx.getDevice(), VkCtx.getCommandPool()[g_FrameIndex], 0);
		check_vk_result(err);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(VkCtx.getCommandBuffer()[g_FrameIndex], &begin_info);
		check_vk_result(err);

		ImGui_ImplGlfwVulkan_CreateFontsTexture(VkCtx.getCommandBuffer()[g_FrameIndex]);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &VkCtx.getCommandBuffer()[g_FrameIndex];
		err = vkEndCommandBuffer(VkCtx.getCommandBuffer()[g_FrameIndex]);
		check_vk_result(err);
		err = vkQueueSubmit(VkCtx.getQueue(), 1, &end_info, VK_NULL_HANDLE);
		check_vk_result(err);

		err = vkDeviceWaitIdle(VkCtx.getDevice());
		check_vk_result(err);
		ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
	}

	Renderer helloVulkan;
	helloVulkan.loadModel("../media/scenes/cube_multi.obj");
	helloVulkan.createProcGeometry();
	helloVulkan.m_framebufferSize = { static_cast<uint32_t>(g_winWidth), static_cast<uint32_t>(g_winHeight) };
	helloVulkan.createUniformBuffer();


	// #VKRay
	helloVulkan.initRayTracing();
	helloVulkan.createGeometryInstances();
	helloVulkan.createAccelerationStructures();
	helloVulkan.createRaytracingDescriptorSet();
	helloVulkan.createRaytracingPipeline();
	helloVulkan.createShaderBindingTable();


	ImVec4 clear_color = ImVec4(1, 1, 1, 1.00f);

	int backBufferFrames = IMGUI_VK_QUEUED_FRAMES;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
		// tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to
		// your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
		// data to your main application. Generally you may always pass all inputs
		// to dear imgui, and hide them from your application based on those two
		// flags.
		glfwPollEvents();

		if (g_ResizeWanted)
		{
			VkCtx.resizeVulkan(g_winWidth, g_winHeight);
			helloVulkan.m_framebufferSize = { static_cast<uint32_t>(g_winWidth), static_cast<uint32_t>(g_winHeight) };
			backBufferFrames = IMGUI_VK_QUEUED_FRAMES;

		}
		g_ResizeWanted = false;

		helloVulkan.updateUniformBuffer();

		// 1. Show a simple window.
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets
		// automatically appears in a window called "Debug".
		if (1 == 1)
		{

			
			ImGui_ImplGlfwVulkan_NewFrame();

			ImGui::Begin("Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize);

			//ImGui::ColorEdit3("clear color",
			//	reinterpret_cast<float*>(
			//		&clear_color));  // Edit 3 floats representing a color

			//ImGui::Checkbox("Raster mode",
			//	&use_raster_render);  // Switch between raster and ray tracing


			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
				1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			{
				static int item = 1;
				if (ImGui::Combo("Up Vector", &item, "X\0Y\0Z\0\0"))
				{
					glm::vec3 pos, eye, up;
					CameraManip.getLookat(pos, eye, up);
					up = glm::vec3(item == 0, item == 1, item == 2);
					CameraManip.setLookat(pos, eye, up);
				}
			}


			//screenshot button
			if (ImGui::Button("Screenshot")) {

				//From tutorial code:
				// https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp

				// surface format for me is: VK_FORMAT_B8G8R8A8_UNORM

				//Linear tiling -> Regular 2d matrix layout (row major?)
				//Optimal tiling -> GPU uses it's own memory-friendly layout

				//My RTX2080 actually doesn't support blitting to linear images -> apperantly linear images are generally useless
				//the general work flow seems to be:
				//TILING OPTIMAL -> copy -> TILING LINEAR -> map -> memcopy

				//I'll omit blitting code, since it doesn't seem to be supported in practice for this use case


				VkImage srcImage = VkCtx.getCurrentBackBuffer();

				VkImage dstImage;
				VkDeviceMemory dstImageMem;

				//create linear-tiled image on GPU
				VkCtx.createImage(g_winWidth,
									g_winHeight,
									VK_FORMAT_R8G8B8A8_UNORM,
									VK_IMAGE_TILING_LINEAR,
									VK_IMAGE_USAGE_TRANSFER_DST_BIT,
									VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
									dstImage,
									dstImageMem);


				VkCommandBuffer cmdBuff = VkCtx.beginSingleTimeCommands();


				VkImageSubresourceRange subresourceRange;
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = 1;
				subresourceRange.baseArrayLayer = 0;
				subresourceRange.layerCount = 1;

				//transition both images 
				nv_helpers_vk::imageBarrier(cmdBuff, dstImage, subresourceRange, 0,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				nv_helpers_vk::imageBarrier(cmdBuff, srcImage, subresourceRange, VK_ACCESS_MEMORY_READ_BIT,
					VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);




				VkImageCopy imageCopyRegion{};
				imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.srcSubresource.layerCount = 1;
				imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.dstSubresource.layerCount = 1;
				imageCopyRegion.extent.width = g_winWidth;
				imageCopyRegion.extent.height = g_winHeight;
				imageCopyRegion.extent.depth = 1;

				//copy from src to dst
				vkCmdCopyImage(
					cmdBuff,
					srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageCopyRegion);
				



				//transition src back to presentation mode, dst to general
				nv_helpers_vk::imageBarrier(cmdBuff, dstImage, subresourceRange, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_GENERAL);

				nv_helpers_vk::imageBarrier(cmdBuff, srcImage, subresourceRange, VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);



				VkCtx.endSingleTimeCommands(cmdBuff);



				//get layout
				VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
				VkSubresourceLayout subResourceLayout;
				vkGetImageSubresourceLayout(VkCtx.getDevice(), dstImage, &subResource, &subResourceLayout);

				//map image to host memory
				const char* data;
				VkResult map = vkMapMemory(VkCtx.getDevice(), dstImageMem, 0, VK_WHOLE_SIZE, 0, (void**)&data);
				assert(map == VK_SUCCESS);
				data += subResourceLayout.offset;


				// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
				bool colorSwizzle = false;
				// Check if source is BGR 
				// Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes
			
				std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
				colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VkCtx.getSurfaceFormat()) != formatsBGR.end());
				

				//fill the image vector
				std::vector<unsigned char> image;

				uint64_t w = g_winWidth;
				uint64_t h = g_winHeight;


				image.resize(w * h * 4);


				for (uint64_t y = 0; y < h; y++)
				{
					//go through data byte by byte
					unsigned char* row = (unsigned char*) data;
					for (uint64_t x = 0; x < w; x++)
					{
						
						if (colorSwizzle) {
							image[4 * w * y + 4 * x + 0] = *(row + 2);
							image[4 * w * y + 4 * x + 1] = *(row + 1);
							image[4 * w * y + 4 * x + 2] = *(row + 0);
							image[4 * w * y + 4 * x + 3] = 255;
						}

						else {
							image[4 * w * y + 4 * x + 0] = *(row + 0);
							image[4 * w * y + 4 * x + 1] = *(row + 1);
							image[4 * w * y + 4 * x + 2] = *(row + 2);
							image[4 * w * y + 4 * x + 3] = 255;
						}

						//increment by 4 bytes (RGBA)
						row+=4;
					}
					//go to next row
					data += subResourceLayout.rowPitch;
				}


				//create filename, with current date and time
				auto p = std::chrono::system_clock::now();
				time_t t = std::chrono::system_clock::to_time_t(p);
				char str[26];
				ctime_s(str, sizeof str, &t);
				std::string fileName = "../screenshots/" + (std::string)str + ".png";

				//clean up the string
				fileName.erase(std::remove(fileName.begin(), fileName.end(), '\n'), fileName.end());
				std::replace(fileName.begin(), fileName.end(),' ', '-');
				std::replace(fileName.begin(), fileName.end(), ':', '-');

				//encode to PNG
				unsigned error = lodepng::encode(fileName, image.data(), g_winWidth, g_winHeight);

				assert(error == 0);


				//clean up
				vkUnmapMemory(VkCtx.getDevice(), dstImageMem);
				vkFreeMemory(VkCtx.getDevice(), dstImageMem, nullptr);
				vkDestroyImage(VkCtx.getDevice(), dstImage, nullptr);

			}
			
			ImGui::End();
		}


		VkClearValue cv;
		memcpy(&cv.color.float32[0], &clear_color.x, 4 * sizeof(float));
		VkCtx.setClearValue(cv);
		VkCtx.frameBegin();
		VkCommandBuffer cmdBuff = VkCtx.getCommandBuffer()[VkCtx.getFrameIndex()];



		// do the raytracing

		VkClearValue clearColor = { 0.0f, 0.5f, 0.0f, 1.0f };

		VkImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		//for the first couple of frames, the layout transition needs to be from undefined to general
		if (backBufferFrames > 0) {
			nv_helpers_vk::imageBarrier(cmdBuff, VkCtx.getCurrentBackBuffer(), subresourceRange, 0,
				VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);
			backBufferFrames -= 1;
		}

		//for the rest of the frames, it's from presentation to general
		else {
			nv_helpers_vk::imageBarrier(cmdBuff, VkCtx.getCurrentBackBuffer(), subresourceRange, 0,
				VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_LAYOUT_GENERAL);
		}

		helloVulkan.updateRaytracingRenderTarget(VkCtx.getCurrentBackBufferView());

		VkCtx.beginRenderPass();

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, helloVulkan.m_rtPipeline);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
			helloVulkan.m_rtPipelineLayout, 0, 1, &helloVulkan.m_rtDescriptorSet,
			0, nullptr);

		VkDeviceSize rayGenOffset = helloVulkan.m_sbtGen.GetRayGenOffset();
		VkDeviceSize missOffset = helloVulkan.m_sbtGen.GetMissOffset();
		VkDeviceSize missStride = helloVulkan.m_sbtGen.GetMissEntrySize();
		VkDeviceSize hitGroupOffset = helloVulkan.m_sbtGen.GetHitGroupOffset();
		VkDeviceSize hitGroupStride = helloVulkan.m_sbtGen.GetHitGroupEntrySize();

		vkCmdTraceRaysNV(cmdBuff, helloVulkan.m_shaderBindingTableBuffer, rayGenOffset,
			helloVulkan.m_shaderBindingTableBuffer, missOffset, missStride,
			helloVulkan.m_shaderBindingTableBuffer, hitGroupOffset, hitGroupStride,
			VK_NULL_HANDLE, 0, 0, helloVulkan.m_framebufferSize.width,
			helloVulkan.m_framebufferSize.height, 1);

	

		{
			vkCmdNextSubpass(cmdBuff, VK_SUBPASS_CONTENTS_INLINE);
			ImGui_ImplGlfwVulkan_Render(cmdBuff);
			VkCtx.endRenderPass();
		}

		VkCtx.frameEnd();

		VkCtx.framePresent();
	}

	// Cleanup
	VkResult err = vkDeviceWaitIdle(VkCtx.getDevice());
	check_vk_result(err);
	ImGui_ImplGlfwVulkan_Shutdown();
	ImGui::DestroyContext();
	helloVulkan.destroyResources();
	VkCtx.cleanupVulkan();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
