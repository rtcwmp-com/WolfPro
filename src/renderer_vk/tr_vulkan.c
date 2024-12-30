
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

static VkFormat GetVkFormatCnt(galDataTypeId type, uint32_t count)
{
	assert((unsigned int)type < galDataTypeIdCount);
	assert(count >= 1 && count <= 4);

	const VkFormat formats[galDataTypeIdCount][4] =
	{
		{ VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT },
		{ VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
		{ VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32A32_UINT }
	};

	return formats[type][count - 1];
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
    createInfo.minImageCount = RHI_FRAMES_IN_FLIGHT;
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

void AcquireSubmitPresent() {


    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vk.commandBuffer[vk.currentFrameIndex].commandBuffer;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &vk.imageAcquired.semaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    =  &vk.renderComplete.semaphore;
    const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    submitInfo.pWaitDstStageMask = &flags;

    
    VK(vkQueueSubmit(vk.queues.present, 1, &submitInfo, vk.inFlightFence.fence));    
    

    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vk.swapChain;
    presentInfo.pImageIndices      = &vk.swapChainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &vk.renderComplete.semaphore;
    
    
    VK(vkQueuePresentKHR(vk.queues.present, &presentInfo));    

}

static void InitSwapChainImages()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    VkClearColorValue clearColor = { 1.0f, 0.0f, 1.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;
    
    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;
    
    for (uint32_t i = 0 ; i < vk.swapChainImageCount ; i++) {
        VkCommandBuffer commandBuffer = vk.commandBuffer[0].commandBuffer;
        VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        {
            VkImageMemoryBarrier2 barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = vk.swapChainImages[i];
            barrier.srcAccessMask = VK_ACCESS_2_NONE;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

            VkDependencyInfo dep = {};
            dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dep.imageMemoryBarrierCount = 1;
            dep.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(commandBuffer, &dep);
        }

        vkCmdClearColorImage(commandBuffer, vk.swapChainImages[i], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);                

        {
            VkImageMemoryBarrier2 barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = vk.swapChainImages[i];
            barrier.srcAccessMask = VK_ACCESS_2_NONE;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
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
            vkCmdPipelineBarrier2(commandBuffer, &dep);
        }

        VkSemaphoreWaitInfo waitInfo = {};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &vk.renderComplete.semaphore;
        
        VK(vkEndCommandBuffer(commandBuffer));
        VK(vkWaitForFences(vk.device, 1, &vk.inFlightFence.fence, VK_TRUE, UINT64_MAX));
        VK(vkResetFences(vk.device, 1, &vk.inFlightFence.fence));
        //vkWaitSemaphores(vk.device, &waitInfo, UINT64_MAX)
        // const VkResult r = vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, vk.imageAcquired.semaphore, VK_NULL_HANDLE, &vk.swapChainImageIndex); 
        // AcquireSubmitPresent();
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        submitInfo.pWaitDstStageMask = &flags;
        VK(vkQueueSubmit(vk.queues.present, 1, &submitInfo, vk.inFlightFence.fence));
        VK(vkDeviceWaitIdle(vk.device));
        
         
    }
    
    
}

static void CreateImageView(){
    for(int i = 0; i < vk.swapChainImageCount; i++){
        VkImageView view;
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vk.swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = SURFACE_FORMAT_RGBA;
        viewInfo.subresourceRange.aspectMask = GetVkImageAspectFlags(SURFACE_FORMAT_RGBA);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VK(vkCreateImageView(vk.device, &viewInfo, NULL, &view));
        SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)view, "ImageView");
        vk.swapChainImageViews[i] = view;
    }
    
}

int n = 0;
static void BuildCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    n += 1;
    if (n == 2500)
        n = 0;
    double sine = sin(2 * M_PI * 8.4e-3 * n);
    
    VkClearColorValue clearColor = { 0.0f, (float)sine*0.5+0.5, 1.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;
    
    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;
          
        
    VK(vkBeginCommandBuffer(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, &beginInfo));

    {
        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = vk.swapChainImages[vk.currentFrameIndex];
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

        VkDependencyInfo dep = {};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, &dep);
    }


    vkCmdClearColorImage(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, vk.swapChainImages[vk.currentFrameIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);                

    {
        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = vk.swapChainImages[vk.currentFrameIndex];
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

        VkDependencyInfo dep = {};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, &dep);
    }

    

    VkRenderingAttachmentInfo colorAttachmentInfo = {};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = vk.swapChainImageViews[vk.swapChainImageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.renderArea.extent.height = glConfig.vidHeight;
    renderingInfo.renderArea.extent.width = glConfig.vidWidth;

    

    vkCmdBeginRendering(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, &renderingInfo);
	vkCmdBindPipeline(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline.pipeline);
    //vkCmdBindDescriptorSets(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	//						vk.pipelineLayout.pipelineLayout, 0, 1, &set.descriptorSet, 0, NULL);
    VkDeviceSize vertexBufferOffset = 0;
    VkDeviceSize colorBufferOffset = 0;
    VkDeviceSize offsets[2] = {vertexBufferOffset, colorBufferOffset };
    VkBuffer buffers[2] = {vk.vertexBuffer, vk.colorBuffer};
    vkCmdBindVertexBuffers(vk.commandBuffer[vk.currentFrameIndex].commandBuffer,0,2, buffers, offsets);
    vkCmdBindIndexBuffer(vk.commandBuffer[vk.currentFrameIndex].commandBuffer,vk.indexBuffer,0,VK_INDEX_TYPE_UINT32);
    //vkCmdBindVertexBuffers(vk.commandBuffer[vk.currentFrameIndex].commandBuffer,0,1,&vk.colorBuffer, &colorBufferOffset);
    //vkCmdDraw(vk.commandBuffer[vk.currentFrameIndex].commandBuffer,4, 1, 0, 0);
    vkCmdDrawIndexed(vk.commandBuffer[vk.currentFrameIndex].commandBuffer,6,1,0,0,0);
    
    vkCmdEndRendering(vk.commandBuffer[vk.currentFrameIndex].commandBuffer);
    {
        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = vk.swapChainImages[vk.currentFrameIndex];
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
        vkCmdPipelineBarrier2(vk.commandBuffer[vk.currentFrameIndex].commandBuffer, &dep);
    }

    VK(vkEndCommandBuffer(vk.commandBuffer[vk.currentFrameIndex].commandBuffer));
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

static void CreateSyncObjects(){
    Semaphore imageAcquired = {};
    {
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &imageAcquired.semaphore));
        imageAcquired.signaled = qfalse;

        SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)imageAcquired.semaphore, "imageAcquired semaphore");
    }
    vk.imageAcquired = imageAcquired;
    
    Semaphore renderComplete = {};
    {
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &renderComplete.semaphore));
        renderComplete.signaled = qfalse;

        SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)renderComplete.semaphore, "renderComplete semaphore");
    }
    vk.renderComplete = renderComplete;

    Fence inFlightFence = {};
    {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //initialize signaled so beginframe's wait passes

        VK(vkCreateFence(vk.device, &fenceInfo, NULL, &inFlightFence.fence));
        inFlightFence.submitted = qfalse;
    }
    vk.inFlightFence = inFlightFence;


}

static void RenderScene()
{
    
    

}



void RHI_BeginFrame() {
    vk.currentFrameIndex = (vk.currentFrameIndex + 1) % RHI_FRAMES_IN_FLIGHT;
    VK(vkWaitForFences(vk.device, 1, &vk.inFlightFence.fence, VK_TRUE, UINT64_MAX));
    VK(vkResetFences(vk.device, 1, &vk.inFlightFence.fence));
    
    const VkResult r = vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, vk.imageAcquired.semaphore, VK_NULL_HANDLE, &vk.swapChainImageIndex); 
 
    // @TODO: when r is VK_ERROR_OUT_OF_DATE_KHR, recreate the swap chain
    if(r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
    {
        vk.imageAcquired.signaled = qtrue;
    }
    else
    {
        Check(r, "vkAcquireNextImageKHR");
        vk.imageAcquired.signaled = qfalse;
    }

    BuildCommandBuffer();

    


}
void RHI_EndFrame() {
    AcquireSubmitPresent();
}

static VkShaderStageFlags GetVkShaderStageFlags(galShaderTypeId shaderType)
{
	assert((unsigned int)shaderType < galShaderTypeIdCount);

	typedef struct 
	{
		galShaderTypeId inValue;
		VkShaderStageFlags outValue;
	} Pair;
	const Pair pairs[] =
	{
		{ galShaderTypeIdVertex, VK_SHADER_STAGE_VERTEX_BIT },
		{ galShaderTypeIdPixel, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ galShaderTypeIdCompute, VK_SHADER_STAGE_COMPUTE_BIT }
	};

	for(int p = 0; p < ARRAY_LEN(pairs); ++p)
	{
		if(shaderType == pairs[p].inValue)
		{
			return pairs[p].outValue;
		}
	}

	assert(0); // means pairs is incomplete!
	return 0;
}

#if 0
void GAL_CreatePipelineLayout(galPipelineLayout* layoutHandle, const galPipelineLayoutDesc* desc)
{
    
	assert(layoutHandle);
	assert(desc);
	assert(desc->name);

	VkDescriptorSetLayout layouts[16];
	assert(desc->descriptorSetLayoutCount <= ARRAY_LEN(layouts));

	for(uint32_t l = 0; l < desc->descriptorSetLayoutCount; ++l)
	{
		const DescriptorSetLayout descSetLayout = vk.descriptorSetLayoutPool.Get(desc->descriptorSetLayouts[l]);
		layouts[l] = descSetLayout.layout;
	}

	// @TODO: check that vertex and pixel shader ranges don't overlap?

	VkPushConstantRange pushConstantRanges[galShaderTypeIdCount];
	uint32_t pushConstantsByteCount = 0;
	uint32_t pushConstantsRangeCount = 0;
	for(int s = 0; s < galShaderTypeIdCount; ++s)
	{
		const uint32_t byteOffset = desc->pushConstantsPerStage[s].byteOffset;
		const uint32_t byteCount = desc->pushConstantsPerStage[s].byteCount;
		assert(byteOffset + byteCount <= 128);

		if(byteCount > 0)
		{
			pushConstantsByteCount += byteCount;
			pushConstantRanges[pushConstantsRangeCount].offset = byteOffset;
			pushConstantRanges[pushConstantsRangeCount].size = byteCount;
			pushConstantRanges[pushConstantsRangeCount].stageFlags = GetVkShaderStageFlags((galShaderTypeId)s);
			++pushConstantsRangeCount;
		}
	}
	assert(pushConstantsByteCount <= 128);

	PipelineLayout layout = {};
	memcpy(layout.constantRanges, desc->pushConstantsPerStage, sizeof(layout.constantRanges));
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = desc->descriptorSetLayoutCount;
	createInfo.pSetLayouts = layouts;
	createInfo.pushConstantRangeCount = pushConstantsRangeCount;
	createInfo.pPushConstantRanges = pushConstantRanges;
	VK(vkCreatePipelineLayout(vk.device, &createInfo, NULL, &layout.pipelineLayout));
	//*layoutHandle = vk.pipelineLayoutPool.Add(layout);
    vk.pipelineLayout = layout;

	SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)layout.pipelineLayout, desc->name);
}


void GAL_CreateDescriptorSetLayout(galDescriptorSetLayout* layoutHandle, const galDescriptorSetLayoutDesc* desc)
{
	assert(layoutHandle);
	assert(desc);
	assert(desc->name);
	assert(desc->bindingCount > 0);
	assert(desc->bindings);

	VkSampler immutableSamplers[64];
	uint32_t immutableSamplerCount = 0;

	TempArray<VkDescriptorSetLayoutBinding> bindings(desc->bindingCount + 1);
	TempArray<VkDescriptorBindingFlags> bindingFlags(desc->bindingCount + 1);
	for(uint32_t b = 0; b < desc->bindingCount; ++b)
	{
		const galDescriptorSetLayoutBinding& src = desc->bindings[b];
		VkDescriptorSetLayoutBinding& dst = bindings[b];
		dst.binding = src.bindingSlot;
		dst.stageFlags = GetVkShaderStageFlags(src.stageFlags);
		dst.descriptorType = GetVkDescriptorType(src.descriptorType);
		dst.descriptorCount = src.descriptorCount;
		dst.pImmutableSamplers = NULL;
		if(src.immutableSamplers != NULL)
		{
			const uint32_t count = src.descriptorCount;
			const uint32_t base = immutableSamplerCount;
			if(base + count > ARRAY_LEN(immutableSamplers))
			{
				ri.Error(ERR_FATAL, "Too many immutable samplers specified\n");
			}
			for(uint32_t d = 0; d < count; ++d)
			{
				immutableSamplers[base + d] = vk.samplerPool.Get(src.immutableSamplers[d]).sampler;
			}
			dst.pImmutableSamplers = &immutableSamplers[base];
			immutableSamplerCount += count;
		}
		bindingFlags[b] = src.canUpdateUnusedWhilePending ? VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT : 0;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo descSetFlagsCreateInfo {};
	descSetFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	descSetFlagsCreateInfo.bindingCount = desc->bindingCount;
	descSetFlagsCreateInfo.pBindingFlags = bindingFlags;

	DescriptorSetLayout layout {};
	VkDescriptorSetLayoutCreateInfo descSetCreateInfo {};
	descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetCreateInfo.bindingCount = desc->bindingCount;
	descSetCreateInfo.pBindings = bindings;
	descSetCreateInfo.pNext = &descSetFlagsCreateInfo;
	VK(vkCreateDescriptorSetLayout(vk.device, &descSetCreateInfo, NULL, &layout.layout));
	*layoutHandle = vk.descriptorSetLayoutPool.Add(layout);

	SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)layout.layout, desc->name);
}
#endif

static void CreateTrianglePipelineLayout()
{

    VkDescriptorBindingFlags bindingFlags = 0;
    

    VkDescriptorSetLayoutBindingFlagsCreateInfo descSetFlagsCreateInfo = {};
	descSetFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	// descSetFlagsCreateInfo.bindingCount = 1;
	// descSetFlagsCreateInfo.pBindingFlags = &bindingFlags;


    // VkDescriptorSetLayoutBinding binding = {};

    // binding.binding = 0;
    // binding.stageFlags = GetVkShaderStageFlags(src.stageFlags);
    // binding.descriptorType = GetVkDescriptorType(src.descriptorType);
    // binding.descriptorCount = src.descriptorCount;
    // binding.pImmutableSamplers = NULL;

    
	DescriptorSetLayout descLayout = {};
	VkDescriptorSetLayoutCreateInfo descSetCreateInfo = {};
	descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetCreateInfo.bindingCount = 0;
	descSetCreateInfo.pBindings = NULL;
	descSetCreateInfo.pNext = &descSetFlagsCreateInfo;
	VK(vkCreateDescriptorSetLayout(vk.device, &descSetCreateInfo, NULL, &descLayout.layout));
	//*layoutHandle = vk.descriptorSetLayoutPool.Add(layout);

	SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)descLayout.layout, "Desc Layout");

    PipelineLayout layout = {};
	//memcpy(layout.constantRanges, desc->pushConstantsPerStage, sizeof(layout.constantRanges));
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//createInfo.setLayoutCount = desc->descriptorSetLayoutCount;
	// createInfo.pSetLayouts = layouts;
	//createInfo.pushConstantRangeCount = pushConstantsRangeCount;
	//createInfo.pPushConstantRanges = pushConstantRanges;
	VK(vkCreatePipelineLayout(vk.device, &createInfo, NULL, &layout.pipelineLayout));
	//*layoutHandle = vk.pipelineLayoutPool.Add(layout);
    vk.pipelineLayout = layout;

	SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)layout.pipelineLayout, "Pipeline Layout");
}

static void BuildSpecializationDesc(const VkSpecializationInfo** outPtr, VkSpecializationInfo* out,
									size_t maxEntries, VkSpecializationMapEntry* entries,
									const galSpecialization* input)
{
	if(input->entryCount == 0)
	{
		*outPtr = VK_NULL_HANDLE;
		return;
	}

	assert(input->entryCount <= maxEntries);

	out->mapEntryCount = input->entryCount;
	out->pMapEntries = entries;
	out->dataSize = input->byteCount;
	out->pData = input->data;

	for(uint32_t c = 0; c < input->entryCount; ++c)
	{
		entries[c].constantID = input->entries[c].constandId;
		entries[c].offset = input->entries[c].byteOffset;
		entries[c].size = input->entries[c].byteCount;
	}

	*outPtr = out;
}

static VkBlendFactor GetSourceColorBlendFactor(unsigned int bits)
{
	switch(bits)
	{
		case GLS_SRCBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
		case GLS_SRCBLEND_ONE: return VK_BLEND_FACTOR_ONE;
		case GLS_SRCBLEND_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GLS_SRCBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GLS_SRCBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GLS_SRCBLEND_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		default: return VK_BLEND_FACTOR_ONE;
	}
}

static VkBlendFactor GetDestinationColorBlendFactor(unsigned int bits)
{
	switch(bits)
	{
		case GLS_DSTBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
		case GLS_DSTBLEND_ONE: return VK_BLEND_FACTOR_ONE;
		case GLS_DSTBLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GLS_DSTBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GLS_DSTBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		default: return VK_BLEND_FACTOR_ZERO;
	}
}

static VkCullModeFlags GetVkCullModeFlags(cullType_t cullType)
{
	assert((unsigned int)cullType < CT_COUNT);

	switch(cullType)
	{
		case CT_FRONT_SIDED: return VK_CULL_MODE_BACK_BIT;
		case CT_BACK_SIDED: return VK_CULL_MODE_FRONT_BIT;
		default: return VK_CULL_MODE_NONE;
	}
}

#include "shaders/triangle_ps.h"
#include "shaders/triangle_vs.h"
static void CreatePipeline() {

    VkShaderModule vsModule;
    VkShaderModuleCreateInfo vsCreateInfo = {};
    vsCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vsCreateInfo.codeSize = sizeof(triangle_vs);
    vsCreateInfo.pCode = triangle_vs;

    VkShaderModule psModule;
    VkShaderModuleCreateInfo psCreateInfo = {};
    psCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    psCreateInfo.codeSize = sizeof(triangle_ps);
    psCreateInfo.pCode = triangle_ps;

    VK(vkCreateShaderModule(vk.device, &vsCreateInfo,NULL,&vsModule));
    VK(vkCreateShaderModule(vk.device, &psCreateInfo,NULL,&psModule));

	int firstStage = 0;
	int stageCount = 2;
	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].pName = "vs";
    stages[0].module = vsModule;
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].pName = "ps";
    stages[1].module = psModule;


	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = glConfig.vidWidth;
	viewport.height = glConfig.vidHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = glConfig.vidWidth;
	scissor.extent.height = glConfig.vidHeight;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = GetVkCullModeFlags(CT_TWO_SIDED);
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;

	// VkPipelineMultisampleStateCreateInfo multiSampling {};
	// multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// multiSampling.alphaToCoverageEnable = VK_FALSE;

	/*const qbool disableBlend =
		desc->srcBlend == GLS_SRCBLEND_ONE &&
		desc->dstBlend == GLS_DSTBLEND_ZERO;*/
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorBlendAttachment.srcColorBlendFactor = GetSourceColorBlendFactor(desc->srcBlend);
	// colorBlendAttachment.dstColorBlendFactor = GetDestinationColorBlendFactor(desc->dstBlend);
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // @TODO:
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // @TODO:
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;


	// @NOTE: VK_DYNAMIC_STATE_CULL_MODE_EXT is not widely available (VK_EXT_extended_dynamic_state)
	// const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	// VkPipelineDynamicStateCreateInfo dynamicState {};
	// dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	// dynamicState.dynamicStateCount = ARRAY_LEN(dynamicStates);
	// dynamicState.pDynamicStates = dynamicStates;

	// VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	// VkPipelineDepthStencilStateCreateInfo* depthStencilPtr = NULL;
	// if(desc->renderTargets.hasDepthStencil)
	// {
	// 	depthStencilPtr = &depthStencil;
	// 	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	// 	depthStencil.depthCompareOp = GetVkCompareOp((depthTest_t)desc->depthTest);
	// 	depthStencil.depthTestEnable = desc->enableDepthTest ? VK_TRUE : VK_FALSE;
	// 	depthStencil.depthWriteEnable = desc->enableDepthWrite ? VK_TRUE : VK_FALSE;
	// }

	Pipeline pipeline = {};
	pipeline.compute = qfalse;
    // Provide information for dynamic rendering
    VkPipelineRenderingCreateInfo pipeline_create = {};
    pipeline_create.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipeline_create.pNext                   = VK_NULL_HANDLE;
    pipeline_create.colorAttachmentCount    = 1;

    VkFormat color_rendering_format = SURFACE_FORMAT_RGBA;
    pipeline_create.pColorAttachmentFormats = &color_rendering_format;
    //pipeline_create.depthAttachmentFormat   = depth_format;
    //pipeline_create.stencilAttachmentFormat = depth_format;

    
    VkVertexInputBindingDescription vertexPositionBindingInfo = {};
    vertexPositionBindingInfo.binding = 0; //shader binding point
    vertexPositionBindingInfo.stride = 4 * sizeof(float); //only has vertex positions (XYZW)
    vertexPositionBindingInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription colorBindingInfo = {};
    colorBindingInfo.binding = 1; //shader binding point
    colorBindingInfo.stride = 4 * sizeof(uint8_t); //only has colors (RGBA)
    colorBindingInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    

    VkVertexInputAttributeDescription vertexPositionAttributeInfo = {};
    //a binding can have multiple attributes (interleaved vertex format)
    vertexPositionAttributeInfo.location = 0; //attribute index
    vertexPositionAttributeInfo.binding = vertexPositionBindingInfo.binding; //binding point of this attribute
    vertexPositionAttributeInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexPositionAttributeInfo.offset = 0; //attribute byte offset

    VkVertexInputAttributeDescription colorAttributeInfo = {};
    //a binding can have multiple attributes (interleaved vertex format)
    colorAttributeInfo.location = 1; //attribute index
    colorAttributeInfo.binding = colorBindingInfo.binding; //binding point of this attribute
    colorAttributeInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttributeInfo.offset = 0; //attribute byte offset

    VkVertexInputAttributeDescription vertexAttributes[2] = {vertexPositionAttributeInfo,  colorAttributeInfo};
    VkVertexInputBindingDescription vertexBindings[2] = {vertexPositionBindingInfo, colorBindingInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.vertexBindingDescriptionCount = 2;

    VkPipelineMultisampleStateCreateInfo multiSampling = {};
	multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSampling.alphaToCoverageEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;


    // Use the pNext to point to the rendering create struct
    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext               = &pipeline_create; // reference the new dynamic structure
    createInfo.renderPass          = VK_NULL_HANDLE; // previously required non-null
    createInfo.layout = vk.pipelineLayout.pipelineLayout;
	createInfo.subpass = 0; // we always target sub-pass 0
	createInfo.stageCount = stageCount;
	createInfo.pStages = stages + firstStage;
	createInfo.pColorBlendState = &colorBlending;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.pDynamicState = NULL;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pMultisampleState = &multiSampling;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pVertexInputState = &vertexInputInfo; //add later
	createInfo.pViewportState = &viewportState;
    VK(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &createInfo, NULL, &pipeline.pipeline));

    SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline.pipeline, "Pipeline");
    vk.pipeline = pipeline;
}

#if 0
void GAL_CreateBuffer(galBuffer* bufferHandle, const galBufferDesc* desc)
{
	assert(bufferHandle);
	assert(desc);
	assert(desc->name);
	assert(desc->byteCount > 0);

	Buffer buffer = {};
	buffer.memoryUsage = desc->memoryUsage;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = GetVmaMemoryUsage(desc->memoryUsage);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = desc->byteCount;
	bufferInfo.usage = GetVkBufferUsageFlags(desc->initialState);

	VmaAllocationInfo allocInfo;
	VK(vmaCreateBuffer(vk.allocator, &bufferInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &allocInfo));

	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(vk.allocator, allocInfo.memoryType, &memFlags);
	buffer.hostCoherent = (memFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != 0;
	if(buffer.hostCoherent)
	{
		VK(vmaMapMemory(vk.allocator, buffer.allocation, &buffer.mappedData));
		buffer.mapped = qtrue;
	}

	//*bufferHandle = vk.bufferPool.Add(buffer);

	SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer.buffer, desc->name);
}
#endif

static void CreateBuffers(void *vertexData, uint32_t vertexSize, void* indexData, uint32_t indexSize, void *colorData, uint32_t colorSize){
    //vertex 
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.size = vertexSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocation vmaAlloc = {};

        VmaAllocationInfo allocInfo = {};
        VK(vmaCreateBuffer(vk.allocator, &bufferInfo, &allocCreateInfo, &vk.vertexBuffer, &vmaAlloc, &allocInfo));
        SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk.vertexBuffer, "Vertex Buffer");
        void *mapped;
        VK(vmaMapMemory(vk.allocator, vmaAlloc, &mapped));
        memcpy(mapped, vertexData, vertexSize);
        vmaUnmapMemory(vk.allocator, vmaAlloc);
        vmaFlushAllocation(vk.allocator, vmaAlloc, 0, VK_WHOLE_SIZE);
    }
    //index 
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.size = indexSize;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocation vmaAlloc = {};

        VmaAllocationInfo allocInfo = {};
        VK(vmaCreateBuffer(vk.allocator, &bufferInfo, &allocCreateInfo, &vk.indexBuffer, &vmaAlloc, &allocInfo));
        SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk.indexBuffer, "Index Buffer");
        void *mapped;
        VK(vmaMapMemory(vk.allocator, vmaAlloc, &mapped));
        memcpy(mapped, indexData, indexSize);
        vmaUnmapMemory(vk.allocator, vmaAlloc);
        vmaFlushAllocation(vk.allocator, vmaAlloc, 0, VK_WHOLE_SIZE);

    }
    //color 
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.size = colorSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocation vmaAlloc = {};

        VmaAllocationInfo allocInfo = {};
        VK(vmaCreateBuffer(vk.allocator, &bufferInfo, &allocCreateInfo, &vk.colorBuffer, &vmaAlloc, &allocInfo));
        SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk.colorBuffer, "Color Buffer");
        void *mapped;
        VK(vmaMapMemory(vk.allocator, vmaAlloc, &mapped));
        memcpy(mapped, colorData, colorSize);
        vmaUnmapMemory(vk.allocator, vmaAlloc);
        vmaFlushAllocation(vk.allocator, vmaAlloc, 0, VK_WHOLE_SIZE);

    }
}

#if 0
static void CreateTextureStagingBuffer()
{
	galBufferDesc desc = {};
	desc.name = "texture staging buffer";
	desc.byteCount = MAX_TEXTURE_SIZE * MAX_TEXTURE_SIZE * 4;
	desc.initialState = CopySourceBit;
	desc.memoryUsage = Upload;
	GAL_CreateBuffer(&vk.textureStagingBuffer, &desc);
}
#endif

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
    CreateImageView();
    CreateTempCommandBuffer();
    CreateSyncObjects();
    InitSwapChainImages();
    CreateTrianglePipelineLayout();
    CreatePipeline();
    float vertex[16] = {
        -0.7, 0.7, 0, 1, //x,y,z,w
        0.7, 0.7, 0, 1,
        0.7, -0.7, 0, 1,
        -0.7, -0.7, 0,1  
    };
    uint8_t colors[16] = {
        255, 0, 255, 255,
        0, 255, 0, 255,
        255, 0, 0, 255,
        0, 255, 255, 255
    };
    uint32_t index[6] = {
        0,1,2,2,3,0
    };
    CreateBuffers(vertex, sizeof(vertex), index, sizeof(index), colors, sizeof(colors));
    vk.initialized = qtrue;
}