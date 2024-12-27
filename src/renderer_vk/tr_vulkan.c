
#include "tr_vulkan.h"
#include "tr_local_gal.h"


static Vulkan vk;


static const char* GetStringForVkObjectType(VkObjectType type)
{
	switch(type)
	{
		case VK_OBJECT_TYPE_INSTANCE: return "instance";
		case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "physical device";
		case VK_OBJECT_TYPE_DEVICE: return "logical device";
		case VK_OBJECT_TYPE_QUEUE: return "queue";
		case VK_OBJECT_TYPE_SEMAPHORE: return "semaphore";
		case VK_OBJECT_TYPE_COMMAND_BUFFER: return "command buffer";
		case VK_OBJECT_TYPE_FENCE: return "fence";
		case VK_OBJECT_TYPE_DEVICE_MEMORY: return "device memory";
		case VK_OBJECT_TYPE_BUFFER: return "buffer";
		case VK_OBJECT_TYPE_IMAGE: return "texture";
		case VK_OBJECT_TYPE_EVENT: return "event";
		case VK_OBJECT_TYPE_QUERY_POOL: return "query pool";
		case VK_OBJECT_TYPE_BUFFER_VIEW: return "buffer view";
		case VK_OBJECT_TYPE_IMAGE_VIEW: return "texture view";
		case VK_OBJECT_TYPE_SHADER_MODULE: return "shader";
		case VK_OBJECT_TYPE_PIPELINE_CACHE: return "pipeline cache";
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "pipeline layout";
		case VK_OBJECT_TYPE_RENDER_PASS: return "render pass";
		case VK_OBJECT_TYPE_PIPELINE: return "pipeline";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "descriptor set layout";
		case VK_OBJECT_TYPE_SAMPLER: return "sampler";
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "descriptor pool";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "descriptor set";
		case VK_OBJECT_TYPE_FRAMEBUFFER: return "framebuffer";
		case VK_OBJECT_TYPE_COMMAND_POOL: return "command pool";
		case VK_OBJECT_TYPE_SURFACE_KHR: return "surface";
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "swap chain";
		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "debug callback";
		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "debug messenger";
		default: return "???";
	}
}

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
// TODO: add cvar to use r_gpu and add /gpulist

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
				deviceFeatures.shaderSampledImageArrayDynamicIndexing && //TODO is this the right variable? want dynamic uniform indexing vs nonuniform indexing
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

	VkPhysicalDeviceVulkan13Features vk13f = {};
	vk13f.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vk13f.synchronization2 = VK_TRUE;
	vk13f.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	indexingFeatures.pNext = &vk13f;

	// @TODO: copy over results from vk.deviceFeatures when they're optional
	VkPhysicalDeviceFeatures2 features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	features2.features.samplerAnisotropy = VK_TRUE;
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

static void CreateAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = MINIMUM_VULKAN_API_VERSION;
	allocatorInfo.physicalDevice = vk.physicalDevice;
	allocatorInfo.device = vk.device;
	allocatorInfo.instance = vk.instance;
	allocatorInfo.flags = 0;
	VK(vmaCreateAllocator(&allocatorInfo, &vk.allocator));
}

static void SetObjectName(VkObjectType type, uint64_t object, const char* name)
{
	if(name == NULL)
	{
		return;
	}

	PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(vk.device, "vkSetDebugUtilsObjectNameEXT");
	if(pfnSetDebugUtilsObjectNameEXT == NULL)
	{
		return;
	}

	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.pObjectName = va("%s [%s]", name, GetStringForVkObjectType(type));
	nameInfo.objectHandle = object;
	nameInfo.objectType = type;
	nameInfo.pNext = NULL;
	if(pfnSetDebugUtilsObjectNameEXT(vk.device, &nameInfo) != VK_SUCCESS)
	{
		ri.Printf(PRINT_DEVELOPER, "vkSetDebugUtilsObjectNameEXT failed\n");
	}
}

#if 0
void GAL_CreateFence(galFence* fenceHandle, const char* name)
{
	assert(fenceHandle);
	assert(name);

	Fence fence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VK(vkCreateFence(vk.device, &createInfo, NULL, &fence.fence));
	fence.submitted = qfalse;
	*fenceHandle = vk.fencePool.Add(fence);

	SetObjectName(VK_OBJECT_TYPE_FENCE, (uint64_t)fence.fence, name);
}

void GAL_DestroyFence(galFence fence)
{
	vkDestroyFence(vk.device, vk.fencePool.Get(fence).fence, NULL);
	vk.fencePool.Remove(fence);
}
#endif
static VkImageUsageFlags GetVkImageUsageFlags(galResourceStateFlags state)
{
	VkImageUsageFlags flags = VK_IMAGE_USAGE_SAMPLED_BIT;
	if(state & RenderTargetBit)
	{
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	else if(state & DepthWriteBit)
	{
		flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if(state & CopySourceBit)
	{
		flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if(state & CopyDestinationBit)
	{
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if(state & ShaderInputBit)
	{
		// @TODO: is this correct ???
		flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}

	return flags;
}

static VkFormat GetVkFormat(galTextureFormatId format)
{
	assert((unsigned int)format < Count);

	switch(format)
	{
		case R8G8B8A8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case B8G8R8A8_sRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case D16_UNorm: return VK_FORMAT_D16_UNORM;
		case D32_SFloat: return VK_FORMAT_D32_SFLOAT;
		//case D24_UNorm_S8_UInt: return VK_FORMAT_D24_UNORM_S8_UINT;
		case R32_UInt: return VK_FORMAT_R32_UINT;
		default: assert(0); return VK_FORMAT_R8G8B8A8_UNORM;
	}
}

static VkImageAspectFlags GetVkImageAspectFlags(VkFormat format)
{
	switch(format)
	{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

void GAL_CreateTexture(galTexture* textureHandle, const galTextureDesc* desc)
{
	assert(textureHandle);
	assert(desc);
	assert(desc->width > 0);
	assert(desc->height > 0);
	assert(desc->mipCount > 0);
	assert(desc->sampleCount > 0);

	VkFormat format = GetVkFormat(desc->format);
	const qbool ownsImage = desc->nativeImage == VK_NULL_HANDLE;

	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	if(!ownsImage)
	{
		image = (VkImage)desc->nativeImage;
		format = SURFACE_FORMAT_RGBA; // @TODO:
	}
	else
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = desc->width;
		imageInfo.extent.height = desc->height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = desc->mipCount;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // @TODO: desc->initialState
		imageInfo.usage = GetVkImageUsageFlags(desc->initialState) | GetVkImageUsageFlags((galResourceStateFlags)desc->descriptorType);
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // @TODO: desc->sampleCount
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK(vmaCreateImage(vk.allocator, &imageInfo, &allocCreateInfo, &image, &allocation, NULL));

		SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)image, desc->name);
	}

	VkImageView view;
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = GetVkImageAspectFlags(format);
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	VK(vkCreateImageView(vk.device, &viewInfo, NULL, &view));
	SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)view, desc->name);

	static uint32_t rtCounter = 1;

	Texture texture = {};
	texture.desc = *desc;
	texture.image = image;
	texture.view = view;
	texture.allocation = allocation;
	texture.ownsImage = ownsImage;
	texture.definedLayout = qfalse;
	texture.uniqueRenderTargetId = (desc->initialState & RenderTargetBit) ? rtCounter++ : 0;
	texture.format = format;
	//*textureHandle = vk.texturePool.Add(texture);
}

static void CreateSwapChain()
{
	VkSurfaceCapabilitiesKHR caps;
	VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &caps));

	if(glConfig.vidWidth < caps.minImageExtent.width ||
	   glConfig.vidWidth > caps.maxImageExtent.width ||
	   glConfig.vidHeight < caps.minImageExtent.height ||
	   glConfig.vidHeight > caps.minImageExtent.height)
	{
		ri.Error(ERR_FATAL, "Can't create a swap chain of the requested dimensions (%d x %d)\n",
				 glConfig.vidWidth, glConfig.vidHeight);
	}

	uint32_t formatCount;
	VK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, NULL));

	qbool formatFound = qfalse;
	VkSurfaceFormatKHR selectedFormat;
	selectedFormat.format = VK_FORMAT_MAX_ENUM;
	selectedFormat.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
	if(formatCount > 0)
	{
		VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR*)ri.Hunk_AllocateTempMemory(formatCount * sizeof(VkSurfaceFormatKHR));
		{
			VK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats));

			for(int f = 0; f < formatCount; ++f)
			{
				if(formats[f].format == SURFACE_FORMAT_RGBA) // @TODO:
				{
					selectedFormat = formats[f];
					formatFound = qtrue;
					break;
				}
			}
		}
		ri.Hunk_FreeTempMemory(formats);
	}

	if(!formatFound)
	{
		ri.Error(ERR_FATAL, "No suitable format found\n");
	}
	vk.swapChainFormat = selectedFormat;

	uint32_t presentModeCount;
	VK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, NULL));

	qbool presentModeFound = qfalse;
	VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
	if(presentModeCount > 0)
	{
		VkPresentModeKHR *presentModes = (VkPresentModeKHR*)ri.Hunk_AllocateTempMemory(presentModeCount * sizeof(VkPresentModeKHR));
		{

			VK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, presentModes));

			for(int p = 0; p < presentModeCount; ++p)
			{
				if(presentModes[p] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					selectedPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					presentModeFound = qtrue;
				}
			}

		/*
		@TODO:add support for all these with correct preference orders
		VK_PRESENT_MODE_IMMEDIATE_KHR <-- no V-sync choice
		VK_PRESENT_MODE_FIFO_KHR <-- V-sync choice #2
		VK_PRESENT_MODE_FIFO_RELAXED_KHR <-- adaptive V-sync
		VK_PRESENT_MODE_MAILBOX_KHR <-- V-sync choice #1
		*/
		}
		ri.Hunk_FreeTempMemory(presentModes);
	}

	if(!presentModeFound)
	{
		ri.Error(ERR_FATAL, "No suitable presentation mode found\n");
	}

	// @NOTE: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT is the only image usage flag
	// that is guaranteed to be available
	const uint32_t queueFamilyIndices[] = { vk.queues.graphicsFamily, vk.queues.presentFamily };
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = vk.surface;
	createInfo.minImageCount = GAL_FRAMES_IN_FLIGHT;
	createInfo.imageFormat = selectedFormat.format;
	createInfo.imageColorSpace = selectedFormat.colorSpace;
	createInfo.imageExtent.width = glConfig.vidWidth;
	createInfo.imageExtent.height = glConfig.vidHeight;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT; //vk error needed to add TRANSFER_DST_BIT
	createInfo.preTransform = caps.currentTransform; // keep whatever is already going on
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = selectedPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if(vk.queues.presentFamily != vk.queues.graphicsFamily)
	{
		// @TODO: handle properly and make exclusive
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = ARRAY_LEN(queueFamilyIndices);
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = NULL;
	}

	VK(vkCreateSwapchainKHR(vk.device, &createInfo, NULL, &vk.swapChain));

	uint32_t imageCount;
	VK(vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, NULL));
	vk.swapChainImageCount = imageCount;
	if(imageCount > MAX_SWAP_CHAIN_IMAGES)
	{
		ri.Error(ERR_FATAL, "Too many swap chain images returned (%d > %d)\n", imageCount, MAX_SWAP_CHAIN_IMAGES);
	}
	VkImage *images = (VkImage*)ri.Hunk_AllocateTempMemory(imageCount * sizeof(VkImage));
	{
		VK(vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, images));

		galTextureDesc rtDesc = {};
		rtDesc.width = glConfig.vidWidth;
		rtDesc.height = glConfig.vidHeight;
		rtDesc.mipCount = 1;
		rtDesc.sampleCount = 1;
		rtDesc.descriptorType = SampledImageBit;
		rtDesc.initialState = RenderTargetBit | PresentBit;
		//rtDesc.format = GAL_SURFACE_FORMAT; // @TODO:
		for(int i = 0; i < imageCount; ++i)
		{
			rtDesc.nativeImage = (uint64_t)images[i];
			rtDesc.name = va("swap chain render target #%d", i + 1);
			//GAL_CreateTexture(&vk.swapChainRenderTargets[i], &rtDesc);
			vk.swapChainImages[i] = images[i];
			
		}
	}
	ri.Hunk_FreeTempMemory(images);
}

void GAL_CreateCommandPool(galCommandPool* poolHandle, const galCommandPoolDesc* desc)
{
	assert(poolHandle);
	assert(desc);

	CommandPool pool = {};
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = desc->presentQueue ? vk.queues.presentFamily : vk.queues.graphicsFamily;
	createInfo.flags = desc->transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
	createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // @TODO:
	VK(vkCreateCommandPool(vk.device, &createInfo, NULL, &pool.commandPool));
	//*poolHandle = vk.commandPoolPool.Add(pool);
	vk.commandPool = pool;
}

void GAL_CreateCommandBuffer(galCommandBuffer* cmdBuf, galCommandPool pool, int instance)
{
	assert(cmdBuf);

	//const VkCommandPool commandPool = vk.commandPoolPool.Get(pool).commandPool;
	const VkCommandPool commandPool = vk.commandPool.commandPool;

	CommandBuffer buffer = {};
	buffer.commandPool = commandPool;
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	VK(vkAllocateCommandBuffers(vk.device, &allocInfo, &buffer.commandBuffer));
	//*cmdBuf = vk.commandBufferPool.Add(buffer);
	vk.commandBuffer[instance] = buffer;
}

static void CreateTempCommandBuffer()
{
	galCommandPoolDesc desc = {};
	desc.presentQueue = qfalse;
	desc.transient = qtrue;
	GAL_CreateCommandPool(&vk.tempCommandPool, &desc);
	GAL_CreateCommandBuffer(&vk.tempCommandBuffer, vk.tempCommandPool, 0);
	GAL_CreateCommandBuffer(&vk.tempCommandBuffer, vk.tempCommandPool, 1);
}

static void RecordCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    VkClearColorValue clearColor = { 0.0f, 1.0f, 1.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;
    
    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;
          
    for (uint32_t i = 0 ; i < GAL_FRAMES_IN_FLIGHT ; i++) {             
        VK(vkBeginCommandBuffer(vk.commandBuffer[i].commandBuffer, &beginInfo));

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.image = vk.swapChainImages[i];
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

		VkDependencyInfo dep = {};
		dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.imageMemoryBarrierCount = 1;
		dep.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(vk.commandBuffer[i].commandBuffer, &dep);

        vkCmdClearColorImage(vk.commandBuffer[i].commandBuffer, vk.swapChainImages[i], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);                

		VK(vkEndCommandBuffer(vk.commandBuffer[i].commandBuffer));
    }
}
#if 0
void GAL_CreateSemaphore(galSemaphore* semaphoreHandle, const char* name)
{
	assert(semaphoreHandle);
	assert(name);

	Semaphore semaphore;
	VkSemaphoreCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &semaphore.semaphore));
	semaphore.signaled = qfalse;
	*semaphoreHandle = vk.semaphorePool.Add(semaphore);

	SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore.semaphore, name);
}
#endif

static void RenderScene()
{
	Semaphore semaphore;
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &semaphore.semaphore));
	semaphore.signaled = qfalse;
	vk.semaphore[0] = semaphore;

	SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore.semaphore, "semaphore 1");

	RecordCommandBuffer();
	uint32_t ImageIndex = 0;

	//Semaphore semaphore = vk.semaphore[0];
    
    const VkResult r = vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, semaphore.semaphore, VK_NULL_HANDLE, &ImageIndex);
 
	// @TODO: when r is VK_ERROR_OUT_OF_DATE_KHR, recreate the swap chain
	if(r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
	{
		semaphore.signaled = qtrue;
	}
	else
	{
		Check(r, "vkAcquireNextImageKHR");
		semaphore.signaled = qfalse;
	}

    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vk.commandBuffer[ImageIndex].commandBuffer;
    
    VK(vkQueueSubmit(vk.queues.present, 1, &submitInfo, VK_NULL_HANDLE));    
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vk.swapChain;
    presentInfo.pImageIndices      = &ImageIndex;
    
    VK(vkQueuePresentKHR(vk.queues.present, &presentInfo));    
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
	CreateAllocator();
    CreateSwapChain();
	CreateTempCommandBuffer();
	RenderScene();
}