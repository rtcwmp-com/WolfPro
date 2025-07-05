#include "cg_local.h"
#include "../client/cl_imgui.h"

void CG_ImGUI_Share(void *ctx, void *alloc, void *free, void **user){
	cgs.igContext = ctx;
	cgs.igAlloc = alloc;
	cgs.igFree = free;
	cgs.igUserData = user;
}

char* GetStringTimestamp(int seconds){
	int hour = seconds / 3600;
	int minute = (seconds / 60) % 60;
	int second = seconds % 60;
	if(hour > 0){
		return va("%02d:%02d:%02d", hour, minute, second);
	}
	return va("%02d:%02d", minute, second);
}

void CG_ImGUI_Update(void) {
	if (cgs.igContext == NULL) {
		return;
	}
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
	if (cg.demoPlayback) {
		demoTimelineActive = qtrue;
	}

	ToggleBooleanWithShortcut(&demoTimelineActive, ImGuiKey_D, ImGUI_ShortcutOptions_Global);
	trap_CL_AddGuiMenu(ImGUI_MainMenu_Info, "Demo Tools", "", &demoTimelineActive, cg.demoPlayback);
   
	if(demoTimelineActive){
		
		if(igBegin("Timeline", &demoTimelineActive, 0)){
			
			int demoDuration = (m_lastServerTime - m_firstServerTime);
			int currentOffset = (m_currServerTime - m_firstServerTime);
			float percent = ((float)currentOffset / (float)demoDuration) * 100.0f;
			float origPercent = percent;
			igPushItemWidth(igGetWindowWidth() - 16.0f);
			igSliderFloat("##timeline", &percent, 0.0f, 100.0f, "", ImGuiSliderFlags_None);
			igPopItemWidth();
			
			igText("%s / %s", GetStringTimestamp(currentOffset / 1000), GetStringTimestamp(demoDuration / 1000));

			if(percent != origPercent){
				int serverTimeAtPercent = (int)((float)((m_lastServerTime - m_firstServerTime) * percent / 100.0f)) + m_firstServerTime;
				CG_NDP_SeekAbsolute(serverTimeAtPercent);
			}
			
			
			igSameLine(0.0f, 16.0f);
			if(igButton("Next Frag", (ImVec2){0.0f, 0.0f})){
				CG_NDP_GoToNextFrag(qtrue);
			}
			igSameLine(0.0f, 16.0f);

			if(igButton("Prev Frag", (ImVec2){0.0f, 0.0f})){
				CG_NDP_GoToNextFrag(qfalse);
			}
			igSameLine(0.0f, 16.0f);
			static qbool paused = qfalse;
			if(!paused){
				if(igButton("Pause", (ImVec2){0.0f, 0.0f})){
					trap_Cvar_Set("timescale", "0");
					paused = qtrue;
				}
			}else{
				if(igButton("Play", (ImVec2){0.0f, 0.0f})){
					trap_Cvar_Set("timescale", "1");
					paused = qfalse;
				}
			}
        }
        igEnd();
    }
}