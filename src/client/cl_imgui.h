#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#ifndef __CL_IMGUI__
#include "../cimgui/cimgui.h"
#include "../game/q_shared.h"





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

#define M(Enum, Desc) Desc,
static const char* mainMenuNames[ImGUI_MainMenu_Count + 1] =
{
	MAIN_MENU_LIST(M)
	""
};
#undef M

typedef struct MainMenuItem
{
	ImGUI_MainMenu_Id menu;
	const char* name;
	const char* shortcut;
	bool* selected;
	bool enabled;
} MainMenuItem;

typedef struct MainMenu
{
	MainMenuItem items[64];
	int itemCount;
	int itemCountPerMenu[ImGUI_MainMenu_Count]; // effectively a histogram
} MainMenu;


typedef enum ImGUI_ShortcutOptions
{
	ImGUI_ShortcutOptions_None = 0,
	ImGUI_ShortcutOptions_Local = 0,
	ImGUI_ShortcutOptions_Global = 1 << 0
} ImGUI_ShortcutOptions;

void GUI_AddMainMenuItem(ImGUI_MainMenu_Id menu, const char* item, const char* shortcut, qbool* selected, qbool enabled);
void GUI_DrawMainMenu();
void ToggleBooleanWithShortcut(qbool *value, ImGuiKey key, ImGUI_ShortcutOptions flags);
qbool IsShortcutPressed(ImGuiKey key, ImGUI_ShortcutOptions flags);

void TableHeader(int count, ...);
void TableRow(int count, ...);
void TableRowBool(const char* item0, qbool item1);
void TableRowInt(const char* item0, int item1);
void TableRowStr(const char* item0, float item1, const char* format);


#endif