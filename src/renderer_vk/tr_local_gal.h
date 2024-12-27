
#ifndef TR_LOCAL_GAL
#define TR_LOCAL_GAL
#include "../renderer/tr_local.h"
// Video initialization:
// - loading Vulkan
// - creating a window and changing video mode if needed,
// - respecting r_fullscreen, r_mode, r_width, r_height
// - creating a valid Vulkan surface
// - filling up the right glconfig fields (see glconfig_t definition)
// returns the surface handle
uint64_t Sys_Vulkan_Init(void* vkInstance);

typedef uint32_t galHandle;

#define GAL_HANDLE_TYPE(TypeName) typedef struct { galHandle v; }  TypeName;

GAL_HANDLE_TYPE(galFence) //sync cpu and gpu, allows you to wait on a queue on CPU for the GPU to finish work
GAL_HANDLE_TYPE(galTexture) //1d,2d,3d data that can be filtered when read (render target when writing from graphics pipeline operation)
GAL_HANDLE_TYPE(galCommandPool) //memory region for allocating command buffers, once in init
GAL_HANDLE_TYPE(galCommandBuffer) //list of commands to be executed on the gpu
								//allocated from cmd pool in init, submitted to a queue
								//clear command buffer every frame
								//submit to gpu every frame

#define GAL_FRAMES_IN_FLIGHT			2
#define GAL_BIT(x)						(1 << x)


typedef struct 
{
	qbool presentQueue;
	qbool transient;
} galCommandPoolDesc;

typedef enum 
{
	galResourceStateFlagUndefined = 0,
	VertexBufferBit = GAL_BIT(0),
	IndexBufferBit = GAL_BIT(1),
	RenderTargetBit = GAL_BIT(2),
	PresentBit = GAL_BIT(3),
	ShaderInputBit = GAL_BIT(4),
	CopySourceBit = GAL_BIT(5),
	CopyDestinationBit = GAL_BIT(6),
	DepthWriteBit = GAL_BIT(7),
	UnorderedAccessBit = GAL_BIT(8),
	CommonBit = GAL_BIT(9),
	IndirectCommandBit = GAL_BIT(10),
	galResourceStateUniformBufferBit = GAL_BIT(11),
	galResourceStateStorageBufferBit = GAL_BIT(12),
	DepthReadBit = GAL_BIT(13)
} galResourceStateFlags;


typedef enum
{
	galDescriptorTypeFlagUndefined,
	SamplerBit = GAL_BIT(1),
	CombinedImageSamplerBit = GAL_BIT(2),
	SampledImageBit = GAL_BIT(3),
	galDescriptorTypeUniformBufferBit = GAL_BIT(4),
	galDescriptorTypeStorageBufferBit = GAL_BIT(5),
	InputAttachmentBit = GAL_BIT(6),
	ReadWriteImageBit = GAL_BIT(7)
} galDescriptorTypeFlags;

typedef enum
{
	R8G8B8A8_UNorm,
	B8G8R8A8_UNorm,
	B8G8R8A8_sRGB,
	D16_UNorm,
	D32_SFloat,
	//D24_UNorm_S8_UInt, // this is not well supported (:wave: AMD)
	R32_UInt,
	Count
} galTextureFormatId;

typedef struct 
{
	uint64_t nativeImage;
	const char* name;
	uint32_t width;
	uint32_t height;
	uint32_t mipCount;
	uint32_t sampleCount;
	galResourceStateFlags initialState;
	galDescriptorTypeFlags descriptorType;
	galTextureFormatId format;
} galTextureDesc;

#if 0



#define GAL_FRAMES_IN_FLIGHT			3
#define GAL_MAX_BOUND_COLOR_TARGETS		4
#define GAL_NULL_HANDLE					{ 0 }
#define GAL_BIT(x)						(1 << x)

typedef uint32_t galHandle;


#define GAL_HANDLE_TYPE(TypeName) struct TypeName { galHandle v; }; 
//	inline qbool isEqual_##TypeName(TypeName a, TypeName b) { return a.v == b.v; } \
//	inline qbool notEqual_##TypeName(TypeName a, TypeName b) { return a.v != b.v; }
GAL_HANDLE_TYPE(galFence)
GAL_HANDLE_TYPE(galSemaphore)
GAL_HANDLE_TYPE(galTexture)
GAL_HANDLE_TYPE(galSampler)
GAL_HANDLE_TYPE(galBuffer)
GAL_HANDLE_TYPE(galShader)
GAL_HANDLE_TYPE(galDescriptorSetLayout)
GAL_HANDLE_TYPE(galPipelineLayout)
GAL_HANDLE_TYPE(galDescriptorSet)
GAL_HANDLE_TYPE(galPipeline)
GAL_HANDLE_TYPE(galCommandPool)
GAL_HANDLE_TYPE(galCommandBuffer)
GAL_HANDLE_TYPE(galDurationQuery)

/*
#define GAL_ENUM_OPERATORS(EnumType) \
	inline EnumType operator|(EnumType a, EnumType b) { return (EnumType)((uint32_t)(a) | (uint32_t)(b)); } \
	inline EnumType operator&(EnumType a, EnumType b) { return (EnumType)((uint32_t)(a) & (uint32_t)(b)); } \
	inline EnumType operator|=(EnumType& a, EnumType b) { return a = (a | b); } \
	inline EnumType operator&=(EnumType& a, EnumType b) { return a = (a & b); } \
	inline EnumType operator~(EnumType a) { return (EnumType)(~(uint32_t)(a)); }
*/
struct galResourceState
{
	enum Flags
	{
		Undefined = 0,
		VertexBufferBit = GAL_BIT(0),
		IndexBufferBit = GAL_BIT(1),
		RenderTargetBit = GAL_BIT(2),
		PresentBit = GAL_BIT(3),
		ShaderInputBit = GAL_BIT(4),
		CopySourceBit = GAL_BIT(5),
		CopyDestinationBit = GAL_BIT(6),
		DepthWriteBit = GAL_BIT(7),
		UnorderedAccessBit = GAL_BIT(8),
		CommonBit = GAL_BIT(9),
		IndirectCommandBit = GAL_BIT(10),
		UniformBufferBit = GAL_BIT(11),
		StorageBufferBit = GAL_BIT(12),
		DepthReadBit = GAL_BIT(13)
	};
};
//GAL_ENUM_OPERATORS(galResourceState::Flags)

struct galShaderType
{
	enum Id
	{
		Vertex,
		Pixel,
		Compute,
		Count
	};
};

struct galShaderStage
{
	enum Flags
	{
		None,
		VertexBit = GAL_BIT(0),
		PixelBit = GAL_BIT(1),
		ComputeBit = GAL_BIT(2)
	};
};
//GAL_ENUM_OPERATORS(galShaderStage::Flags)

struct galDescriptorType
{
	enum Flags
	{
		Undefined,
		SamplerBit = GAL_BIT(1),
		CombinedImageSamplerBit = GAL_BIT(2),
		SampledImageBit = GAL_BIT(3),
		UniformBufferBit = GAL_BIT(4),
		StorageBufferBit = GAL_BIT(5),
		InputAttachmentBit = GAL_BIT(6),
		ReadWriteImageBit = GAL_BIT(7)
	};
};
//GAL_ENUM_OPERATORS(galDescriptorType::Flags)

struct galMemoryUsage
{
	enum Id
	{
		DeviceLocal,
		Upload,
		Readback,
		Count
	};
};

struct galIndexType
{
	enum Id
	{
		UInt32,
		UInt16
	};
};

struct galDataType
{
	enum Id
	{
		Float32,
		UNorm8,
		UInt32,
		Count
	};
};

struct galTextureFormat
{
	enum Id
	{
		R8G8B8A8_UNorm,
		B8G8R8A8_UNorm,
		B8G8R8A8_sRGB,
		D16_UNorm,
		D32_SFloat,
		//D24_UNorm_S8_UInt, // this is not well supported (:wave: AMD)
		R32_UInt,
		Count
	};
};

typedef struct galLoadAction_s
{
	enum 
	{
		Load, // @TODO: safety as the default to avoid headaches
		DontCare,
		Clear,
		Count
	} Id;
} galLoadAction;

typedef union galClearValue_u
{
	struct galClearColor
	{
		float color[4];
	}
	color;
	struct galClearDepthStencil
	{
		float depth;
		uint32_t stencil;
	}
	depthStencil;
} galClearValue;

struct galLoadActions
{
	galClearValue colorClearValues[GAL_MAX_BOUND_COLOR_TARGETS];
	galClearValue depthStencilClearValue;
	galLoadAction::Id colorLoadActions[GAL_MAX_BOUND_COLOR_TARGETS];
	galLoadAction::Id depthLoadAction;
};

struct galTextureDesc
{
	uint64_t nativeImage;
	const char* name;
	uint32_t width;
	uint32_t height;
	uint32_t mipCount;
	uint32_t sampleCount;
	galResourceState::Flags initialState;
	galDescriptorType::Flags descriptorType;
	galTextureFormat::Id format;
};

struct galTextureUploadDesc
{
	const void* data;
	uint32_t mipLevel;
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

struct galSamplerDesc
{
	const char* name;
	qbool clampToEdge;
	qbool anisotropicFiltering;
	qbool negativeLODBias;
};

struct galBufferDesc
{
	const char* name;
	uint32_t byteCount;
	galResourceState::Flags initialState;
	galMemoryUsage::Id memoryUsage;
};

struct galSubmitGraphicsDesc
{
	galSemaphore* waitSemaphores;
	galSemaphore* signalSemaphores;
	galFence signalFence;
	uint32_t waitSemaphoreCount;
	uint32_t signalSemaphoreCount;
};

struct galSubmitPresentDesc
{
	galSemaphore* waitSemaphores;
	uint32_t waitSemaphoreCount;
	uint32_t swapChainImageIndex; // what you get from GAL_AcquireNextImage
};

struct galSwapChainInfo
{
	const galTexture* renderTargets;
	uint32_t imageCount;
	// @TODO: galTextureFormat::Id format;
};

struct galShaderDesc
{
	const char* name;
	const void* data;
	size_t byteCount;
	galShaderType::Id type;
	// @TODO: specialization constants, entry point, ...
};

struct galDescriptorSetLayoutBinding
{
	uint32_t bindingSlot;
	galShaderStage::Flags stageFlags;
	galDescriptorType::Flags descriptorType;
	uint32_t descriptorCount;
	const galSampler* immutableSamplers;
	qbool canUpdateUnusedWhilePending;
};

struct galDescriptorSetLayoutDesc
{
	const char* name;
	const galDescriptorSetLayoutBinding* bindings;
	uint32_t bindingCount;
};

struct galPipelineLayoutDesc
{
	struct PushConstantsRange
	{
		uint32_t byteOffset;
		uint32_t byteCount;
	}
	pushConstantsPerStage[galShaderType::Count];
	const char* name;
	const galDescriptorSetLayout* descriptorSetLayouts;
	uint32_t descriptorSetLayoutCount;
};

struct galDescriptorSetDesc
{
	const char* name;
	galDescriptorSetLayout descriptorSetLayout;
};

struct galVertexAttribDesc
{
	uint32_t binding;
	uint32_t location;
	galDataType::Id dataType;
	uint32_t vectorLength;
	uint32_t byteOffset;
};

struct galVertexBindingDesc
{
	uint32_t binding;
	uint32_t stride;
};

struct galSpecializationEntry
{
	uint32_t constandId;
	uint32_t byteOffset;
	uint32_t byteCount;
};

struct galSpecialization
{
	const galSpecializationEntry* entries;
	const void* data;
	uint32_t entryCount;
	uint32_t byteCount;
};

struct galGraphicsPipelineDesc
{
	const char* name;
	struct VertexLayout
	{
		const galVertexAttribDesc* attribs;
		const galVertexBindingDesc* bindings;
		uint32_t attribCount;
		uint32_t bindingCount;
	}
	vertexLayout;
	struct RenderTargets
	{
		galTextureFormat::Id colorTargetFormats[GAL_MAX_BOUND_COLOR_TARGETS];
		galTextureFormat::Id depthStencilFormat;
		uint32_t colorTargetCount;
		qbool hasDepthStencil;
	}
	renderTargets;
	galShader vertexShader;
	galShader fragmentShader;
	galSpecialization vertexSpec;
	galSpecialization fragmentSpec;
	galPipelineLayout pipelineLayout;
	uint32_t cullType; // cullType_t
	uint32_t srcBlend; // stateBits & GLS_SRCBLEND_BITS
	uint32_t dstBlend; // stateBits & GLS_DSTBLEND_BITS
	uint32_t depthTest; // depthTest_t
	qbool enableDepthWrite;
	qbool enableDepthTest;
};

struct galComputePipelineDesc
{
	const char* name;
	galShader computeShader;
	galPipelineLayout pipelineLayout;
};

struct galBufferBarrier
{
	galBuffer buffer;
	galResourceState::Flags oldState;
	galResourceState::Flags newState;
};

struct galTextureBarrier
{
	galTexture texture;
	galResourceState::Flags oldLayout;
	galResourceState::Flags newLayout;
};

struct galCommandPoolDesc
{
	qbool presentQueue;
	qbool transient;
};

struct galBufferRegion
{
	uint32_t srcByteOffset;
	uint32_t dstByteOffset;
	uint32_t byteCount;
};

void GAL_Init();
void GAL_Shutdown(qbool fullShutDown);

void GAL_BeginFrame();
void GAL_DrawImGUI();

void GAL_CreateFence(galFence* fence, const char* name);
void GAL_DestroyFence(galFence fence);
void GAL_WaitForAllFences(uint32_t fenceCount, const galFence* fences);

void GAL_CreateSemaphore(galSemaphore* semaphore, const char* name);
void GAL_DestroySemaphore(galSemaphore semaphore);

void GAL_CreateBuffer(galBuffer* buffer, const galBufferDesc* desc);
void GAL_DestroyBuffer(galBuffer buffer);
void GAL_MapBuffer(void** data, galBuffer buffer);
void GAL_UnmapBuffer(galBuffer buffer);

void GAL_CreateTexture(galTexture* texture, const galTextureDesc* desc);
void GAL_UploadTextureData(galTexture texture, const galTextureUploadDesc* desc);
void GAL_DestroyTexture(galTexture texture);

void GAL_CreateSampler(galSampler* sampler, const galSamplerDesc* desc);
void GAL_DestroySampler(galSampler sampler);

void GAL_CreateShader(galShader* shader, const galShaderDesc* desc);
void GAL_DestroyShader(galShader shader);

void GAL_CreateDescriptorSetLayout(galDescriptorSetLayout* layout, const galDescriptorSetLayoutDesc* desc);
void GAL_DestroyDescriptorSetLayout(galDescriptorSetLayout layout);

void GAL_CreatePipelineLayout(galPipelineLayout* layout, const galPipelineLayoutDesc* desc);
void GAL_DestroyPipelineLayout(galPipelineLayout layout);

// technically, galDescriptorType::Flags is not needed but it's useful to check that you haven't messed up
void GAL_CreateDescriptorSet(galDescriptorSet* set, const galDescriptorSetDesc* desc);
void GAL_UpdateDescriptorSet(galDescriptorSet set, uint32_t binding, galDescriptorType::Flags type,
							 uint32_t handleCount, const void* handleArray);
void GAL_DestroyDescriptorSet(galDescriptorSet set);

void GAL_CreateGraphicsPipeline(galPipeline* pipeline, const galGraphicsPipelineDesc* desc);
void GAL_CreateComputePipeline(galPipeline* pipeline, const galComputePipelineDesc* desc);
void GAL_DestroyPipeline(galPipeline pipeline);

void GAL_GetSwapChainInfo(galSwapChainInfo* desc);
void GAL_AcquireNextImage(uint32_t* outImageIndex, galSemaphore signalSemaphore);
void GAL_WaitUntilDeviceIdle();

void GAL_SubmitGraphics(const galSubmitGraphicsDesc* desc);
void GAL_SubmitPresent(const galSubmitPresentDesc* desc);

// think of the command pool as a linear allocator and reset as a clear operation
// make sure to call reset to prevent the pool from growing too much
void GAL_CreateCommandPool(galCommandPool* pool, const galCommandPoolDesc* desc);
void GAL_ResetCommandPool(galCommandPool pool);
void GAL_DestroyCommandPool(galCommandPool pool);

void GAL_CreateCommandBuffer(galCommandBuffer* cmdBuf, galCommandPool pool);
void GAL_BindCommandBuffer(galCommandBuffer cmdBuf);
void GAL_BeginCommandBuffer();
void GAL_EndCommandBuffer();
void GAL_DestroyCommandBuffer(galCommandBuffer cmdBuf);

void GAL_CmdBindRenderTargets(uint32_t colorCount, const galTexture* colors, galTexture depthStencil = { 0 }, const galLoadActions* loadActions = NULL);
void GAL_CmdBindPipeline(galPipeline pipeline);
void GAL_CmdBindDescriptorSet(uint32_t index, galDescriptorSet descriptorSet, galPipelineLayout pipelineLayout);
void GAL_CmdBindVertexBuffers(uint32_t count, const galBuffer* vertexBuffers);
void GAL_CmdBindIndexBuffer(galBuffer indexBuffer, galIndexType::Id type);
void GAL_CmdSetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h, float minDepth = 0.0f, float maxDepth = 1.0f);
void GAL_CmdSetScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void GAL_CmdPushConstants(galPipelineLayout layoutHandle, galShaderType::Id shaderStage, const void* constants);
void GAL_CmdDraw(uint32_t vertexCount, uint32_t firstVertex);
void GAL_CmdDrawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex);
void GAL_CmdBarriers(uint32_t buffCount, const galBufferBarrier* buffDescs,
					 uint32_t texCount, const galTextureBarrier* texDescs);

void GAL_CmdCopyBuffer(galBuffer dst, uint32_t dstOffset, galBuffer src, uint32_t srcOffset, uint32_t byteCount);
void GAL_CmdCopyBufferRegions(galBuffer dst, galBuffer src, uint32_t count, const galBufferRegion* regions);

// @TODO: specify mip level, type union for clear color, ...
void GAL_CmdClearColorTexture(galTexture texture, const uint32_t* clearColor);

void GAL_CmdInsertDebugLabel(const char* name, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
void GAL_CmdBeginDebugLabel(const char* name, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
void GAL_CmdEndDebugLabel();

// when resolving a query, said query must be from the oldest frame in flight
// if the query to resolve is a null handle, it will succeed with a 0 return value
void GAL_CmdBeginDurationQuery(galDurationQuery* query);
void GAL_CmdEndDurationQuery(galDurationQuery query);
void GAL_CmdResetDurationQueries();
void GAL_ResolveDurationQuery(uint32_t* microSeconds, galDurationQuery query);


#define	MAX_DYNAMIC_VERTEXES (1 << 20)
#define	MAX_DYNAMIC_INDEXES	 (6 * MAX_DYNAMIC_VERTEXES)

#endif
#endif