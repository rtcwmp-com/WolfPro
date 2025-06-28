#include "cl_imgui.h"



// global requires shift key down, local requires shift up


qbool IsShortcutPressed(ImGuiKey key, ImGUI_ShortcutOptions flags)
{
	const bool globalShortcut = (flags & ImGUI_ShortcutOptions_Global) != 0;
	const bool shiftStateOK = globalShortcut == igIsKeyDown_Nil(ImGuiMod_Shift);
	
	return igIsKeyDown_Nil(ImGuiMod_Ctrl) && shiftStateOK && igIsKeyPressed_Bool(key, false);
}

void ToggleBooleanWithShortcut(qbool *value, ImGuiKey key, ImGUI_ShortcutOptions flags)
{
	if (IsShortcutPressed(key, flags))
	{
		*value = !*value;
	}
}


void TableHeader(int count, ...)
{
	va_list args;
	va_start(args, count);
	for(int i = 0; i < count; ++i)
	{
		const char* header = va_arg(args, const char*);
		igTableSetupColumn(header, 0, 0, 0);
	}
	va_end(args);

	igTableHeadersRow();
}

void TableRow(int count, ...)
{
	igTableNextRow(0, 0.0f);

	va_list args;
	va_start(args, count);
	for(int i = 0; i < count; ++i)
	{
		const char* item = va_arg(args, const char*);
		igTableSetColumnIndex(i);
		igText(item);
	}
	va_end(args);
}

void TableRowBool(const char* item0, qbool item1)
{
	TableRow(2, item0, item1 ? "YES" : "NO");
}

void TableRowInt(const char* item0, int item1)
{
	TableRow(2, item0, va("%d", item1));
}

void TableRowStr(const char* item0, float item1, const char* format)
{
	TableRow(2, item0, va(format, item1));
}