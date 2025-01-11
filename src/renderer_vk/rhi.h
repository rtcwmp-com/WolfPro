

void RHI_Init();
void RHI_Shutdown();

void RHI_WaitUntilDeviceIdle(); //debug only

void RHI_CreateBinarySemaphore();
void RHI_CreateTimelineSemaphore();
void RHI_WaitOnSempaphore();

void RHI_CreateBuffer();
void RHI_MapBuffer(); //cpu visible buffers
void RHI_UnmapBuffer();

void RHI_CreateTexture();

void RHI_CreateSampler();

void RHI_CreateShader();

void RHI_CreateDescriptorSetLayout();
void RHI_CreateDescriptorSet();

void RHI_CreateGraphicsPipeline();
void RHI_CreateComputePipeline();

void RHI_GetSwapChainInfo();
void RHI_AcquireNextImage();

void RHI_SubmitGraphics();
void RHI_SubmitPresent();

void RHI_CreateCommandBuffer();
void RHI_BindCommandBuffer();
void RHI_BeginCommandBuffer();
void RHI_EndCommandBuffer();

void RHI_BeginRendering(); //bind render targets
void RHI_EndRendering();

void RHI_CmdBindPipeline(); //create pipeline layout here
void RHI_CmdBindDescriptorSet();
void RHI_CmdBindVertexBuffers();
void RHI_CmdBindIndexBuffer();
void RHI_CmdSetViewport();
void RHI_CmdSetScissor();
void RHI_CmdPushConstants();
void RHI_CmdDraw();
void RHI_CmdDrawIndexed();

void RHI_CmdBeginBarrier();
void RHI_CmdEndBarrier();
void RHI_CmdTextureBarrier(); //pass handle to this
void RHI_CmdBufferBarrier(); //pass handle to this

void RHI_CmdCopyBuffer();
void RHI_CmdCopyBufferRegions();
void RHI_CmdClearColorTexture();
void RHI_CmdInsertDebugLabel();
void RHI_CmdBeginDebugLabel();
void RHI_CmdEndDebugLabel();

void RHI_CmdBeginDurationQuery();
void RHI_CmdEndDurationQuery();
void RHI_CmdResetDurationQueries();
void RHI_ResolveDurationQuery();

//upload manager
void RHI_BeginBufferUpload();
void RHI_EndBufferUpload();
void RHI_BeginTextureUpload();
void RHI_EndTextureUpload();
void RHI_GetUploadSemaphore(); //(semaphore handle, value to wait on)


