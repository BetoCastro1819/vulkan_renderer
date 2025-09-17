#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <assert.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720

#define APP_NAME "Vulkan Renderer"

#define DEBUG_LOG(msg, ...) printf("[%s] ", __TIME__); printf((msg), ##__VA_ARGS__); printf("\n");
#define DEBUG_LOG_ERROR(msg) printf("[%s] ERROR in %s - Line: %d\n", __TIME__, __FILE__, __LINE__); printf(msg); printf("\n");

struct PhysicalDevice
{
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	std::vector<VkQueueFamilyProperties> familyProperties;
	std::vector<VkBool32> supportsPresent;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	std::vector<VkPresentModeKHR> presentModes;
};

void CheckVkResult( const char* msg, VkResult res)
{
	if (res != VK_SUCCESS)
	{
		fprintf(stderr, "Error in %s:%d - %s, code %x\n", __FILE__, __LINE__, msg, res);
		exit(1);
	}
}

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

void PrintImageUsageFlags(const VkImageUsageFlags& flags)
{
	if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
		printf("Image usage transfer src is supported\n");
	
	if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		printf("Image usage transfer dst is supported\n");
	
	if (flags & VK_IMAGE_USAGE_SAMPLED_BIT)
		printf("Image usage sampled is supported\n");
	
	if (flags & VK_IMAGE_USAGE_STORAGE_BIT)
		printf("Image usage storage is supported\n");
	
	if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		printf("Image usage color attachmetn is supported\n");
	
	if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		printf("Image usage depth setncil attachment is supported\n");
	
	if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		printf("Image usage transient attachment is supported\n");
	
	if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		printf("Image usage input attachment is supported\n");
	
	if (flags & VK_IMAGE_USAGE_HOST_TRANSFER_BIT)
		printf("Image usage host transfer is supported\n");
}

void PrintMemoryProperty(const VkMemoryPropertyFlags& flags)
{
 	if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		printf("Memory property device local\n");

	if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		printf("Memory property host visible\n");

	if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		printf("Memory property host coherent\n");

	if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		printf("Memory property host cached\n");

	if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
		printf("Memory property lazily allocated\n");

	if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
		printf("Memory property protected\n");

	if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
		printf("Memory property device coherent amd\n");

	if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
		printf("Memory property device uncached amd\n");

	if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
		printf("Memory property rdma capable nv\n");
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

	VkInstance vulkanInstance = VK_NULL_HANDLE;
	CheckVkResult("Creating vulkan instance", vkCreateInstance(&createInfo, nullptr, &vulkanInstance));
	DEBUG_LOG("Vulkan instance created");

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger)
	{
		DEBUG_LOG_ERROR("Failed to find address of vkCreateDebugUtilsMessengerEXT");
		exit(1);
	}

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	CheckVkResult("Debug utils messenger", vkCreateDebugUtilsMessenger(vulkanInstance, &messengerCreateInfo, nullptr, &debugMessenger));
	DEBUG_LOG("Debug utils messenger created");

	VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
	CheckVkResult("Create window surface", glfwCreateWindowSurface(vulkanInstance, window, nullptr, &vkSurface));


	// Setup physical device
	uint32_t numDevices = 0;
	CheckVkResult("vkEnumeratePhysicalDevices (1)", vkEnumeratePhysicalDevices(vulkanInstance, &numDevices, nullptr));
	printf("Num physical devices %d\n", numDevices);

	std::vector<PhysicalDevice> physicalDevices;
	physicalDevices.resize(numDevices);

	std::vector<VkPhysicalDevice> vkDevices;
	vkDevices.resize(numDevices);
	CheckVkResult("vkEnumeratePhysicalDevices (2)", vkEnumeratePhysicalDevices(vulkanInstance, &numDevices, vkDevices.data()));

	for (uint32_t i = 0; i < numDevices; i++)
	{
		VkPhysicalDevice vkPhysicalDevice = vkDevices[i];
		physicalDevices[i].physicalDevice = vkPhysicalDevice;

		vkGetPhysicalDeviceProperties(vkPhysicalDevice, &physicalDevices[i].deviceProperties);

		printf("Device name: %s\n", physicalDevices[i].deviceProperties.deviceName);
		uint32_t apiVersion = physicalDevices[i].deviceProperties.apiVersion;
		printf("	API version: %d.%d.%d.%d\n",
			VK_API_VERSION_VARIANT(apiVersion),
			VK_VERSION_MAJOR(apiVersion),
			VK_VERSION_MINOR(apiVersion),
			VK_VERSION_PATCH(apiVersion));


		uint32_t numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &numQueueFamilies, nullptr);
		printf("	Num of family queues: %d\n", numQueueFamilies);
		
		physicalDevices[i].familyProperties.resize(numQueueFamilies);
		physicalDevices[i].supportsPresent.resize(numQueueFamilies);

		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &numQueueFamilies, physicalDevices[i].familyProperties.data());

		for (uint32_t queueFamIndex = 0; queueFamIndex < numQueueFamilies; queueFamIndex++)
		{
			const VkQueueFamilyProperties& familyProperties = physicalDevices[i].familyProperties[queueFamIndex];

			printf("	Family %d Num queues: %d", queueFamIndex, familyProperties.queueCount);

			VkQueueFlags flags = familyProperties.queueFlags;
			printf("	GFX %s, Compute %s, Transfer %s, Sparse Binding %s\n",
				(flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
				(flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
				(flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
				(flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

			CheckVkResult("vkGetPhysicalDeviceSurfaceSupportKHR error",
				vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueFamIndex, vkSurface, &(physicalDevices[i].supportsPresent[queueFamIndex])));
		}

		uint32_t numFormats = 0;
		CheckVkResult("vkGetPhysicalDeviceSurfaceFormatsKHR error (1)",
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &numFormats, nullptr));
		assert(numFormats > 0);

		physicalDevices[i].surfaceFormats.resize(numFormats);
		CheckVkResult("vkGetPhysicalDeviceSurfaceFormatsKHR error (2)",
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &numFormats, physicalDevices[i].surfaceFormats.data()));

		for (uint32_t formatIndex = 0; formatIndex < numFormats; formatIndex++)
		{
			const VkSurfaceFormatKHR& surfaceFormat = physicalDevices[i].surfaceFormats[formatIndex];
			printf("	Format %x color space %x\n", surfaceFormat.format, surfaceFormat.colorSpace);
		}

		CheckVkResult("vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &physicalDevices[i].surfaceCapabilities));

		PrintImageUsageFlags(physicalDevices[i].surfaceCapabilities.supportedUsageFlags);

		uint32_t numPresentModes = 0;
		CheckVkResult("vkGetPhysicalDeviceSurfacePresentModesKHR error (1)",
			vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &numPresentModes, nullptr));
		assert(numPresentModes != 0);

		physicalDevices[i].presentModes.resize(numPresentModes);
		CheckVkResult("vkGetPhysicalDeviceSurfacePresentModesKHR error (2)",
			vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &numPresentModes, physicalDevices[i].presentModes.data()));
		
		printf("Number of presentation modes %d\n");

		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &physicalDevices[i].memoryProperties);

		const uint32_t memoryTypeCount = physicalDevices[i].memoryProperties.memoryTypeCount;
		printf("\nNumber of memory types %d\n", memoryTypeCount);
		for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryTypeCount; memoryTypeIndex++)
		{
			printf("%d: flags %x heap %d\n",
				memoryTypeIndex,
				physicalDevices[i].memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags,
				physicalDevices[i].memoryProperties.memoryTypes[memoryTypeIndex].heapIndex);
			
			PrintMemoryProperty(physicalDevices[i].memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags);

			printf("\n");
		}

		printf("Num heap types %d\n", physicalDevices[i].memoryProperties.memoryHeapCount);
		printf("\n");
	}

	VkQueueFlags requiredQueueType = VK_QUEUE_GRAPHICS_BIT;
	bool supportPresent = true;
	uint32_t selectedDeviceIndex = -1;
	uint32_t selectedQueueFamily = -1;
	for (uint32_t deviceIndex = 0; deviceIndex < physicalDevices.size(); deviceIndex++)
	{
		for (uint32_t queueFamPropIndex = 0; queueFamPropIndex < physicalDevices[deviceIndex].familyProperties.size(); queueFamPropIndex++)
		{
			const VkQueueFamilyProperties& queueFamilyProp = physicalDevices[deviceIndex].familyProperties[queueFamPropIndex];
			if ((queueFamilyProp.queueFlags & requiredQueueType) && ((bool)physicalDevices[deviceIndex].supportsPresent[queueFamPropIndex] == supportPresent))
			{
				selectedDeviceIndex = deviceIndex;
				selectedQueueFamily = queueFamPropIndex;
				DEBUG_LOG("Using GFX device %d and queue family %d", selectedDeviceIndex, selectedQueueFamily);
			}
		}
	}

	if (selectedDeviceIndex == -1 || selectedQueueFamily == -1)
	{
		DEBUG_LOG_ERROR("Required queue type %x and supports present %d not found!", requiredQueueType, supportPresent);
	}

	const PhysicalDevice& selectedDevice = physicalDevices[selectedDeviceIndex];

	while (!glfwWindowShouldClose(window))
	{
		// RenderScene();
		glfwPollEvents();
	}

	printf("\n*------- CLOSED APPLICATION -------*\n");

	PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
	vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(vulkanInstance, "vkDestroySurfaceKHR");
	if (!vkDestroySurface)
	{
		DEBUG_LOG_ERROR("Failed to find address of vkDestroySurface");
		exit(1);
	}
	vkDestroySurface(vulkanInstance, vkSurface, nullptr);
	DEBUG_LOG("GLFW window surface destroyed");

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugUtilsMessenger)
	{
		DEBUG_LOG_ERROR("Failed to find address of vkDestroyDebugUtilsMessenger");
		exit(1);
	}
	vkDestroyDebugUtilsMessenger(vulkanInstance, debugMessenger, nullptr);
	DEBUG_LOG("Debug callback destroyed");

	vkDestroyInstance(vulkanInstance, nullptr);
	DEBUG_LOG("Vulkan instance destroyed");

	glfwTerminate();
	DEBUG_LOG("GLFW terminated");

	return 0;
}