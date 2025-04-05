#include "cl_imgui.h"
#include "font_bahnschrift.h"
#include "client.h" 
#include <float.h>

static int keyMap[256];
void CL_ImGUI_ButtonMapping();

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

static void ToggleGui_f()
{
	const bool guiActive = Cvar_VariableIntegerValue("r_debugUI") != 0;
	const char* const newValue = guiActive ? "0" : "1";
	Cvar_Set("r_debugUI", newValue);
	Cvar_Set("r_debugInput", newValue);
}

static void ToggleGuiInput_f()
{
	Cvar_Set("r_debugInput", Cvar_VariableIntegerValue("r_debugInput") ? "0" : "1");
}



void CL_ImGUI_Init(void){
    igCreateContext(NULL);
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
    ioptr->MouseDrawCursor = false;
    igStyleColorsDark(NULL);
    AddFont();
    CL_ImGUI_ButtonMapping();

	Cmd_AddCommand("togglegui", ToggleGui_f);
	Cmd_AddCommand("toggleguiinput", ToggleGuiInput_f);
}

void CL_ImGUI_Shutdown(void){
    igDestroyContext(NULL);
}

void CL_ImGUI_Frame()
{
	if(Cvar_VariableIntegerValue("r_debugInput"))
	{
		cls.keyCatchers |= KEYCATCH_IMGUI;
	}
	else
	{
		cls.keyCatchers &= ~KEYCATCH_IMGUI;
	}

	static int64_t prevMS = 0;
	const int64_t currMS = Sys_Milliseconds();
	const int64_t elapsedMS = currMS - prevMS;
	prevMS = currMS;

	int x, y;
	Sys_GetCursorPosition(&x, &y);
	re.ComputeCursorPosition(&x, &y);

	ImGuiIO *ioptr = igGetIO();
	ioptr->DeltaTime = (float)((double)elapsedMS / 1000.0);
	ioptr->MousePos.x = x;
	ioptr->MousePos.y = y;
}

void CL_ImGUI_MouseEvent(int dx, int dy, int time){
    ImGuiIO *ioptr = igGetIO();
    ImGuiIO_AddMousePosEvent(ioptr,(float) dx,(float) dy);
}

void CL_ImGUI_CharEvent(int key){
	ImGuiIO *ioptr = igGetIO();
	ImGuiIO_AddInputCharacter(ioptr, key);
}

qboolean CL_ImGUI_KeyEvent(int key, qboolean down, const char* cmd) {

	if(down)
	{
		if(cmd != NULL)
		{
			const char* const prefix = "keycatchgui";
			if(Q_stristr(cmd, prefix) == cmd)
			{
				Cbuf_AddText(cmd + strlen(prefix));
				Cbuf_AddText("\n");
				return qtrue;
			}
		}
	}
	
	if(down && (key == '`' || key == '~'))
	{
		// continue displaying the GUI but route input to the console
		Cvar_Set("r_debugInput", "0");
		return qfalse;
	}

	if(cls.keyCatchers & KEYCATCH_IMGUI){
		ImGuiIO *ioptr = igGetIO();
		unsigned int imguiKey;
		switch(key)
			{
				case K_MOUSE1:
				case K_MOUSE2:
				case K_MOUSE3:
				case K_MOUSE4:
				case K_MOUSE5:
					ImGuiIO_AddMouseButtonEvent(ioptr, key - K_MOUSE1, !!down);
					break;
				case K_MWHEELDOWN:
				case K_MWHEELUP:
					ImGuiIO_AddMouseWheelEvent(ioptr, 0.0f, key == K_MWHEELDOWN ? -1.0f : 1.0f);
					break;
				default:
					imguiKey = (unsigned int)keyMap[key];
					if(imguiKey != 0xFFFFFFFF)
					{
						ImGuiIO_AddKeyEvent(ioptr, (ImGuiKey)imguiKey, !!down);
					}
					break;
			}
		return qtrue;
	}
	return qfalse;
    
}

void CL_ImGUI_ButtonMapping(){
    memset(keyMap, 0xFF, sizeof(keyMap));
	keyMap[K_CTRL] = ImGuiMod_Ctrl;
	keyMap[K_ALT] = ImGuiMod_Alt;
	keyMap[K_SHIFT] = ImGuiMod_Shift;
	keyMap[K_TAB] = ImGuiKey_Tab;
	keyMap[K_LEFTARROW] = ImGuiKey_LeftArrow;
	keyMap[K_RIGHTARROW] = ImGuiKey_RightArrow;
	keyMap[K_UPARROW] = ImGuiKey_UpArrow;
	keyMap[K_DOWNARROW] = ImGuiKey_DownArrow;
	keyMap[K_PGUP] = ImGuiKey_PageUp;
	keyMap[K_PGDN] = ImGuiKey_PageDown;
	keyMap[K_HOME] = ImGuiKey_Home;
	keyMap[K_END] = ImGuiKey_End;
	keyMap[K_INS] = ImGuiKey_Insert;
	keyMap[K_DEL] = ImGuiKey_Delete;
	keyMap[K_BACKSPACE] = ImGuiKey_Backspace;
	keyMap[K_SPACE] = ImGuiKey_Space;
	keyMap[K_ENTER] = ImGuiKey_Enter;
	keyMap[K_ESCAPE] = ImGuiKey_Escape;
	keyMap[K_CAPSLOCK] = ImGuiKey_CapsLock;
	keyMap[K_PAUSE] = ImGuiKey_Pause;
	//keyMap[K_BACKSLASH] = ImGuiKey_Backslash;
	keyMap[K_KP_INS] = ImGuiKey_Keypad0;
	keyMap[K_KP_END] = ImGuiKey_Keypad1;
	keyMap[K_KP_DOWNARROW] = ImGuiKey_Keypad2;
	keyMap[K_KP_PGDN] = ImGuiKey_Keypad3;
	keyMap[K_KP_LEFTARROW] = ImGuiKey_Keypad4;
	keyMap[K_KP_5] = ImGuiKey_Keypad5;
	keyMap[K_KP_RIGHTARROW] = ImGuiKey_Keypad6;
	keyMap[K_KP_HOME] = ImGuiKey_Keypad7;
	keyMap[K_KP_UPARROW] = ImGuiKey_Keypad8;
	keyMap[K_KP_PGUP] = ImGuiKey_Keypad9;
	//keyMap[K_KP_ENTER] = ImGuiKey_KeyPadEnter;
	keyMap[K_KP_SLASH] = ImGuiKey_KeypadDivide;
	keyMap[K_KP_MINUS] = ImGuiKey_KeypadSubtract;
	keyMap[K_KP_PLUS] = ImGuiKey_KeypadAdd;
	keyMap[K_KP_STAR] = ImGuiKey_KeypadMultiply;
	keyMap[K_KP_EQUALS] = ImGuiKey_KeypadEqual;
	for(int i = 0; i < 26; ++i)
	{
		keyMap['a' + i] = ImGuiKey_A + i;
	}
	for(int i = 0; i < 10; ++i)
	{
		keyMap['0' + i] = ImGuiKey_0 + i;
	}
	for(int i = 0; i < 12; ++i)
	{
		keyMap[K_F1 + i] = ImGuiKey_F1 + i;
	}
}

