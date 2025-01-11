#include "rhi.h"


void RHI_Init()
{
}

void RHI_Shutdown()
{
}

void RHI_WaitUntilDeviceIdle()
{
}

void RHI_CreateBinarySemaphore()
{
}

void RHI_CreateTimelineSemaphore()
{
}

void RHI_WaitOnSempaphore()
{
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

void RHI_AcquireNextImage()
{
}

void RHI_SubmitGraphics()
{
}

void RHI_SubmitPresent()
{
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
    rhiCommandBuffer cmdBuffer;
    cmdBuffer.h = Pool_Add(&vk.commandBufferPool, &buffer);
    return cmdBuffer;
}

void RHI_BindCommandBuffer(rhiCommandBuffer commandBuffer)
{
    CommandBuffer* cmdBuffer = (CommandBuffer*)Pool_Get(&vk.commandBufferPool, commandBuffer.v);
    vk.activeCommandBuffer = cmdBuffer->buffer;
}

void RHI_BeginCommandBuffer()
{
}

void RHI_EndCommandBuffer()
{
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
