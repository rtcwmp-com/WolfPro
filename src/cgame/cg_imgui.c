#include "cg_local.h"
#include "../client/cl_imgui.h"
#include <float.h>

qbool BeginTimeline(const char* str_id);
qbool TimelineEvent(const char* str_id, float* values, int numVals);
void EndTimeline(void);

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
			const char *event_types[] = {"Frags", "Obj Taken", "Obj Dropped"};
			static qbool selected[] = { qtrue, qtrue, qfalse };
			if(igTreeNode_Str("Events")){
				igBeginChild_Str("##EventsAvailable", (ImVec2){0.0f, 0.0f,}, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, 0);
				for(int i = 0; i < ARRAY_LEN(event_types); i++){
					igCheckbox(event_types[i], &selected[i]);
				}
				
				igEndChild();
				igTreePop();
			}
			
			
			int demoDuration = (m_lastServerTime - m_firstServerTime);
			int currentOffset = (m_currServerTime - m_firstServerTime);
			float percent = ((float)currentOffset / (float)demoDuration);
			float origPercent = percent;
			igPushItemWidth(igGetWindowWidth() - 16.0f);
			
			//igSliderFloat("##timeline", &percent, 0.0f, 100.0f, "", ImGuiSliderFlags_None);
			if (BeginTimeline("Timeline3")) {
				float killAtPercent[1024];
				float docsPickupAtPercent[1024];
				float docsDropAtPercent[1024];
				
				for(int k = 0; k < ndp_docPickupSize; k++){
					docsPickupAtPercent[k] =  (float)(ndp_docPickupTime[k] - m_firstServerTime) / demoDuration;
				}
				for(int k = 0; k < ndp_docDropSize; k++){
					docsDropAtPercent[k] =  (float)(ndp_docDropTime[k] - m_firstServerTime) / demoDuration;
				}
		
				for(int i = 0; i < ARRAY_LEN(selected); i++){

					if(selected[i]){
						switch (i) {
						case 0:
						{
							for(int k = 0; k < ndp_myKillsSize; k++){
								killAtPercent[k] =  (float)(ndp_myKills[k] - m_firstServerTime) / demoDuration;
							}
							TimelineEvent(event_types[i], killAtPercent, ndp_myKillsSize);
							break;
						}
						case 1:
							TimelineEvent(event_types[i], docsPickupAtPercent, ndp_docPickupSize);
							break;
						}
						
					}
				}
				EndTimeline();
			}
			
			
			igPopItemWidth();
			
			igText("%s / %s", GetStringTimestamp(currentOffset / 1000), GetStringTimestamp(demoDuration / 1000));

			
			
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

static ImVec2 minBgRect, maxBgRect;
static ImVec2 minRect, maxRect;

qbool BeginTimeline(const char* str_id)
{
	ImGuiWindow* win = igGetCurrentWindow();
	minBgRect.y = FLT_MAX;
	minBgRect.x = FLT_MAX;
	maxBgRect.y = -FLT_MAX;
	maxBgRect.x = -FLT_MAX;
	minRect.y = FLT_MAX;
	minRect.x = FLT_MAX;
	maxRect.y = -FLT_MAX;
	maxRect.x = -FLT_MAX;
	return (qbool)igBeginChild_Str(str_id, (ImVec2){0.0f, 0.0f}, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, 0);
	
}


static const float TIMELINE_RADIUS = 20.0f;
static const float delta = 5.0f;

qbool TimelineEvent(const char* str_id, float* values, int numVals)
{
	ImGuiWindow* win = igGetCurrentWindow();
	ImGuiContext *ctx = igGetCurrentContext();
	
	const ImU32 inactive_color = igColorConvertFloat4ToU32(ctx->Style.Colors[ImGuiCol_Button]);
	const ImU32 active_color = igColorConvertFloat4ToU32(ctx->Style.Colors[ImGuiCol_ButtonHovered]);
	const ImU32 line_color = igColorConvertFloat4ToU32(ctx->Style.Colors[ImGuiCol_ButtonActive]);
	const ImU32 background = igColorConvertFloat4ToU32(ctx->Style.Colors[ImGuiCol_FrameBgActive]);
	qbool changed = qfalse;
	ImVec2 localMin = win->DC.CursorPos;
	
	ImVec2 freeSpace;
	igGetContentRegionAvail(&freeSpace);
	//ImVec2 localMax = (ImVec2){ win->DC.CursorPos.x + freeSpace.x - 2 * TIMELINE_RADIUS - (2*delta),  win->DC.CursorPos.y + (2 * TIMELINE_RADIUS) + delta };
	ImVec2 localMax = (ImVec2){ win->DC.CursorPos.x + freeSpace.x,  win->DC.CursorPos.y + (2 * TIMELINE_RADIUS) + 2 * delta };
	ImDrawList_AddRectFilled(igGetWindowDrawList(), localMin, localMax, background, 0.0f, 0);

	for (int i = 0; i < numVals; i++)
	{
		ImVec2 pos = localMin;
		pos.x += TIMELINE_RADIUS + delta + (freeSpace.x - 2 * TIMELINE_RADIUS - (2*delta)) * values[i];
		pos.y += TIMELINE_RADIUS + delta;

		igSetCursorScreenPos((ImVec2){pos.x - TIMELINE_RADIUS, pos.y - TIMELINE_RADIUS - delta});
		igPushID_Int(i); 
		if (igInvisibleButton(str_id, (ImVec2) { 2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS + 2 * delta }, 0)) {
			CG_NDP_SeekAbsolute(max(values[i] * (m_lastServerTime - m_firstServerTime) + m_firstServerTime - 2000, m_firstServerTime));
		}
		if (igIsItemActive() || igIsItemHovered(0))
		{
			int currentOffset = values[i] * (m_lastServerTime - m_firstServerTime);
			igSetTooltip("%s", GetStringTimestamp(currentOffset / 1000)); 
		}
		
		igPopID();
		ImDrawList_AddCircleFilled(igGetWindowDrawList(), pos, TIMELINE_RADIUS, igIsItemActive() || igIsItemHovered(0) ? active_color : inactive_color, 0);
	}

	minBgRect.x = min(minBgRect.x, localMin.x);
	minBgRect.y = min(minBgRect.y, localMin.y);
	maxBgRect.x = max(maxBgRect.x, localMax.x);
	maxBgRect.y = max(maxBgRect.y, localMax.y);


	localMin.x += TIMELINE_RADIUS + delta;
	localMin.y += delta;
	localMax.x -= TIMELINE_RADIUS + delta;
	localMax.y -= delta;

	minRect.x = min(minRect.x, localMin.x);
	minRect.y = min(minRect.y, localMin.y);
	maxRect.x = max(maxRect.x, localMax.x);
	maxRect.y = max(maxRect.y, localMax.y);

	return changed;
}


void EndTimeline(void)
{
	ImVec2 start, end;
	int demoDuration = (m_lastServerTime - m_firstServerTime);
	int currentOffset = (m_currServerTime - m_firstServerTime);
	float pc = (float)currentOffset / (float)demoDuration;
	ImGuiContext *ctx = igGetCurrentContext();
	const ImU32 color = igColorConvertFloat4ToU32(ctx->Style.Colors[ImGuiCol_PlotLines]);
	float barWidth = 2.0f;
	ImVec2 localMin, localMax;
	localMin.x = minRect.x + pc * (maxRect.x - minRect.x) - 0.5f * barWidth;
	localMin.y = minBgRect.y;
	localMax.x = localMin.x + 0.5f * barWidth;
	localMax.y = maxBgRect.y;
	
	ImDrawList_AddRectFilled(igGetWindowDrawList(),localMin, localMax, color, 0.0f, 0);
	igEndChild();
}