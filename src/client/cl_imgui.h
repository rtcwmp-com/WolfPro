#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#ifndef __CL_IMGUI__
#include "../cimgui/cimgui.h"



#define MAIN_MENU_LIST(M) \
	M(ImGUI_MainMenu_Tools, "Tools") \
	M(ImGUI_MainMenu_Info, "Information") \
	M(ImGUI_MainMenu_Perf, "Performance") \
	M(ImGUI_MainMenu_Im3D, "Im3D")

#define M(Enum, Desc) Enum,

typedef enum ImGUI_MainMenu_Id
{
    MAIN_MENU_LIST(M)
    ImGUI_MainMenu_Count
} ImGUI_MainMenu_Id;

#undef M

typedef enum ImGUI_ShortcutOptions
{
	ImGUI_ShortcutOptions_None = 0,
	ImGUI_ShortcutOptions_Local = 0,
	ImGUI_ShortcutOptions_Global = 1 << 0
} ImGUI_ShortcutOptions;

void GUI_AddMainMenuItem(ImGUI_MainMenu_Id menu, const char* item, const char* shortcut, bool* selected, bool enabled);
void GUI_DrawMainMenu();
void ToggleBooleanWithShortcut(bool *value, ImGuiKey key, ImGUI_ShortcutOptions flags);

#endif