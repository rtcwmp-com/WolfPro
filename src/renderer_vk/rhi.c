#include "rhi.h"
#include "tr_vulkan.h"

#define GET_SEMAPHORE(handle) ((Semaphore*)Pool_Get(&vk.semaphorePool, handle.h))
#define GET_TEXTURE(handle) ((Texture*)Pool_Get(&vk.texturePool, handle.h))
#define GET_BUFFER(handle) ((Buffer*)Pool_Get(&vk.bufferPool, handle.h))
#define GET_LAYOUT(handle) ((DescriptorSetLayout*)Pool_Get(&vk.descriptorSetLayoutPool, handle.h))
#define GET_PIPELINE(handle) ((Pipeline*)Pool_Get(&vk.pipelinePool, handle.h))
#define GET_DESCRIPTORSET(handle) ((DescriptorSet*)Pool_Get(&vk.descriptorSetPool, handle.h))
void RHI_Init()
{
}

void RHI_Shutdown()
{
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
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = GetVmaMemoryUsage(desc->memoryUsage); 

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = desc->byteCount;
    bufferInfo.usage = GetVkBufferUsageFlags(desc->initialState);

    VmaAllocation vmaAlloc = {};
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
        assert(0);
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

void RHI_CreateSampler()
{
}

void RHI_CreateShader()
{
}

rhiDescriptorSetLayout RHI_CreateDescriptorSetLayout()
{

    VkDescriptorSetLayoutBindingFlagsCreateInfo descSetFlagsCreateInfo = {};
	descSetFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

    
    VkDescriptorSetLayout layout = {};
	VkDescriptorSetLayoutCreateInfo descSetCreateInfo = {};
	descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetCreateInfo.pNext = &descSetFlagsCreateInfo;
	VK(vkCreateDescriptorSetLayout(vk.device, &descSetCreateInfo, NULL, &layout));

	SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)layout, "Desc Layout");

    DescriptorSetLayout descLayout = {};
    descLayout.layout = layout;

    return (rhiDescriptorSetLayout) { Pool_Add(&vk.descriptorSetLayoutPool, &descLayout) };
}

void RHI_CreateDescriptorSet()
{
}

#include "shaders/triangle_ps.h"
#include "shaders/triangle_vs.h"

rhiPipeline RHI_CreateGraphicsPipeline(rhiDescriptorSetLayout descLayout)
{
    DescriptorSetLayout *descriptorSetLayout = GET_LAYOUT(descLayout);

    VkPushConstantRange pcr = {};
    pcr.size = 4;
    pcr.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayout vkPipelineLayout = {};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout->layout;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	VK(vkCreatePipelineLayout(vk.device, &pipelineLayoutCreateInfo, NULL, &vkPipelineLayout));
	SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)vkPipelineLayout, "Pipeline Layout");

    PipelineLayout layout = {};
    layout.pipelineLayout = vkPipelineLayout;

    

    // --------------------------------------------------------------------------------------------------

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
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // @TODO:
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // @TODO:
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

    VkVertexInputBindingDescription tcBindingInfo = {};
    tcBindingInfo.binding = 2; //shader binding point
    tcBindingInfo.stride = 2 * sizeof(float); //
    tcBindingInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    

    VkVertexInputAttributeDescription vertexPositionAttributeInfo = {};
    //a binding can have multiple attributes (interleaved vertex format)
    vertexPositionAttributeInfo.location = 0; //shader bindings / shader input location (vk::location in HLSL) 
    vertexPositionAttributeInfo.binding = vertexPositionBindingInfo.binding; //buffer bindings / vertex buffer index (CmdBindVertexBuffers in C)
    vertexPositionAttributeInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexPositionAttributeInfo.offset = 0; //attribute byte offset

    VkVertexInputAttributeDescription colorAttributeInfo = {};
    //a binding can have multiple attributes (interleaved vertex format)
    colorAttributeInfo.location = 1; //shader bindings / shader input location (vk::location in HLSL) 
    colorAttributeInfo.binding = colorBindingInfo.binding; //buffer bindings / vertex buffer index (CmdBindVertexBuffers in C)
    colorAttributeInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttributeInfo.offset = 0; //attribute byte offset

    VkVertexInputAttributeDescription tcAttributeInfo = {};
    //a binding can have multiple attributes (interleaved vertex format)
    tcAttributeInfo.location = 2; //shader bindings / shader input location (vk::location in HLSL) 
    tcAttributeInfo.binding = tcBindingInfo.binding; //buffer bindings / vertex buffer index (CmdBindVertexBuffers in C)
    tcAttributeInfo.format = VK_FORMAT_R32G32_SFLOAT;
    tcAttributeInfo.offset = 0; //attribute byte offset

    VkVertexInputAttributeDescription vertexAttributes[3] = {vertexPositionAttributeInfo,  colorAttributeInfo, tcAttributeInfo};
    VkVertexInputBindingDescription vertexBindings[3] = {vertexPositionBindingInfo, colorBindingInfo, tcBindingInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings;
    vertexInputInfo.vertexAttributeDescriptionCount = ARRAY_LEN(vertexAttributes);
    vertexInputInfo.vertexBindingDescriptionCount = ARRAY_LEN(vertexBindings);

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
    createInfo.layout = layout.pipelineLayout;
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

    VkPipeline vkPipeline;
    VK(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &createInfo, NULL, &vkPipeline));

    SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)vkPipeline, "Pipeline");

    Pipeline pipeline = {};
    pipeline.pipeline = vkPipeline;
    pipeline.compute = qfalse;
    pipeline.layout = layout;

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

rhiCommandBuffer RHI_CreateCommandBuffer( void ) //pass queue enum
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
    return (rhiCommandBuffer) { Pool_Add(&vk.commandBufferPool, &buffer) };
}

void RHI_BindCommandBuffer(rhiCommandBuffer commandBuffer)
{
    CommandBuffer* cmdBuffer = (CommandBuffer*)Pool_Get(&vk.commandBufferPool, commandBuffer.h);
    vk.activeCommandBuffer = cmdBuffer->commandBuffer;
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
    
}

void RHI_EndRendering()
{
    vkCmdEndRendering(vk.activeCommandBuffer);
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

void RHI_CmdSetViewport()
{
}

void RHI_CmdSetScissor()
{
}

void RHI_CmdPushConstants(rhiPipeline pipeline, const void *constants, uint32_t byteCount)
{
    Pipeline *privatePipeline = GET_PIPELINE(pipeline);
    assert(byteCount == 64);
    //TODO: update to dynamically choose shader stage and size
    vkCmdPushConstants(vk.activeCommandBuffer, privatePipeline->layout.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, byteCount, constants);
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
}

void RHI_CmdEndBarrier()
{
    VkImageMemoryBarrier2 barriers[2048];
    uint32_t barrierCount = 0;
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
        
        barriers[barrierCount] = barrier;
        barrierCount++;

    }
    
    if(barrierCount == 0){
        return;
    }

    VkDependencyInfo dep = {};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = barrierCount;
    dep.pImageMemoryBarriers = barriers;
    vkCmdPipelineBarrier2(vk.activeCommandBuffer, &dep);
}

void RHI_CmdTextureBarrier(rhiTexture handle, RHI_ResourceState flag)
{
    int idx = vk.textureBarrierCount++;
    vk.textureState[idx] = flag; 
    vk.textureBarriers[idx] = handle;
}

void RHI_CmdBufferBarrier()
{
}

void RHI_CmdCopyBuffer()
{

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

void RHI_BeginTextureUpload()
{
}

void RHI_EndTextureUpload()
{
}

void RHI_GetUploadSemaphore()
{
}
