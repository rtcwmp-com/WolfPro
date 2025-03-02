#include "cl_imgui.h"
#include "font_bahnschrift.h"
#include "client.h"

static const ImWchar codepointRanges[] =
{
    32, 126,
    0
};


static void AddFont()
{
    ImGuiIO* io = igGetIO();
    int height = 13;
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = true;
    config.SizePixels = height;
    Com_sprintf(config.Name, sizeof(config.Name), "%s (%dpx)", "Banschrift", height);
    ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, bahnschrift_ttf, sizeof(bahnschrift_ttf), config.SizePixels, &config, codepointRanges);
}

void CL_ImGUI_Init(void){
    igCreateContext(NULL);
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);
    AddFont();
}

void CL_ImGUI_Shutdown(void){
    igDestroyContext(NULL);
}