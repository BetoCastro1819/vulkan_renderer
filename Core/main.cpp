#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720

#define APP_NAME "Vulkan Renderer"

#define CHECK_VK_RESULT(res, msg) \
	if (res != VK_SUCCESS) { \
		fprintf(stderr, "Error in %s:%d - %s, code %x\n", __FILE__, __LINE__, msg, res); \
		exit(1); \
	}

#define DEBUG_LOG(msg) printf(msg); printf("\n");
#define DEBUG_LOG_ERROR(msg) printf("ERROR: "); printf(msg); printf("\n");

void GLFW_KeyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
	if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

const char* GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "Info";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "Error";
	default:
		DEBUG_LOG_ERROR("Invalid severity code %d", severity);
		exit(1);
	}

	return "No such severity!";
}

const char* GetDebugType(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device address binding";
	default:
		DEBUG_LOG_ERROR("Invalid type code %d", type);
		exit(1);
	}

	return "No such type!";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	DEBUG_LOG("Debug callback: %s\n", callbackData->pMessage);
	DEBUG_LOG("	Severity %s\n", GetDebugSeverityStr(severity));
	DEBUG_LOG("	Type %s\n", GetDebugType(type));
	DEBUG_LOG("	Objects");
	for (uint32_t i = 0; i < callbackData->objectCount; i++)
	{
		DEBUG_LOG("%llx", callbackData->pObjects[i].objectHandle);
	}

	// false = should not be aborted
	return VK_FALSE;
}


int main()
{
	if (!glfwInit())
	{
		DEBUG_LOG_ERROR("Failed to initialize GLFW.");
		return 1;
	}
	DEBUG_LOG("GLFW initialized");

	if (!glfwVulkanSupported())
	{
		DEBUG_LOG_ERROR("Vulkan is not supported.");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, NULL, NULL);
	if (!window)
	{
		DEBUG_LOG_ERROR("Failed to create GLFW window.");
		exit(EXIT_FAILURE);
	}
	DEBUG_LOG("GLFW window created.");

	glfwSetKeyCallback(window, GLFW_KeyCallback);


	// Vulkan setup
	std::vector<const char*> layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> extensions  = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined (_WIN32)
		"VK_KHR_win32_surface",
#endif
#if defined (__linux__)
		"VK_KHR_xcb_surface",
#endif
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = APP_NAME;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = (uint32_t)(layers.size());
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkInstance vulkanInstance = VK_NULL_HANDLE;
	VkResult isntanceResult = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
	CHECK_VK_RESULT(isntanceResult, "Created vulkan instance");
	DEBUG_LOG("Vulkan instance created");

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	
	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.pNext = nullptr;
	messengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	messengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	
	messengerCreateInfo.pfnUserCallback = &DebugCallback;
	messengerCreateInfo.pUserData = nullptr;

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger)
	{
		DEBUG_LOG_ERROR("Failed to find address of vkCreateDebugUtilsMessengerEXT");
		exit(1);
	}

	VkResult debugUtilsResult = vkCreateDebugUtilsMessenger(vulkanInstance, &messengerCreateInfo, nullptr, &debugMessenger);
	CHECK_VK_RESULT(debugUtilsResult, "Debug utils messenger");
	DEBUG_LOG("Debug utils messenger created");

	while (!glfwWindowShouldClose(window))
	{
		// RenderScene();
		glfwPollEvents();
	}

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugUtilsMessenger)
	{
		DEBUG_LOG_ERROR("Failed to find address of vkDestroyDebugUtilsMessenger");
		exit(1);
	}

	DEBUG_LOG("\n");
	DEBUG_LOG("*------- CLOSED APPLICATION -------*");

	vkDestroyDebugUtilsMessenger(vulkanInstance, debugMessenger, nullptr);
	DEBUG_LOG("Debug callback destroyed");

	vkDestroyInstance(vulkanInstance, nullptr);
	DEBUG_LOG("Vulkan instance destroyed");

	glfwTerminate();
	DEBUG_LOG("GLFW terminated");

	return 0;
}