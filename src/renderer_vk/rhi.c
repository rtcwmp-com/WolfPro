#include "rhi.h"
#include "tr_vulkan.h"

#define GET_SEMAPHORE(handle) ((Semaphore*)Pool_Get(&vk.semaphorePool, handle.h))


void RHI_Init()
{
}

void RHI_Shutdown()
{
}

void RHI_WaitUntilDeviceIdle()
{
}

rhiSemaphore RHI_CreateBinarySemaphore()
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

rhiSemaphore RHI_CreateTimelineSemaphore()
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

void RHI_CreateTexture()
{
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

void RHI_GetSwapChainInfo()
{
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
        wait[i] = GET_SEMAPHORE(graphicsDesc->waitSemaphores[i])->semaphore;
    }

    for(int i = 0; i < graphicsDesc->signalSemaphoreCount; i++){
        signal[i] = GET_SEMAPHORE(graphicsDesc->signalSemaphores[i])->semaphore;
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
    VkSemaphore wait = GET_SEMAPHORE(waitSemaphore)->semaphore;

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vk.swapChain;
    presentInfo.pImageIndices      = &swapChainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &wait;

    VK(vkQueuePresentKHR(vk.queues.present, &presentInfo));    
}

rhiCommandBuffer RHI_CreateCommandBuffer() //pass queue enum
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

void RHI_BeginCommandBuffer()
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
}

void RHI_CmdEndBarrier()
{
}

void RHI_CmdTextureBarrier()
{
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
