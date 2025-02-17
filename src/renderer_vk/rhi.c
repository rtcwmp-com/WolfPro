#include "rhi.h"
#include "tr_vulkan.h"

#define GET_SEMAPHORE(handle) ((Semaphore*)Pool_Get(&vk.semaphorePool, handle.h))
#define GET_TEXTURE(handle) ((Texture*)Pool_Get(&vk.texturePool, handle.h))
#define GET_BUFFER(handle) ((Buffer*)Pool_Get(&vk.bufferPool, handle.h))
#define GET_LAYOUT(handle) ((DescriptorSetLayout*)Pool_Get(&vk.descriptorSetLayoutPool, handle.h))
#define GET_PIPELINE(handle) ((Pipeline*)Pool_Get(&vk.pipelinePool, handle.h))
#define GET_DESCRIPTORSET(handle) ((DescriptorSet*)Pool_Get(&vk.descriptorSetPool, handle.h))
#define GET_SAMPLER(handle) ((Sampler*)Pool_Get(&vk.samplerPool, handle.h))

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != NULL){
        func(instance, messenger, NULL);
    }
}

void RHI_Shutdown(qboolean destroyWindow)
{
    vkDeviceWaitIdle(vk.device);

    //destroy user resources
    PoolIterator it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.samplerPool, &it)){
        Sampler *sampler = (Sampler*)it.value;
        vkDestroySampler(vk.device, sampler->sampler, NULL);
    }
    Pool_Clear(&vk.samplerPool);

    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.bufferPool, &it)){
        Buffer *buffer = (Buffer*)it.value;
        if(!buffer->desc.longLifetime || destroyWindow){
            vmaDestroyBuffer(vk.allocator, buffer->buffer, buffer->allocation);
            Pool_Remove(&vk.bufferPool, it.handle);
        }
    }


    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.semaphorePool, &it)){
        Semaphore *semaphore = (Semaphore*)it.value;
        if(!semaphore->longLifetime || destroyWindow){
            vkDestroySemaphore(vk.device, semaphore->semaphore, NULL);
            Pool_Remove(&vk.semaphorePool, it.handle);
        }
    }

    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.descriptorSetLayoutPool, &it)){
        DescriptorSetLayout *descSetLayout = (DescriptorSetLayout*)it.value;
        if(!descSetLayout->desc.longLifetime || destroyWindow){
            vkDestroyDescriptorSetLayout(vk.device,descSetLayout->layout, NULL);
            Pool_Remove(&vk.descriptorSetLayoutPool, it.handle);
        }
    }

    //it = Pool_BeginIteration();
    //while(Pool_Iterate(&vk.descriptorSetPool, &it)){
    //    DescriptorSet *descSet = (DescriptorSet*)it.value;
    //    vkFreeDescriptorSets(vk.device, vk.descriptorPool, 1, &descSet->set);
    //}
    Pool_Clear(&vk.descriptorSetPool);

    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.pipelinePool, &it)){
        Pipeline *pipeline = (Pipeline*)it.value;
        if(!pipeline->graphicsDesc.longLifetime || destroyWindow){
            vkDestroyPipelineLayout(vk.device, pipeline->layout.pipelineLayout, NULL);
            vkDestroyPipeline(vk.device,pipeline->pipeline, NULL);
            Pool_Remove(&vk.pipelinePool, it.handle);
        }
    }

    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.commandBufferPool, &it)){
        CommandBuffer *commandBuffer = (CommandBuffer*)it.value;
        if(!commandBuffer->longLifetime || destroyWindow){
            vkFreeCommandBuffers(vk.device, commandBuffer->commandPool, 1, &commandBuffer->commandBuffer);
            Pool_Remove(&vk.commandBufferPool, it.handle);
        }
    }

    it = Pool_BeginIteration();
    while(Pool_Iterate(&vk.texturePool, &it)){
        Texture *texture = (Texture*)it.value;
        
        if(!texture->desc.longLifetime || destroyWindow){
            if(!texture->desc.nativeImage){
                vmaDestroyImage(vk.allocator, texture->image, texture->allocation);
            }
            vkDestroyImageView(vk.device, texture->view, NULL);
            Pool_Remove(&vk.texturePool, it.handle);
        }
    }
    
    if(destroyWindow){
        //destroy private resources
        vkDestroyDescriptorPool(vk.device, vk.descriptorPool, NULL);
        vkDestroyCommandPool(vk.device, vk.commandPool, NULL);
        vkDestroySwapchainKHR(vk.device, vk.swapChain, NULL);
        vmaDestroyAllocator(vk.allocator);
        vkDestroyDevice(vk.device, NULL);
        vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
        if (vk.ext.EXT_debug_utils){
            DestroyDebugUtilsMessengerEXT(vk.instance, vk.ext.debugMessenger);
        }
        vkDestroyInstance(vk.instance, NULL);
        Sys_Vulkan_Shutdown();
        vk.initialized = qfalse;
    }

}


void RHI_WaitUntilDeviceIdle()
{
}

rhiSemaphore RHI_CreateBinarySemaphore( void )
{
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &semaphore));
    SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, "binary semaphore");

    Semaphore binarySemaphore = {};
    binarySemaphore.signaled = qfalse;
    binarySemaphore.binary = qtrue;
    binarySemaphore.semaphore = semaphore;
    return (rhiSemaphore) { Pool_Add(&vk.semaphorePool, &binarySemaphore) };
}

rhiSemaphore RHI_CreateTimelineSemaphore( void )
{
    VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
    timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineCreateInfo.pNext = NULL;
    timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue = 0;


    VkSemaphore semaphore;
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = &timelineCreateInfo;
    VK(vkCreateSemaphore(vk.device, &createInfo, NULL, &semaphore));
    SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, "binary semaphore");

    Semaphore timelineSemaphore = {};
    timelineSemaphore.signaled = qfalse;
    timelineSemaphore.binary = qfalse;
    timelineSemaphore.semaphore = semaphore;
    return (rhiSemaphore) { Pool_Add(&vk.semaphorePool, &timelineSemaphore) };
}

void RHI_WaitOnSemaphore(rhiSemaphore semaphore, uint64_t semaphoreValue)
{
    VkSemaphore sem = GET_SEMAPHORE(semaphore)->semaphore;
    VkSemaphoreWaitInfo waitInfo = {};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.pNext = NULL;
    waitInfo.flags = 0;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &sem;
    waitInfo.pValues = &semaphoreValue;
    VK(vkWaitSemaphores(vk.device, &waitInfo, UINT64_MAX));
}

rhiBuffer RHI_CreateBuffer(const rhiBufferDesc *desc)
{
    assert(desc);
    assert(desc->byteCount > 0);
    assert(desc->name);
    assert(desc->allowedStates != 0);
    assert(__popcnt(desc->initialState) == 1);
    assert((desc->initialState & desc->allowedStates) != 0);
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = GetVmaMemoryUsage(desc->memoryUsage); 

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = desc->byteCount;
    bufferInfo.usage = GetVkBufferUsageFlags(desc->allowedStates);

    VmaAllocation vmaAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo = {};

    VkBuffer newBuffer;
    VK(vmaCreateBuffer(vk.allocator, &bufferInfo, &allocCreateInfo, &newBuffer, &vmaAlloc, &allocInfo));
    SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)newBuffer, desc->name);

    Buffer buffer = {};
    buffer.desc = *desc;
    buffer.buffer = newBuffer;
    buffer.allocation = vmaAlloc;
    buffer.mapped = qfalse;
    buffer.mappedData = NULL;
    buffer.memoryUsage = desc->memoryUsage;
    buffer.currentLayout = desc->initialState;
    
    VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(vk.allocator, allocInfo.memoryType, &memFlags);
	buffer.hostCoherent = (memFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != 0;
	// if(buffer.hostCoherent)
	// {
	// 	VK(vmaMapMemory(vk.allocator, buffer.allocation, &buffer.mappedData));
	// 	buffer.mapped = qtrue;
	// }

    return (rhiBuffer) { Pool_Add(&vk.bufferPool, &buffer) };
}

byte* RHI_MapBuffer(rhiBuffer buffer)
{
    Buffer *currentBuffer = GET_BUFFER(buffer);
    assert(currentBuffer->mapped == qfalse);
    VK(vmaMapMemory(vk.allocator, currentBuffer->allocation, &currentBuffer->mappedData));
    currentBuffer->mapped = qtrue;
    return (byte*)currentBuffer->mappedData;
}

void RHI_UnmapBuffer(rhiBuffer buffer)
{
    Buffer *currentBuffer = GET_BUFFER(buffer);
    assert(currentBuffer->mapped == qtrue);
    vmaUnmapMemory(vk.allocator, currentBuffer->allocation);
    if(!(currentBuffer->hostCoherent) && currentBuffer->memoryUsage == RHI_MemoryUsage_Upload){
        vmaFlushAllocation(vk.allocator, currentBuffer->allocation, 0, VK_WHOLE_SIZE);
    } else if(!(currentBuffer->hostCoherent) && currentBuffer->memoryUsage == RHI_MemoryUsage_Readback){
        vmaInvalidateAllocation(vk.allocator, currentBuffer->allocation, 0, VK_WHOLE_SIZE);
    }
    currentBuffer->mapped = qfalse;
}

rhiTexture RHI_CreateTexture(const rhiTextureDesc *desc)
{
    assert(desc);
    assert(desc->width > 0);
    assert(desc->height > 0);
    assert(desc->mipCount > 0);
    assert(desc->sampleCount > 0);
    assert(__popcnt(desc->initialState) == 1);
    assert((desc->initialState & desc->allowedStates) != 0);

    VkFormat format = GetVkFormat(desc->format);
    const qbool ownsImage = desc->nativeImage == VK_NULL_HANDLE;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    if(!ownsImage)
    {
        image = (VkImage)desc->nativeImage;
        format = (VkFormat)desc->nativeFormat;
        layout = (VkImageLayout)desc->nativeLayout;
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
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
		imageInfo.usage = GetVkImageUsageFlags(desc->allowedStates);
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

    Texture texture = {};
    texture.desc = *desc;
    texture.image = image;
    texture.view = view;
    texture.allocation = allocation;
    texture.ownsImage = ownsImage;
    texture.format = format;
    texture.currentLayout = layout;
    return (rhiTexture) { Pool_Add(&vk.texturePool, &texture) };
}

rhiSampler RHI_CreateSampler(const char* name, RHI_TextureAddressing mode, uint32_t anisotropy)
{
    VkSamplerAddressMode wrapMode = (mode == RHI_TextureAddressing_Repeat) ?  VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkSampler sampler;
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.addressModeU = wrapMode;
	createInfo.addressModeV = wrapMode;
	createInfo.addressModeW = wrapMode;
	createInfo.anisotropyEnable = anisotropy > 1 ? VK_TRUE : VK_FALSE;
	createInfo.maxAnisotropy = anisotropy;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 666.0f;
	createInfo.mipLodBias = 0.0f;
	VK(vkCreateSampler(vk.device, &createInfo, NULL, &sampler));

	SetObjectName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler, name);

    Sampler privateSampler;
    privateSampler.sampler = sampler;

    return (rhiSampler) { Pool_Add(&vk.samplerPool, &privateSampler) };
}

void RHI_CreateShader()
{
}





rhiDescriptorSetLayout RHI_CreateDescriptorSetLayout(const rhiDescriptorSetLayoutDesc *desc)
{
    VkDescriptorSetLayoutBinding bindings[ARRAY_LEN(desc->bindings)];
    VkDescriptorBindingFlags bindingFlags[ARRAY_LEN(desc->bindings)];

    for(int i = 0; i < desc->bindingCount; i++){
        assert(desc->bindings[i].stageFlags != 0);
        VkDescriptorSetLayoutBinding *binding = &bindings[i];
        *binding = (VkDescriptorSetLayoutBinding){};
        binding->binding = i;
        binding->descriptorCount = desc->bindings[i].descriptorCount;
        binding->descriptorType = GetVkDescriptorType(desc->bindings[i].descriptorType);
        binding->stageFlags = GetVkShaderStageFlags(desc->bindings[i].stageFlags);
        binding->pImmutableSamplers = VK_NULL_HANDLE;

        bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo descSetFlagsCreateInfo = {};
	descSetFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    descSetFlagsCreateInfo.pBindingFlags = bindingFlags;
    descSetFlagsCreateInfo.bindingCount = desc->bindingCount;

    
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkDescriptorSetLayoutCreateInfo descSetCreateInfo = {};
	descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetCreateInfo.pNext = &descSetFlagsCreateInfo;
    descSetCreateInfo.pBindings = bindings;
    descSetCreateInfo.bindingCount = desc->bindingCount;
	VK(vkCreateDescriptorSetLayout(vk.device, &descSetCreateInfo, NULL, &layout));

	SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)layout, desc->name);

    DescriptorSetLayout descLayout = {};
    descLayout.layout = layout;
    descLayout.desc = *desc;

    return (rhiDescriptorSetLayout) { Pool_Add(&vk.descriptorSetLayoutPool, &descLayout) };
}

rhiDescriptorSet RHI_CreateDescriptorSet(const char *name, rhiDescriptorSetLayout layoutHandle)
{
    DescriptorSetLayout *layout = GET_LAYOUT(layoutHandle);

    VkDescriptorSet descriptorSet;

    VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout->layout;
    
	VK(vkAllocateDescriptorSets(vk.device, &allocInfo, &descriptorSet));
    SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)descriptorSet, name);
    DescriptorSet desc = {};
    desc.set = descriptorSet;
    desc.layout = layoutHandle;

    return (rhiDescriptorSet) { Pool_Add(&vk.descriptorSetPool, &desc) };
}

void RHI_UpdateDescriptorSet(rhiDescriptorSet descriptorHandle, uint32_t bindingIndex, RHI_DescriptorType type, uint32_t offset, uint32_t descriptorCount, const void *handles){
    DescriptorSet *descSet = GET_DESCRIPTORSET(descriptorHandle);
    DescriptorSetLayout *layout = GET_LAYOUT(descSet->layout);

    assert(bindingIndex < layout->desc.bindingCount);
    assert(layout->desc.bindings[bindingIndex].descriptorType == type);
    assert(layout->desc.bindings[bindingIndex].descriptorCount >= offset + descriptorCount);


    VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descSet->set;
	write.dstBinding = bindingIndex;
	write.descriptorCount = descriptorCount;
	write.descriptorType = GetVkDescriptorType(type);
	write.pBufferInfo = NULL;
	write.pImageInfo = NULL;
    write.dstArrayElement = offset;

    static VkDescriptorImageInfo imageInfo[MAX_DRAWIMAGES];
    static VkDescriptorBufferInfo bufferInfo[64];

    if(type == RHI_DescriptorType_ReadOnlyTexture){
        assert(descriptorCount <= ARRAY_LEN(imageInfo));
        const rhiTexture *textureHandle = (const rhiTexture*)handles;
        for(int i = 0; i < descriptorCount; i++){
            Texture *texture = GET_TEXTURE(textureHandle[i]);
            imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[i].imageView = texture->view;
            imageInfo[i].sampler = VK_NULL_HANDLE;
        }
        write.pImageInfo = imageInfo;
        vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);
    } 
    else if(type == RHI_DescriptorType_Sampler){
        assert(descriptorCount <= ARRAY_LEN(imageInfo));
        const rhiSampler *samplerHandle = (const rhiSampler*)handles;
        for(int i = 0; i < descriptorCount; i++){
            imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[i].imageView = VK_NULL_HANDLE;
            imageInfo[i].sampler = GET_SAMPLER(samplerHandle[i])->sampler;
        }
        write.pImageInfo = imageInfo;
        vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);
    }
    else if(type == RHI_DescriptorType_ReadOnlyBuffer){
        assert(descriptorCount <= ARRAY_LEN(bufferInfo));
        const rhiBuffer *bufferHandle = (const rhiBuffer*)handles;
        for(int i = 0; i < descriptorCount; i++){
            bufferInfo[i].buffer = GET_BUFFER(bufferHandle[i])->buffer;
            bufferInfo[i].offset = 0;
            bufferInfo[i].range = VK_WHOLE_SIZE;
        }
       
        write.pBufferInfo = bufferInfo;
        vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);
    }else{
        assert(!"Unhandled descriptor type");
    }




}

VkBool32 isBlendEnabled(uint32_t srcBlend, uint32_t dstBlend){
    if(srcBlend == GLS_SRCBLEND_ONE && dstBlend == GLS_DSTBLEND_ZERO){
        return VK_FALSE;
    }
    if(srcBlend == 0 && dstBlend == 0){
        return VK_FALSE;
    }
    return VK_TRUE;
}


rhiPipeline RHI_CreateGraphicsPipeline(const rhiGraphicsPipelineDesc *graphicsDesc)
{
    assert((graphicsDesc->srcBlend & (~GLS_SRCBLEND_BITS)) == 0);
    assert((graphicsDesc->dstBlend & (~GLS_DSTBLEND_BITS)) == 0);
    DescriptorSetLayout *descriptorSetLayout = GET_LAYOUT(graphicsDesc->descLayout);

    

    VkPushConstantRange pcr[2] = {};
    uint32_t pcrOffset = 0;
    uint32_t pcrCount = 0;
    if(graphicsDesc->pushConstants.vsBytes > 0){
        VkPushConstantRange *range = &pcr[pcrCount++];
        range->size = graphicsDesc->pushConstants.vsBytes;
        range->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        range->offset = pcrOffset;
        pcrOffset += graphicsDesc->pushConstants.vsBytes;
    }
    if(graphicsDesc->pushConstants.psBytes > 0){
        VkPushConstantRange *range = &pcr[pcrCount++];
        range->size = graphicsDesc->pushConstants.psBytes;
        range->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        range->offset = pcrOffset;
    }
    

    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout->layout;
    pipelineLayoutCreateInfo.pPushConstantRanges = pcr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pcrCount;

	VK(vkCreatePipelineLayout(vk.device, &pipelineLayoutCreateInfo, NULL, &vkPipelineLayout));
	SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)vkPipelineLayout, "Pipeline Layout");

    PipelineLayout layout = {};
    layout.pipelineLayout = vkPipelineLayout;


    // --------------------------------------------------------------------------------------------------

    VkShaderModule vsModule;
    VkShaderModuleCreateInfo vsCreateInfo = {};
    vsCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vsCreateInfo.codeSize = graphicsDesc->vertexShader.byteCount;
    vsCreateInfo.pCode = (const uint32_t*)graphicsDesc->vertexShader.data;

    VkShaderModule psModule;
    VkShaderModuleCreateInfo psCreateInfo = {};
    psCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    psCreateInfo.codeSize = graphicsDesc->pixelShader.byteCount;
    psCreateInfo.pCode = (const uint32_t*)graphicsDesc->pixelShader.data;

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


	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = graphicsDesc->wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = GetVkCullModeFlags(graphicsDesc->cullType);
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
    
    if(graphicsDesc->polygonOffset){
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = -1.0f;
        rasterizer.depthBiasSlopeFactor = -1.0f;
    }

	// VkPipelineMultisampleStateCreateInfo multiSampling {};
	// multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// multiSampling.alphaToCoverageEnable = VK_FALSE;


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = isBlendEnabled(graphicsDesc->srcBlend, graphicsDesc->dstBlend);
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcColorBlendFactor = GetSourceColorBlendFactor(graphicsDesc->srcBlend);
	colorBlendAttachment.dstColorBlendFactor = GetDestinationColorBlendFactor(graphicsDesc->dstBlend);
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = GetSourceColorBlendFactor(graphicsDesc->srcBlend);
	colorBlendAttachment.dstAlphaBlendFactor = GetDestinationColorBlendFactor(graphicsDesc->dstBlend);
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
	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAY_LEN(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;


    // Provide information for dynamic rendering
    VkPipelineRenderingCreateInfo pipeline_create = {};
    pipeline_create.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipeline_create.pNext                   = VK_NULL_HANDLE;
    pipeline_create.colorAttachmentCount    = 1;

    VkFormat color_rendering_format = SURFACE_FORMAT_RGBA;
    pipeline_create.pColorAttachmentFormats = &color_rendering_format;
    pipeline_create.depthAttachmentFormat   = GetVkFormat(D32_SFloat);
    pipeline_create.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  
    VkVertexInputAttributeDescription vertexAttributes[ARRAY_LEN(graphicsDesc->attributes)];
    VkVertexInputBindingDescription vertexBindings[ARRAY_LEN(graphicsDesc->attributes)];
    for(int i = 0; i < graphicsDesc->attributeCount; i++){
        VkVertexInputBindingDescription vertexBindingInfo = {};
        vertexBindingInfo.binding = i; //shader binding point
        vertexBindingInfo.stride = graphicsDesc->attributes[i].stride; //only has vertex positions (XYZW)
        vertexBindingInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vertexBindings[i] = vertexBindingInfo;

        VkVertexInputAttributeDescription vertexAttributeInfo = {};
        //a binding can have multiple attributes (interleaved vertex format)
        vertexAttributeInfo.location = i; //shader bindings / shader input location (vk::location in HLSL) 
        vertexAttributeInfo.binding = i; //buffer bindings / vertex buffer index (CmdBindVertexBuffers in C)
        vertexAttributeInfo.format = GetVkFormatFromVertexFormat(graphicsDesc->attributes[i].elementFormat, graphicsDesc->attributes[i].elementCount);
        vertexAttributeInfo.offset = 0; //attribute byte offset

        vertexAttributes[i] = vertexAttributeInfo;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings;
    vertexInputInfo.vertexAttributeDescriptionCount = graphicsDesc->attributeCount;
    vertexInputInfo.vertexBindingDescriptionCount = graphicsDesc->attributeCount;

    VkPipelineMultisampleStateCreateInfo multiSampling = {};
	multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSampling.alphaToCoverageEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthWriteEnable = graphicsDesc->depthWrite;
    depthStencil.depthTestEnable = graphicsDesc->depthTest;
    depthStencil.depthCompareOp = graphicsDesc->depthTestEqual? VK_COMPARE_OP_EQUAL: VK_COMPARE_OP_LESS_OR_EQUAL;


    // Use the pNext to point to the rendering create struct
    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext               = &pipeline_create; // reference the new dynamic structure
    createInfo.renderPass          = VK_NULL_HANDLE; // previously required non-null
    createInfo.layout = layout.pipelineLayout;
	createInfo.subpass = 0; // we always target sub-pass 0
	createInfo.stageCount = stageCount;
	createInfo.pStages = stages + firstStage;
	createInfo.pColorBlendState = &colorBlending;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.pDynamicState = &dynamicState;
    
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pMultisampleState = &multiSampling;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pVertexInputState = &vertexInputInfo; //add later
	createInfo.pViewportState = &viewportState;

    VkPipeline vkPipeline;
    VK(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &createInfo, NULL, &vkPipeline));

    vkDestroyShaderModule(vk.device, psModule, NULL);
    vkDestroyShaderModule(vk.device, vsModule, NULL);

    SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)vkPipeline, graphicsDesc->name);

    Pipeline pipeline = {};
    pipeline.pipeline = vkPipeline;
    pipeline.compute = qfalse;
    pipeline.layout = layout;
    pipeline.pushConstantOffsets[RHI_Shader_Vertex] = 0;
    pipeline.pushConstantOffsets[RHI_Shader_Pixel] = graphicsDesc->pushConstants.vsBytes;
    pipeline.pushConstantSize[RHI_Shader_Vertex] = graphicsDesc->pushConstants.vsBytes;
    pipeline.pushConstantSize[RHI_Shader_Pixel] = graphicsDesc->pushConstants.psBytes;


    return (rhiPipeline) { Pool_Add(&vk.pipelinePool, &pipeline) };

}

void RHI_CreateComputePipeline()
{
}

rhiTexture* RHI_GetSwapChainImages( void )
{
    return vk.swapChainRenderTargets;
}

uint32_t RHI_GetSwapChainImageCount( void ){
    return vk.swapChainImageCount;
}

void RHI_AcquireNextImage(uint32_t* outImageIndex, rhiSemaphore signalSemaphore)
{
    Semaphore* signal = GET_SEMAPHORE(signalSemaphore);
    assert(signal->binary == qtrue);
    assert(signal->signaled == qfalse);

    const VkResult r = vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, signal->semaphore, VK_NULL_HANDLE, outImageIndex); 
    // @TODO: when r is VK_ERROR_OUT_OF_DATE_KHR, recreate the swap chain
    if(r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
    {
        signal->signaled = qtrue;
    }
    else
    {
        Check(r, "vkAcquireNextImageKHR");
        signal->signaled = qfalse;
    }
}


void RHI_SubmitGraphics(const rhiSubmitGraphicsDesc *graphicsDesc)
{

    VkTimelineSemaphoreSubmitInfo timelineInfo;
    timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timelineInfo.pNext = NULL;
    timelineInfo.waitSemaphoreValueCount = graphicsDesc->waitSemaphoreCount;
    timelineInfo.pWaitSemaphoreValues = VK_NULL_HANDLE;
    timelineInfo.signalSemaphoreValueCount = graphicsDesc->signalSemaphoreCount;
    timelineInfo.pSignalSemaphoreValues = graphicsDesc->signalValues;

    VkSemaphore wait[ARRAY_LEN(graphicsDesc->waitSemaphores)];
    VkSemaphore signal[ARRAY_LEN(graphicsDesc->signalSemaphores)];

    for(int i = 0; i < graphicsDesc->waitSemaphoreCount; i++){
        Semaphore *semaphore = GET_SEMAPHORE(graphicsDesc->waitSemaphores[i]);
        wait[i] = semaphore->semaphore;
    
        if(semaphore->binary){
            assert(semaphore->signaled == qtrue);
            semaphore->signaled = qfalse;
        }
    }

    for(int i = 0; i < graphicsDesc->signalSemaphoreCount; i++){
        Semaphore *semaphore = GET_SEMAPHORE(graphicsDesc->signalSemaphores[i]);
        signal[i] = semaphore->semaphore;
        if(semaphore->binary){
            assert(semaphore->signaled == qfalse);
            semaphore->signaled = qtrue;
        }
        
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                = &timelineInfo;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vk.activeCommandBuffer;
    submitInfo.waitSemaphoreCount   = graphicsDesc->waitSemaphoreCount;
    submitInfo.pWaitSemaphores      = wait;
    submitInfo.signalSemaphoreCount = graphicsDesc->signalSemaphoreCount;
    submitInfo.pSignalSemaphores    = signal;
    const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    submitInfo.pWaitDstStageMask = &flags;


    VK(vkQueueSubmit(vk.queues.present, 1, &submitInfo, VK_NULL_HANDLE)); 
}



void RHI_SubmitPresent(rhiSemaphore waitSemaphore, uint32_t swapChainImageIndex)
{
    Semaphore* semaphore = GET_SEMAPHORE(waitSemaphore);
    VkSemaphore wait = semaphore->semaphore;

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vk.swapChain;
    presentInfo.pImageIndices      = &swapChainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &wait;

    assert(semaphore->binary == qtrue);
    assert(semaphore->signaled == qtrue);
    semaphore->signaled = qfalse;

    VK(vkQueuePresentKHR(vk.queues.present, &presentInfo));    
}

rhiCommandBuffer RHI_CreateCommandBuffer( qboolean longLifetime)
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vk.commandPool;
    allocInfo.commandBufferCount = 1;
    VK(vkAllocateCommandBuffers(vk.device, &allocInfo, &commandBuffer));

    CommandBuffer buffer = {};
    buffer.commandPool = vk.commandPool;
    buffer.commandBuffer = commandBuffer;
    buffer.longLifetime = longLifetime;
    return (rhiCommandBuffer) { Pool_Add(&vk.commandBufferPool, &buffer) };
}

void RHI_BindCommandBuffer(rhiCommandBuffer commandBuffer)
{
    CommandBuffer* cmdBuffer = (CommandBuffer*)Pool_Get(&vk.commandBufferPool, commandBuffer.h);
    vk.activeCommandBuffer = cmdBuffer->commandBuffer;
    assert(vk.activeCommandBuffer != NULL);
}


void RHI_BeginCommandBuffer( void )
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    VK(vkBeginCommandBuffer(vk.activeCommandBuffer, &beginInfo));
}

void RHI_EndCommandBuffer()
{
    VK(vkEndCommandBuffer(vk.activeCommandBuffer));
}

void RHI_BeginRendering(const RHI_RenderPass* renderPass)
{
    Texture* colorTexture = GET_TEXTURE(renderPass->colorTexture);
    VkRenderingAttachmentInfo colorAttachmentInfo = {};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = colorTexture->view;
    colorAttachmentInfo.imageLayout = colorTexture->currentLayout;
    colorAttachmentInfo.loadOp = GetVkAttachmentLoadOp(renderPass->colorLoad);
    memcpy(colorAttachmentInfo.clearValue.color.float32, renderPass->color, sizeof(float) * 4);
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachmentInfo = {};
    VkRenderingAttachmentInfo *pDepthAttachmentInfo = NULL;
    if(renderPass->depthTexture.h != 0){
        Texture* depthTexture = GET_TEXTURE(renderPass->depthTexture);
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.imageView = depthTexture->view;
        depthAttachmentInfo.imageLayout = depthTexture->currentLayout;
        depthAttachmentInfo.loadOp = GetVkAttachmentLoadOp(renderPass->depthLoad);
        depthAttachmentInfo.clearValue.depthStencil.depth = renderPass->depth;
        depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pDepthAttachmentInfo = &depthAttachmentInfo;
    }

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = pDepthAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.renderArea.extent.height = glConfig.vidHeight;
    renderingInfo.renderArea.extent.width = glConfig.vidWidth;

    vkCmdBeginRendering(vk.activeCommandBuffer, &renderingInfo);
    vk.renderingActive = qtrue;
    vk.currentRenderPass = *renderPass;
    
}

qboolean RHI_IsRenderingActive(void){
    return vk.renderingActive;
}

RHI_RenderPass* RHI_CurrentRenderPass(void){
    return &vk.currentRenderPass;
}

void RHI_EndRendering(void)
{
    vkCmdEndRendering(vk.activeCommandBuffer);
    vk.renderingActive = qfalse;
}

void RHI_CmdBindPipeline(rhiPipeline pipeline)
{
    Pipeline* privatePipeline = GET_PIPELINE(pipeline);
    VkPipelineBindPoint bindPoint = privatePipeline->compute? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkCmdBindPipeline(vk.activeCommandBuffer, bindPoint, privatePipeline->pipeline);
}

void RHI_CmdBindDescriptorSet(rhiPipeline pipeline, rhiDescriptorSet descriptorSet)
{
    Pipeline *privatePipeline = GET_PIPELINE(pipeline);
    DescriptorSet *privateDescSet = GET_DESCRIPTORSET(descriptorSet);
    VkPipelineBindPoint bindPoint = privatePipeline->compute? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkCmdBindDescriptorSets(vk.activeCommandBuffer, bindPoint, privatePipeline->layout.pipelineLayout, 0, 1, &privateDescSet->set, 0, NULL);
}

void RHI_CmdBindVertexBuffers(const rhiBuffer *vertexBuffers, uint32_t bufferCount)
{
    VkBuffer buffers[16]; 
    VkDeviceSize offsets[16];
    assert(bufferCount < ARRAY_LEN(buffers));
    for(int i = 0; i < bufferCount; i++){
        Buffer *buffer = GET_BUFFER(vertexBuffers[i]);
        buffers[i] = buffer->buffer;
        offsets[i] = 0;
    }
    vkCmdBindVertexBuffers(vk.activeCommandBuffer,0, bufferCount, buffers, offsets);
}

void RHI_CmdBindIndexBuffer(rhiBuffer indexBuffer)
{
    Buffer *privateBuffer = GET_BUFFER(indexBuffer);
    vkCmdBindIndexBuffer(vk.activeCommandBuffer,privateBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
}

void RHI_CmdSetViewport(int32_t x, int32_t y, uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    if(x < 0){
        width = Com_ClampInt(1, width, width + x);
        x = 0;
    }
    if(y < 0){
        height = Com_ClampInt(1, height, height + y);
        y = 0;
    }
    
    VkViewport viewport;
	viewport.x = (float)x;
	viewport.y = (float)y;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;

    vkCmdSetViewport(vk.activeCommandBuffer, 0, 1, &viewport);
}

void RHI_CmdSetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    if(x < 0){
        width = Com_ClampInt(1, width, width + x);
        x = 0;
    }
    if(y < 0){
        height = Com_ClampInt(1, height, height + y);
        y = 0;
    }
    VkRect2D scissor;
	scissor.offset.x = x;
	scissor.offset.y = y;
	scissor.extent.width = width;
	scissor.extent.height = height;

    vkCmdSetScissor(vk.activeCommandBuffer, 0, 1, &scissor);
}

void RHI_CmdPushConstants(rhiPipeline pipeline, RHI_Shader shader, const void *constants, uint32_t byteCount)
{
    Pipeline *privatePipeline = GET_PIPELINE(pipeline);
    assert(byteCount == privatePipeline->pushConstantSize[shader]);
    assert((uint32_t)shader < RHI_Shader_Count);

    vkCmdPushConstants(vk.activeCommandBuffer, privatePipeline->layout.pipelineLayout, GetVkShaderStageFlagsFromShader(shader), privatePipeline->pushConstantOffsets[shader], byteCount, constants);
}

void RHI_CmdDraw(uint32_t vertexCount, uint32_t firstVertex)
{
    vkCmdDraw(vk.activeCommandBuffer, vertexCount, 1, firstVertex, 0);
}

void RHI_CmdDrawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
{
    vkCmdDrawIndexed(vk.activeCommandBuffer, indexCount, 1, firstIndex, firstVertex, 0);
}

void RHI_CmdBeginBarrier()
{
    vk.textureBarrierCount = 0;    
    vk.bufferBarrierCount = 0;  
}

void RHI_CmdEndBarrier()
{
    VkBufferMemoryBarrier2 bufferBarriers[ARRAY_LEN(vk.bufferBarriers)];
    uint32_t bufferBarrierCount = 0;

    for(int i = 0; i < vk.bufferBarrierCount; i++){
        Buffer* buffer = GET_BUFFER(vk.bufferBarriers[i]);
        RHI_ResourceState newLayout = vk.bufferState[i];
        if(buffer->currentLayout == newLayout ){
            continue;
        }
        VkBufferMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;
        barrier.srcAccessMask = GetVkAccessFlagsFromResource(buffer->currentLayout);
        barrier.dstAccessMask = GetVkAccessFlagsFromResource(newLayout);
        barrier.srcStageMask = GetVkStageFlagsFromResource(buffer->currentLayout);
        barrier.dstStageMask = GetVkStageFlagsFromResource(newLayout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer->buffer;

        buffer->currentLayout = newLayout;
        
        bufferBarriers[bufferBarrierCount] = barrier;
        bufferBarrierCount++;

    }


    VkImageMemoryBarrier2 textureBarriers[ARRAY_LEN(vk.textureBarriers)];
    uint32_t textureBarrierCount = 0;
    for(int i = 0; i < vk.textureBarrierCount; i++){
        Texture* texture = GET_TEXTURE(vk.textureBarriers[i]);
        VkImageLayout newLayout = GetVkImageLayout(vk.textureState[i]);
        if(texture->currentLayout == newLayout ){
            continue;
        }
        

        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = texture->image;
        barrier.srcAccessMask = GetVkAccessFlags(texture->currentLayout);
        barrier.dstAccessMask = GetVkAccessFlags(newLayout);
        barrier.srcStageMask = GetVkStageFlags(texture->currentLayout);
        barrier.dstStageMask = GetVkStageFlags(newLayout);
        barrier.oldLayout = texture->currentLayout;
        barrier.newLayout = newLayout;
        barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(texture->format); 
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

        texture->currentLayout = newLayout;
        
        textureBarriers[textureBarrierCount] = barrier;
        textureBarrierCount++;

    }
    
    vk.textureBarrierCount = 0;
    vk.bufferBarrierCount = 0;

    if(textureBarrierCount == 0 && bufferBarrierCount == 0){
        return;
    }


    VkDependencyInfo dep = {};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = textureBarrierCount;
    dep.pImageMemoryBarriers = textureBarriers;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = bufferBarriers;
    vkCmdPipelineBarrier2(vk.activeCommandBuffer, &dep);
}

void RHI_CmdTextureBarrier(rhiTexture handle, RHI_ResourceState flag)
{
    assert(vk.textureBarrierCount < ARRAY_LEN(vk.textureState));
    int idx = vk.textureBarrierCount++;
    vk.textureState[idx] = flag; 
    vk.textureBarriers[idx] = handle;
}

void RHI_CmdBufferBarrier(rhiBuffer handle, RHI_ResourceState state)
{
    assert(vk.bufferBarrierCount < ARRAY_LEN(vk.bufferState));
    int idx = vk.bufferBarrierCount++;
    vk.bufferState[idx] = state; 
    vk.bufferBarriers[idx] = handle;
}

void RHI_CmdCopyBuffer(rhiBuffer dstBuffer, uint32_t dstOffset, rhiBuffer srcBuffer, uint32_t srcOffset, uint32_t size)
{
    VkBufferCopy region = {};
    region.dstOffset = dstOffset;
    region.srcOffset = srcOffset;
    region.size = size;

    
    
    vkCmdCopyBuffer(vk.activeCommandBuffer, GET_BUFFER(srcBuffer)->buffer, GET_BUFFER(dstBuffer)->buffer, 1, &region);
}

void RHI_CmdCopyBufferRegions()
{
}


void RHI_CmdInsertDebugLabel()
{
    
}

void RHI_CmdBeginDebugLabel()
{
}

void RHI_CmdEndDebugLabel()
{
}

void RHI_CmdBeginDurationQuery()
{
}

void RHI_CmdEndDurationQuery()
{
}

void RHI_CmdResetDurationQueries()
{
}

void RHI_ResolveDurationQuery()
{
}

void RHI_BeginBufferUpload()
{
}

void RHI_EndBufferUpload()
{
}

void RHI_BeginTextureUpload(rhiTextureUpload *textureUpload, rhiTexture handle, uint32_t mipLevel)
{
    assert(vk.uploadTextureHandle.h == 0);
    //use vk.deviceProperties.limits.optimalBufferCopyRowPitchAlignment
    //use vk.deviceProperties.limits.optimalBufferCopyOffsetAlignment
    Texture *texture = GET_TEXTURE(handle);

    textureUpload->data = RHI_MapBuffer(vk.uploadBuffer);
    textureUpload->width = max(texture->desc.width >> mipLevel, 1);
    textureUpload->height = max(texture->desc.height >> mipLevel, 1);
    textureUpload->rowPitch = textureUpload->width * GetByteCountsPerPixel(texture->format); 
    vk.uploadTextureHandle = handle;
    vk.uploadTextureMipLevel = mipLevel;

}

void RHI_EndTextureUpload()
{
    RHI_UnmapBuffer(vk.uploadBuffer);
    VkCommandBuffer previousCmdBuffer = vk.activeCommandBuffer;
    RHI_BindCommandBuffer(vk.uploadCmdBuffer); //@TODO: return VkCommandBuffer to assign previous?
    Texture *texture = GET_TEXTURE(vk.uploadTextureHandle);
    RHI_BeginCommandBuffer();
    RHI_CmdBeginBarrier();
    RHI_CmdBufferBarrier(vk.uploadBuffer, RHI_ResourceState_CopySourceBit);
    RHI_CmdTextureBarrier(vk.uploadTextureHandle, RHI_ResourceState_CopyDestinationBit);
    RHI_CmdEndBarrier();

    // copy from the staging buffer into the texture mip
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = vk.uploadTextureMipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = max(texture->desc.width >> vk.uploadTextureMipLevel, 1);
    region.imageExtent.height = max(texture->desc.height >> vk.uploadTextureMipLevel, 1);
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(vk.activeCommandBuffer, GET_BUFFER(vk.uploadBuffer)->buffer, texture->image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    RHI_CmdBeginBarrier();
    RHI_CmdTextureBarrier(vk.uploadTextureHandle, texture->desc.initialState);
    RHI_CmdEndBarrier();

    RHI_EndCommandBuffer();

    rhiSubmitGraphicsDesc submitDesc = {};
    RHI_SubmitGraphics(&submitDesc);
    vkDeviceWaitIdle(vk.device); //@TODO remove - signal timeline semaphore, wait on sem before uploading 
    
    vk.uploadTextureHandle.h = 0;
    vk.activeCommandBuffer = previousCmdBuffer;
}

void RHI_GetUploadSemaphore()
{
}

void RHI_PrintPools(void){
    ri.Printf(PRINT_ALL, "%d in vk.commandBufferPool\n", Pool_Size(&vk.commandBufferPool));
    ri.Printf(PRINT_ALL, "%d in vk.semaphorePool\n", Pool_Size(&vk.semaphorePool));
    ri.Printf(PRINT_ALL, "%d in vk.texturePool\n", Pool_Size(&vk.texturePool));
    ri.Printf(PRINT_ALL, "%d in vk.bufferPool\n", Pool_Size(&vk.bufferPool));
    ri.Printf(PRINT_ALL, "%d in vk.descriptorSetLayoutPool\n", Pool_Size(&vk.descriptorSetLayoutPool));
    ri.Printf(PRINT_ALL, "%d in vk.descriptorSetPool\n", Pool_Size(&vk.descriptorSetPool));
    ri.Printf(PRINT_ALL, "%d in vk.pipelinePool\n", Pool_Size(&vk.pipelinePool));
    ri.Printf(PRINT_ALL, "%d in vk.samplerPool\n", Pool_Size(&vk.samplerPool));
}

