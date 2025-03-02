#include "cl_imgui.h"
#include "font_bahnschrift.h"
#include "client.h" 
#include <float.h>

static const ImWchar codepointRanges[] =
{
    32, 126,
    0
};


static void AddFont()
{
    ImGuiIO* io = igGetIO();
    int height = 13;
    ImFontConfig config = {};
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = 0;
    config.OversampleV = 0;
    config.PixelSnapH = true;
    config.SizePixels = height;

    config.GlyphMaxAdvanceX = FLT_MAX;
    config.RasterizerDensity = 1.0f;
    config.RasterizerMultiply = 1.0f;


    Com_sprintf(config.Name, sizeof(config.Name), "%s (%dpx)", "Banschrift", height);
    ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, bahnschrift_ttf, sizeof(bahnschrift_ttf), config.SizePixels, &config, codepointRanges);
    //ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, bahnschrift_ttf, sizeof(bahnschrift_ttf), height, NULL, codepointRanges);
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