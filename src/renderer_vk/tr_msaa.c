#include "tr_local.h"
#include "rhi.h"
#include "shaders/msaa_2_cs.h"
#include "shaders/msaa_4_cs.h"
#include "shaders/msaa_8_cs.h"


#pragma pack(push,1)

typedef struct MSAAPC {
    float invGamma;
    float brightness;
} MSAAPC;

#pragma pack(pop)

rhiDescriptorSetLayout msaaDescSetLayout;
rhiPipeline msaaPipeline;
rhiDescriptorSet msaaDescSet[2];

void RB_MSAA_Init(void){
    if(!RB_IsMSAARequested()){
        return;
    }

    rhiDescriptorSetLayoutDesc descriptorSetLayoutDesc = {};
    descriptorSetLayoutDesc.name = "MSAA Desc Set Layout";
    descriptorSetLayoutDesc.bindingCount = 2;
    descriptorSetLayoutDesc.bindings[0].descriptorCount = 1;
    descriptorSetLayoutDesc.bindings[0].descriptorType = RHI_DescriptorType_ReadOnlyTexture;
    descriptorSetLayoutDesc.bindings[0].stageFlags = RHI_PipelineStage_ComputeBit;

    descriptorSetLayoutDesc.bindings[1].descriptorCount = 1;
    descriptorSetLayoutDesc.bindings[1].descriptorType = RHI_DescriptorType_ReadWriteTexture;
    descriptorSetLayoutDesc.bindings[1].stageFlags = RHI_PipelineStage_ComputeBit;

    msaaDescSetLayout = RHI_CreateDescriptorSetLayout(&descriptorSetLayoutDesc);

    rhiComputePipelineDesc desc = {};
    desc.name = "MSAA";
    desc.descLayout = msaaDescSetLayout;
    desc.pushConstantsBytes = sizeof(MSAAPC);

    switch(r_msaa->integer){
        case 2:
            desc.shader.data = msaa_2_cs;
            desc.shader.byteCount = sizeof(msaa_2_cs);
            break;
        case 4:
            desc.shader.data = msaa_4_cs;
            desc.shader.byteCount = sizeof(msaa_4_cs);
            break;
        case 8:
            desc.shader.data = msaa_8_cs;
            desc.shader.byteCount = sizeof(msaa_8_cs);
            break;
        default:
            assert(!"bad MSAA sample count");
            break;
    }


    msaaPipeline = RHI_CreateComputePipeline(&desc);
    msaaDescSet[0] = RHI_CreateDescriptorSet("MSAA 1", msaaDescSetLayout, qfalse);
    msaaDescSet[1] = RHI_CreateDescriptorSet("MSAA 2", msaaDescSetLayout, qfalse);

    
}

void RB_MSAA_Resolve(rhiTexture MSTexture, rhiTexture resTexture){
    RB_EndRenderPass();
    RB_BeginComputePass("MSAA Resolve");

    RHI_CmdBeginBarrier();
    RHI_CmdTextureBarrier(resTexture, RHI_ResourceState_ShaderReadWriteBit);
    RHI_CmdTextureBarrier(MSTexture, RHI_ResourceState_ShaderInputBit);
    RHI_CmdEndBarrier();

    MSAAPC gammaPC;
    gammaPC.brightness = 1.0f;
    gammaPC.invGamma = 1.0f / r_gamma->value;

    RHI_CmdPushConstants(msaaPipeline, RHI_Shader_Compute, &gammaPC, sizeof(MSAAPC));

    RHI_CmdBindPipeline(msaaPipeline);

    rhiDescriptorSet set = msaaDescSet[backEnd.currentFrameIndex];
    RHI_UpdateDescriptorSet(set, 0, RHI_DescriptorType_ReadOnlyTexture, 0, 1, &MSTexture, 0);
    RHI_UpdateDescriptorSet(set, 1, RHI_DescriptorType_ReadWriteTexture, 0, 1, &resTexture, 0);

    RHI_CmdBindDescriptorSet(msaaPipeline, set);

    RHI_CmdDispatch((glConfig.vidWidth + 7)/8, (glConfig.vidHeight + 7)/8, 1);
    RB_EndComputePass();
}