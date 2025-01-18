#include "rhi.h"
#include "tr_vulkan.h"

#define GET_SEMAPHORE(handle) ((Semaphore*)Pool_Get(&vk.semaphorePool, handle.h))
#define GET_TEXTURE(handle) ((Texture*)Pool_Get(&vk.texturePool, handle.h))

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

void RHI_CreateBuffer()
{
}

void RHI_MapBuffer()
{
}

void RHI_UnmapBuffer()
{
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

void RHI_CreateDescriptorSetLayout()
{
}

void RHI_CreateDescriptorSet()
{
}

void RHI_CreateGraphicsPipeline()
{
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

void RHI_BeginRendering()
{
}

void RHI_EndRendering()
{
}

void RHI_CmdBindPipeline()
{
}

void RHI_CmdBindDescriptorSet()
{
}

void RHI_CmdBindVertexBuffers()
{
}

void RHI_CmdBindIndexBuffer()
{
}

void RHI_CmdSetViewport()
{
}

void RHI_CmdSetScissor()
{
}

void RHI_CmdPushConstants()
{
}

void RHI_CmdDraw()
{
}

void RHI_CmdDrawIndexed()
{
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
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //TODO depth buffer VK_IMAGE_ASPECT_DEPTH_BIT
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        
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

void RHI_CmdTextureBarrier(rhiTexture handle, rhiResourceStateFlags flag)
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

void RHI_CmdClearColorTexture()
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
