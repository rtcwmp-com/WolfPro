#ifndef RHI_H
#define RHI_H
//#pragma once 

#include "../game/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"

#define RHI_FRAMES_IN_FLIGHT			2


// Video initialization:
// - loading Vulkan
// - creating a window and changing video mode if needed,
// - respecting r_fullscreen, r_mode, r_width, r_height
// - creating a valid Vulkan surface
// - filling up the right glconfig fields (see glconfig_t definition)
// returns the surface handle
uint64_t Sys_Vulkan_Init(void* vkInstance);

typedef uint64_t rhiHandle;

#define RHI_HANDLE_TYPE(TypeName) typedef struct { rhiHandle h; }  TypeName;

RHI_HANDLE_TYPE(rhiSemaphore) //either binary or timeline semaphore
RHI_HANDLE_TYPE(rhiTexture) //1d,2d,3d data that can be filtered when read (render target when writing from graphics pipeline operation)
RHI_HANDLE_TYPE(rhiCommandPool) //memory region for allocating command buffers, once in init
RHI_HANDLE_TYPE(rhiCommandBuffer) //list of commands to be executed on the gpu
									//allocated from cmd pool in init, submitted to a queue
									//clear command buffer every frame
									//submit to gpu every frame
RHI_HANDLE_TYPE(rhiDescriptorSetLayout) //descriptor is any resource you use from a shader (buffers/textures/samplers can be descriptors)
										//layout describes which range in the set has which type of resource
										//ex: read only textures, read-write, samplers, RO/RW/WO buffers
RHI_HANDLE_TYPE(rhiPipeline) //pipeline state object (PSO) all of the state of the graphics pipeline 
							// (what is expensive to change on the GPU)
							// describes what state the pipeline needs to be in
							// viewport, scissor rect, cull mode (*?) are not in the pipeline state
							// viewport related to the projection matrix
							// scissor rect is where you are allowed to write (outside of this does not get written to render target)
RHI_HANDLE_TYPE(rhiShader) //programmable logic to run on the gpu (vertex/fragment shading) multiple fragments can contribute to a pixel (MSAA)
RHI_HANDLE_TYPE(rhiPipelineLayout) // push constant's byte count for both shader stages; which descriptor set layouts you have
								   // push constant - constant data (up to 128 bytes) in the command stream, for the entire draw call
								   // ex: matrices in vertex shader, texture/sampler indices for bindless rendering in frag shader
RHI_HANDLE_TYPE(rhiBuffer) //raw data in gpu space (vertex/index can be specailized for the API's knowledge of what is in it)





#define RHI_BIT(x)						(1 << x)


typedef struct rhiCommandPoolDesc
{
	qbool presentQueue;
	qbool transient;
} rhiCommandPoolDesc;

typedef enum rhiResourceStateFlags {
        rhiResourceStateUndefined = 0,
        VertexBufferBit = RHI_BIT(0),
        IndexBufferBit = RHI_BIT(1),
        RenderTargetBit = RHI_BIT(2),
        PresentBit = RHI_BIT(3),
        ShaderInputBit = RHI_BIT(4),
        CopySourceBit = RHI_BIT(5),
        CopyDestinationBit = RHI_BIT(6),
        DepthWriteBit = RHI_BIT(7),
        UnorderedAccessBit = RHI_BIT(8),
        CommonBit = RHI_BIT(9),
        IndirectCommandBit = RHI_BIT(10),
        rhiResourceStateUniformBufferBit = RHI_BIT(11),
        rhiResourceStateStorageBufferBit = RHI_BIT(12),
        DepthReadBit = RHI_BIT(13)
} rhiResourceStateFlags;





typedef enum rhiDescriptorTypeFlags
{
	rhiDescriptorTypeFlagUndefined,
	SamplerBit = RHI_BIT(1),
	CombinedImageSamplerBit = RHI_BIT(2),
	SampledImageBit = RHI_BIT(3),
	rhiDescriptorTypeUniformBufferBit = RHI_BIT(4),
	rhiDescriptorTypeStorageBufferBit = RHI_BIT(5),
	InputAttachmentBit = RHI_BIT(6),
	ReadWriteImageBit = RHI_BIT(7)
} rhiDescriptorTypeFlags;

typedef enum rhiTextureFormatId
{
	R8G8B8A8_UNorm,
	B8G8R8A8_UNorm,
	B8G8R8A8_sRGB,
	D16_UNorm,
	D32_SFloat,
	//D24_UNorm_S8_UInt, // this is not well supported (:wave: AMD)
	R32_UInt,
	Count
} rhiTextureFormatId;

typedef struct rhiTextureDesc
{
	uint64_t nativeImage;
    uint64_t nativeFormat;
	const char* name;
	uint32_t width;
	uint32_t height;
	uint32_t mipCount;
	uint32_t sampleCount;
	rhiResourceStateFlags initialState;
	rhiDescriptorTypeFlags descriptorType;
	rhiTextureFormatId format;
} rhiTextureDesc;

typedef struct PushConstantsRange
{
	uint32_t byteOffset;
	uint32_t byteCount;
} PushConstantsRange; 

typedef enum rhiShaderTypeId
{
	rhiShaderTypeIdVertex,
	rhiShaderTypeIdPixel,
	rhiShaderTypeIdCompute,
	rhiShaderTypeIdCount
} rhiShaderTypeId;

typedef struct rhiPipelineLayoutDesc
{
	PushConstantsRange pushConstantsPerStage[rhiShaderTypeIdCount];
	const char* name;
	const rhiDescriptorSetLayout* descriptorSetLayouts;
	uint32_t descriptorSetLayoutCount;
} rhiPipelineLayoutDesc;

typedef enum rhiDataTypeId
{
	Float32,
	UNorm8,
	UInt32,
	rhiDataTypeIdCount
} rhiDataTypeId;

typedef struct rhiVertexAttribDesc
{
	uint32_t binding;
	uint32_t location;
	rhiDataTypeId dataType;
	uint32_t vectorLength;
	uint32_t byteOffset;
} rhiVertexAttribDesc;

typedef struct rhiVertexBindingDesc
{
	uint32_t binding;
	uint32_t stride;
} rhiVertexBindingDesc;

typedef struct VertexLayout
{
	const rhiVertexAttribDesc* attribs;
	const rhiVertexBindingDesc* bindings;
	uint32_t attribCount;
	uint32_t bindingCount;
} VertexLayout;

typedef struct rhiSpecializationEntry
{
	uint32_t constandId;
	uint32_t byteOffset;
	uint32_t byteCount;
} rhiSpecializationEntry;

typedef struct rhiSpecialization
{
	const rhiSpecializationEntry* entries;
	const void* data;
	uint32_t entryCount;
	uint32_t byteCount;
} rhiSpecialization;

typedef struct rhiGraphicsPipelineDesc
{
	const char* name;
	
	VertexLayout vertexLayout;
	rhiShader vertexShader;
	rhiShader fragmentShader;
	rhiSpecialization vertexSpec;
	rhiSpecialization fragmentSpec;
	rhiPipelineLayout pipelineLayout;
	uint32_t cullType; // cullType_t
	uint32_t srcBlend; // stateBits & GLS_SRCBLEND_BITS
	uint32_t dstBlend; // stateBits & GLS_DSTBLEND_BITS
	uint32_t depthTest; // depthTest_t
	qbool enableDepthWrite;
	qbool enableDepthTest;
} rhiGraphicsPipelineDesc;

typedef enum 
{
	DeviceLocal,
	Upload,
	Readback,
	rhiMemoryUsageIdCount
} rhiMemoryUsageId;

typedef struct rhiBufferDesc
{
	const char* name;
	uint32_t byteCount;
	rhiResourceStateFlags initialState;
	rhiMemoryUsageId memoryUsage;
} rhiBufferDesc;

typedef struct rhiSubmitGraphicsDesc {
    rhiSemaphore signalSemaphores[8];
    uint16_t signalSemaphoreCount;
    uint64_t signalValues[8];
    rhiSemaphore waitSemaphores[8];
    uint16_t waitSemaphoreCount;
} rhiSubmitGraphicsDesc;

typedef enum {
	ImageLayoutUndefined = 0,
	ImageLayoutGeneral
} rhiImageLayoutFormat;

inline void RHI_SubmitGraphicsDesc_Signal(rhiSubmitGraphicsDesc *graphicsDesc, rhiSemaphore semaphore, uint64_t semaphoreValue){
    assert(graphicsDesc->signalSemaphoreCount < ARRAY_LEN(graphicsDesc->signalSemaphores));
    int newIndex = graphicsDesc->signalSemaphoreCount++;
    graphicsDesc->signalSemaphores[newIndex] = semaphore;
    graphicsDesc->signalValues[newIndex] = semaphoreValue;
}

inline void RHI_SubmitGraphicsDesc_Wait(rhiSubmitGraphicsDesc *graphicsDesc, rhiSemaphore semaphore){
    assert(graphicsDesc->waitSemaphoreCount < ARRAY_LEN(graphicsDesc->waitSemaphores));
    int newIndex = graphicsDesc->waitSemaphoreCount++;
    graphicsDesc->waitSemaphores[newIndex] = semaphore;
}


void RHI_Init();
void RHI_Shutdown();

void RHI_WaitUntilDeviceIdle(); //debug only

rhiSemaphore RHI_CreateBinarySemaphore();
rhiSemaphore RHI_CreateTimelineSemaphore();
void RHI_WaitOnSemaphore(rhiSemaphore semaphore, uint64_t semaphoreValue);

void RHI_CreateBuffer();
void RHI_MapBuffer(); //cpu visible buffers
void RHI_UnmapBuffer();

rhiTexture RHI_CreateTexture(const rhiTextureDesc *desc);

void RHI_CreateSampler();

void RHI_CreateShader();

void RHI_CreateDescriptorSetLayout();
void RHI_CreateDescriptorSet();

void RHI_CreateGraphicsPipeline();
void RHI_CreateComputePipeline();


rhiTexture* RHI_GetSwapChainImages( void );
uint32_t RHI_GetSwapChainImageCount( void );

void RHI_AcquireNextImage(uint32_t* outImageIndex, rhiSemaphore signalSemaphore);


void RHI_SubmitGraphics(const rhiSubmitGraphicsDesc* graphicsDesc);
void RHI_SubmitPresent(rhiSemaphore waitSemaphore, uint32_t swapChainImageIndex);

rhiCommandBuffer RHI_CreateCommandBuffer(void); // pass queue enum as argument
void RHI_BindCommandBuffer(rhiCommandBuffer cmdBuffer);
void RHI_BeginCommandBuffer( void );
void RHI_EndCommandBuffer( void );

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

void RHI_CmdBeginBarrier( void );
void RHI_CmdEndBarrier( void );
void RHI_CmdTextureBarrier(rhiTexture handle, rhiResourceStateFlags flag); //pass handle to this
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


#endif