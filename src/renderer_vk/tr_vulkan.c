
#include "tr_vulkan.h"
#include "tr_local_gal.h"

static Vulkan vk;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if(pCallbackData->messageIdNumber == 0x48a09f6c)
	{
		// "You are using VK_PIPELINE_STAGE_ALL_COMMANDS_BIT when vkQueueSubmit is called"
		return VK_FALSE;
	}

	ri.Printf(PRINT_ALL, "Vulkan: %s\n", pCallbackData->pMessage);
#if defined(_WIN32)
	OutputDebugStringA(va("\n\n\nVulkan: %s\n\n\n", pCallbackData->pMessage));
	if((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0 &&
	   messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT &&
	   IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif
	return VK_FALSE; // @NOTE: must be false as mandated by the spec
}

static const char* GetVulkanResultShortString(VkResult result)
{
#define VULKAN_CODE(Val, Desc) case Val: return #Val;
	switch(result)
	{
		VULKAN_SUCCESS_CODES(VULKAN_CODE)
		VULKAN_ERROR_CODES(VULKAN_CODE)
		default:
			return "???";
	}
#undef VULKAN_CODE
}

static const char* GetVulkanResultString(VkResult result)
{
#define VULKAN_CODE(Val, Desc) case Val: return Desc;
	switch(result)
	{
		VULKAN_SUCCESS_CODES(VULKAN_CODE)
		VULKAN_ERROR_CODES(VULKAN_CODE)
		default:
			return "???";
	}
#undef VULKAN_CODE
}

static qbool IsErrorCode(VkResult result)
{
#define VULKAN_CODE(Val, Desc) case Val:
	switch(result)
	{
		VULKAN_SUCCESS_CODES(VULKAN_CODE)
			return qfalse;
		default:
			return qtrue;
	}
#undef VULKAN_CODE
}

static void Check(VkResult result, const char* function)
{
	if(!IsErrorCode(result))
	{
		return;
	}

	char funcName[256];
	Q_strncpyz(funcName, function, sizeof(funcName));
	char* s = funcName;
	while(*s != '\0')
	{
		if(*s == '(')
		{
			*s = '\0';
			break;
		}
		++s;
	}

	ri.Error(ERR_FATAL, "'%s' failed with code %s (0x%08X)\n%s\n",
			 funcName,
			 GetVulkanResultShortString(result),
			 (unsigned int)result,
			 GetVulkanResultString(result));
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != NULL)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static qbool UseValidationLayer()
{
	// @TODO:
#if defined(ENABLE_VALIDATION)
	return qtrue;
#else
	return qfalse;
#endif
}

static qbool IsLayerAvailable(const char* name, int itemCount, const VkLayerProperties* itemNames)
{
	for(int i = 0; i < itemCount; ++i)
	{
		if(strcmp(name, itemNames[i].layerName) == 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qbool IsExtensionAvailable(const char* name, int itemCount, const VkExtensionProperties* itemNames)
{
	for(int i = 0; i < itemCount; ++i)
	{
		if(strcmp(name, itemNames[i].extensionName) == 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static void BuildLayerAndExtensionLists()
{
	const char* neededLayers[MAX_LAYERS];
	const char* wantedLayers[MAX_LAYERS];
	const char* neededExtensions[MAX_EXTENSIONS];
	const char* wantedExtensions[MAX_EXTENSIONS];
	int neededLayerCount = 0;
	int wantedLayerCount = 0;
	int neededExtensionCount = 0;
	int wantedExtensionCount = 0;

	if(UseValidationLayer())
	{
		wantedLayers[wantedLayerCount++] = "VK_LAYER_KHRONOS_validation"; // full validation
		//wantedExtensions[wantedExtensionCount++] = "VK_EXT_validation_features"; // update to non-deprecated
	}
	neededExtensions[neededExtensionCount++] = "VK_KHR_surface"; // swap chain
#if defined(_WIN32)
	neededExtensions[neededExtensionCount++] = "VK_KHR_win32_surface"; // Windows swap chain
#endif
	wantedExtensions[wantedExtensionCount++] = "VK_EXT_debug_utils"; // naming resources

	uint32_t layerCount;
	VK(vkEnumerateInstanceLayerProperties(&layerCount, NULL));
	
	{
		VkLayerProperties *layers = (VkLayerProperties*)ri.Hunk_AllocateTempMemory(layerCount * sizeof(VkLayerProperties));

		VK(vkEnumerateInstanceLayerProperties(&layerCount, layers));
		for(int n = 0; n < neededLayerCount; ++n)
		{
			const char* name = neededLayers[n];
			if(!IsLayerAvailable(name, layerCount, layers))
			{
				ri.Error(ERR_FATAL, "Required Vulkan layer '%s' was not found\n", name);
			}
			vk.layers[vk.layerCount++] = name;
		}
		for(int w = 0; w < wantedLayerCount; ++w)
		{
			const char* name = wantedLayers[w];
			if(!IsLayerAvailable(name, layerCount, layers))
			{
				ri.Printf(PRINT_WARNING, "Desired Vulkan layer '%s' was not found, dropped from list\n", name);
			}
			else
			{
				vk.layers[vk.layerCount++] = name;
			}
		}
		ri.Hunk_FreeTempMemory(layers);
	}

	uint32_t extCount, tempCount;
	VK(vkEnumerateInstanceExtensionProperties(NULL, &tempCount, NULL));
	extCount = tempCount;
	for(int l = 0; l < vk.layerCount; ++l)
	{
		VK(vkEnumerateInstanceExtensionProperties(vk.layers[l], &tempCount, NULL));
		extCount += tempCount;
	}
	
	{
		VkExtensionProperties *ext = (VkExtensionProperties*)ri.Hunk_AllocateTempMemory(extCount * sizeof(VkExtensionProperties));
		VK(vkEnumerateInstanceExtensionProperties(NULL, &tempCount, NULL));
		VK(vkEnumerateInstanceExtensionProperties(NULL, &tempCount, ext));
		uint32_t startOffset = tempCount;
		for(int l = 0; l < vk.layerCount; ++l)
		{
			VK(vkEnumerateInstanceExtensionProperties(vk.layers[l], &tempCount, NULL));
			VK(vkEnumerateInstanceExtensionProperties(vk.layers[l], &tempCount, ext + startOffset));
			startOffset += tempCount;
		}
		for(int n = 0; n < neededExtensionCount; ++n)
		{
			const char* name = neededExtensions[n];
			if(!IsExtensionAvailable(name, extCount, ext))
			{
				ri.Error(ERR_FATAL, "Required Vulkan extension '%s' was not found\n", name);
			}
			vk.extensions[vk.extensionCount++] = name;
		}
		for(int w = 0; w < wantedExtensionCount; ++w)
		{
			const char* name = wantedExtensions[w];
			if(!IsExtensionAvailable(name, extCount, ext))
			{
				ri.Printf(PRINT_WARNING, "Desired Vulkan extension '%s' was not found, dropped from list\n", name);
			}
			else
			{
				vk.extensions[vk.extensionCount++] = name;
			}
		}

		vk.ext.EXT_validation_features = IsExtensionAvailable("VK_EXT_validation_features", extCount, ext);
		vk.ext.EXT_debug_utils = IsExtensionAvailable("VK_EXT_debug_utils", extCount, ext);
		ri.Hunk_FreeTempMemory(ext);
	}
}


void CreateInstance()
{
	ri.Printf( PRINT_ALL, "Vulkan Create Instance\n" );
	#if 0
	const VkValidationFeatureEnableEXT validationFeaturesEnabled[] =
	{
		// "GPU-assisted" can detect out of bounds descriptor accesses in shaders, etc
		// @TODO: anything else I must do to make it work? doesn't trigger for me
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
	};
	/*const VkValidationFeatureDisableEXT validationFeaturesDisabled[] =
	{
		VK_VALIDATION_FEATURE_DISABLE_ALL_EXT
	};*/
	VkValidationFeaturesEXT validationFeatures = {};
	validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeatures.enabledValidationFeatureCount = ARRAY_LEN(validationFeaturesEnabled);
	validationFeatures.pEnabledValidationFeatures = validationFeaturesEnabled;
	//validationFeatures.disabledValidationFeatureCount = ARRAY_LEN(validationFeaturesDisabled);
	//validationFeatures.pDisabledValidationFeatures = validationFeaturesDisabled;
	validationFeatures.pNext = NULL;

	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerCreateInfo.pfnUserCallback = &DebugCallback;
	messengerCreateInfo.pUserData = NULL;
	messengerCreateInfo.pNext = NULL;
	messengerCreateInfo.flags = 0;

	// @TODO: get the proper version string
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = MINIMUM_VULKAN_API_VERSION;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 4, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 4, 0);
	appInfo.pApplicationName = "Wolfenstein-vk";
	appInfo.pEngineName = "Wolfenstein-vk";

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	if(vk.layerCount > 0)
	{
		createInfo.enabledLayerCount = vk.layerCount;
		createInfo.ppEnabledLayerNames = vk.layers;
		if(vk.ext.EXT_debug_utils)
		{
			// @NOTE: the messenger must always be chained in first so it ends up
			// in last place and has its own pNext set to NULL, as mandated by the spec
			const void* pSavedNext = createInfo.pNext;
			createInfo.pNext = &messengerCreateInfo;
			messengerCreateInfo.pNext = pSavedNext;
		}
		if(vk.ext.EXT_validation_features)
		{
			const void* pSavedNext = createInfo.pNext;
			createInfo.pNext = &validationFeatures;
			validationFeatures.pNext = pSavedNext;
		}
	}
	if(vk.extensionCount > 0)
	{
		createInfo.enabledExtensionCount = vk.extensionCount;
		createInfo.ppEnabledExtensionNames = vk.extensions;
	}
	VkInstance instance;
	VK(vkCreateInstance(&createInfo, NULL, &instance));
	vk.instance = instance;

	if(vk.ext.EXT_debug_utils)
	{
		if(CreateDebugUtilsMessengerEXT(vk.instance, &messengerCreateInfo, NULL, &vk.ext.debugMessenger) != VK_SUCCESS)
		{
			ri.Printf(PRINT_WARNING, "Failed to register Vulkan debug messenger\n");
		}
	}
	#endif
	/*vk.extensions[0] = "VK_KHR_win32_surface";
	vk.extensionCount = 1;*/

	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerCreateInfo.pfnUserCallback = &DebugCallback;
	messengerCreateInfo.pUserData = NULL;
	messengerCreateInfo.pNext = NULL;
	messengerCreateInfo.flags = 0;


	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = MINIMUM_VULKAN_API_VERSION;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 4, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 4, 0);
	appInfo.pApplicationName = "Wolfenstein-vk";
	appInfo.pEngineName = "Wolfenstein-vk";
	appInfo.pNext = NULL;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.pNext = &messengerCreateInfo;
	if (vk.extensionCount > 0) {
		createInfo.enabledExtensionCount = vk.extensionCount;
		createInfo.ppEnabledExtensionNames = vk.extensions;
	}
	
	if (vk.layerCount > 0) {
		createInfo.enabledLayerCount = vk.layerCount;
		createInfo.ppEnabledLayerNames = vk.layers;
	}

	VK(vkCreateInstance(&createInfo, NULL, &vk.instance));


	if (vk.ext.EXT_debug_utils)
	{
		if (CreateDebugUtilsMessengerEXT(vk.instance, &messengerCreateInfo, NULL, &vk.ext.debugMessenger) != VK_SUCCESS)
		{
			ri.Printf(PRINT_WARNING, "Failed to register Vulkan debug messenger\n");
			
		}
	}
}


static qbool FindSuitableQueueFamilies(Queues* queues, VkPhysicalDevice physicalDevice)
{
	uint32_t familyCount = 0;
	qbool graphicsFound = qfalse;
	qbool presentFound = qfalse;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, NULL);

	
	VkQueueFamilyProperties *families = (VkQueueFamilyProperties*)ri.Hunk_AllocateTempMemory(familyCount * sizeof(VkQueueFamilyProperties));
	{
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families);

		//
		// look for graphics + compute + present
		//

		for(int f = 0; f < familyCount; ++f)
		{
			if(families[f].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
			{
				VkBool32 presentSupport = qfalse;
				VK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, f, vk.surface, &presentSupport));
				if(presentSupport)
				{
					queues->graphicsFamily = f;
					queues->presentFamily = f;
					ri.Hunk_FreeTempMemory(families);
					return qtrue;
				}
			}
		}

		//
		// look for graphics + compute and present separately
		//

		for(int f = 0; f < familyCount; ++f)
		{
			if(families[f].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
			{
				queues->graphicsFamily = f;
				graphicsFound = qtrue;
				break;
			}
		}
	
		for(int f = 0; f < familyCount; ++f)
		{
			VkBool32 presentSupport = qfalse;
			VK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, f, vk.surface, &presentSupport));
			if(presentSupport)
			{
				queues->presentFamily = f;
				presentFound = qtrue;
				break;
			}
		}
	}
	ri.Hunk_FreeTempMemory(families);

	return graphicsFound && presentFound;
}

void PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	VK(vkEnumeratePhysicalDevices(vk.instance, &deviceCount, NULL));
	if(deviceCount == 0)
	{
		ri.Error(ERR_FATAL, "Vulkan found no physical device\n");
	}

	VkPhysicalDevice *devices = (VkPhysicalDevice*)ri.Hunk_AllocateTempMemory(deviceCount * sizeof(VkPhysicalDevice));
	{
		VK(vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices));

		int selection = -1;
		int highestScore = 0;
		for(int d = 0; d < deviceCount; ++d)
		{
			const VkPhysicalDevice physicalDevice = devices[d];

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			Queues queues;
			if(deviceProperties.apiVersion >= MINIMUM_VULKAN_API_VERSION &&
			FindSuitableQueueFamilies(&queues, physicalDevice) &&
			deviceFeatures.shaderSampledImageArrayDynamicIndexing &&
			deviceFeatures.samplerAnisotropy &&
			deviceProperties.limits.timestampComputeAndGraphics &&
			deviceProperties.limits.maxDescriptorSetSampledImages >= MAX_SAMPLER_DESCRIPTORS)
			{
				// score the physical device based on specifics that aren't requirements
				int score = 1;
				if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					score += 1000;
				}

				// select the physical device if it scored better than the others
				if(score > highestScore)
				{
					highestScore = score;
					selection = d;
				}
			}
		}

		if(selection < 0)
		{
			ri.Error(ERR_FATAL, "No suitable physical device found\n");
		}

		vk.physicalDevice = devices[selection];
		vkGetPhysicalDeviceProperties(vk.physicalDevice, &vk.deviceProperties);
		vkGetPhysicalDeviceFeatures(vk.physicalDevice, &vk.deviceFeatures);
		FindSuitableQueueFamilies(&vk.queues, devices[selection]);
	}
	ri.Hunk_FreeTempMemory(devices);
	ri.Printf(PRINT_ALL, "Physical device selected: %s\n", vk.deviceProperties.deviceName);
}

static void CreateDevice()
{
	uint32_t queueCount = 0;
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo[2];
	memset(queueCreateInfo, 0, sizeof(queueCreateInfo));
	queueCreateInfo[queueCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[queueCount].queueFamilyIndex = vk.queues.graphicsFamily;
	queueCreateInfo[queueCount].queueCount = 1;
	queueCreateInfo[queueCount].pQueuePriorities = &queuePriority;
	++queueCount;
	if(vk.queues.presentFamily != vk.queues.graphicsFamily)
	{
		queueCreateInfo[queueCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[queueCount].queueFamilyIndex = vk.queues.presentFamily;
		queueCreateInfo[queueCount].queueCount = 1;
		queueCreateInfo[queueCount].pQueuePriorities = &queuePriority;
		++queueCount;
	}

	const char* extensions[] =
	{
		"VK_KHR_swapchain"
	};

	// @NOTE: alright, we're gonna stick to uniform indexing for now
	//VkPhysicalDeviceDescriptorIndexingFeatures featuresExt {};
	//featuresExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	//featuresExt.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;

	// @TODO: copy over results from vk.deviceFeatures when they're optional
	VkPhysicalDeviceFeatures2 features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	features2.features.samplerAnisotropy = VK_TRUE;
	features2.features.fragmentStoresAndAtomics = VK_TRUE;
	//features2.features.multiDrawIndirect = VK_TRUE;
	//features2.features.shaderClipDistance = VK_TRUE;
	//features2.pNext = &featuresExt;
	features2.pNext = &indexingFeatures;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfo;
	createInfo.queueCreateInfoCount = queueCount;
	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledExtensionCount = ARRAY_LEN(extensions);
	createInfo.pEnabledFeatures = NULL;
	createInfo.pNext = &features2;

	VK(vkCreateDevice(vk.physicalDevice, &createInfo, NULL, &vk.device));

	vkGetDeviceQueue(vk.device, vk.queues.graphicsFamily, 0, &vk.queues.graphics);
	vkGetDeviceQueue(vk.device, vk.queues.presentFamily, 0, &vk.queues.present);
}


void VKimp_Init( void ) {
    if(vk.initialized &&
	   vk.width == glConfig.vidWidth &&
	   vk.height == glConfig.vidHeight)
	{
		return;
	}
	
	ri.Printf( PRINT_ALL, "Initializing Vulkan subsystem\n" );

	vk.instance = VK_NULL_HANDLE;
    BuildLayerAndExtensionLists();
	CreateInstance();
	vk.surface = (VkSurfaceKHR)Sys_Vulkan_Init(vk.instance);
	PickPhysicalDevice();
	CreateDevice();
	
    
}