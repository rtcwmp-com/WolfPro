#include "cg_local.h"
#include "../client/cl_imgui.h"

void CG_ImGUI_Share(void *ctx, void *alloc, void *free, void **user){
	cgs.igContext = ctx;
	cgs.igAlloc = alloc;
	cgs.igFree = free;
	cgs.igUserData = user;
}

void CG_ImGUI_Update(void) {
	igSetCurrentContext((ImGuiContext*)cgs.igContext);
	igSetAllocatorFunctions((ImGuiMemAllocFunc)cgs.igAlloc, (ImGuiMemFreeFunc)cgs.igFree, cgs.igUserData);

    static qbool windowActive = qfalse;
	ToggleBooleanWithShortcut(&windowActive, ImGuiKey_C, ImGUI_ShortcutOptions_Global);
	trap_CL_AddGuiMenu(ImGUI_MainMenu_Info, "CGame info", "", &windowActive, qtrue);
   
	if(windowActive){
		
		if(igBegin("CGame info", &windowActive, 0)){
			igText("hello from CGame");
			igShowDemoWindow(&windowActive);
        }
        igEnd();
    }

	static qbool demoTimelineActive = qfalse;
	ToggleBooleanWithShortcut(&demoTimelineActive, ImGuiKey_D, ImGUI_ShortcutOptions_Global);
	trap_CL_AddGuiMenu(ImGUI_MainMenu_Info, "Demo Tools", "", &demoTimelineActive, cg.demoPlayback);
   
	if(demoTimelineActive){
		int percent;
		if(igBegin("Timeline", &demoTimelineActive, 0)){
			igSliderInt("playback slider", &percent, 0, 100, "", 0);

			
        }
        igEnd();
    }
}