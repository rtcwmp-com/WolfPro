#include "cl_imgui.h"

void CL_ImGUI_Init(void){
    igCreateContext(NULL);
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);
}

void CL_ImGUI_Shutdown(void){
    igDestroyContext(NULL);
}