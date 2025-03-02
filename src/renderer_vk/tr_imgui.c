#include "../client/cl_imgui.h"
#include "rhi.h"
#include "tr_local.h"


void RB_ImGUI_Init(void){
    ImGuiIO* io = igGetIO();
    io->DisplaySize.x = glConfig.vidWidth;
    io->DisplaySize.y = glConfig.vidHeight;
    io->BackendRendererName = "Wolfenstein";
    io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    
    //RHI_CreateGraphicsPipeline(desc);
}

void RB_ImGUI_BeginFrame(void){
    igNewFrame();
}

void RB_ImGUI_EndFrame(void){
    igEndFrame();
}

void RB_ImGUI_Draw(void){
    igShowDemoWindow(NULL);
    igRender();
}


