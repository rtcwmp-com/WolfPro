#include "tr_local.h"
#include "rhi.h"
#include "shaders/msaa_cs.h"


rhiDescriptorSetLayout msaaDescSetLayout;
rhiPipeline msaaPipeline;
rhiDescriptorSet msaaDescSet;
rhiTexture msaaTexture;
rhiTexture resolvedTexture;

void RB_MSAA_Init(rhiTexture MSTexture, rhiTexture resTexture){
    msaaTexture = MSTexture;
    resolvedTexture = resTexture;
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

    desc.shader.data = msaa_cs;
    desc.shader.byteCount = sizeof(msaa_cs);

    msaaPipeline = RHI_CreateComputePipeline(&desc);
    msaaDescSet = RHI_CreateDescriptorSet("MSAA", msaaDescSetLayout, qfalse);

    RHI_UpdateDescriptorSet(msaaDescSet, 0, RHI_DescriptorType_ReadOnlyTexture, 0, 1, &msaaTexture, 0);
    RHI_UpdateDescriptorSet(msaaDescSet, 1, RHI_DescriptorType_ReadWriteTexture, 0, 1, &resolvedTexture, 0);
}

void RB_MSAA_Resolve(void){
    RB_EndRenderPass();

    RHI_CmdBeginBarrier();
    RHI_CmdTextureBarrier(resolvedTexture, RHI_ResourceState_ShaderReadWriteBit);
    RHI_CmdTextureBarrier(msaaTexture, RHI_ResourceState_ShaderInputBit);
    RHI_CmdEndBarrier();


    RHI_CmdBindPipeline(msaaPipeline);
    RHI_CmdBindDescriptorSet(msaaPipeline, msaaDescSet);

    RHI_CmdDispatch((glConfig.vidWidth + 7)/8, (glConfig.vidHeight + 7)/8, 1);

}