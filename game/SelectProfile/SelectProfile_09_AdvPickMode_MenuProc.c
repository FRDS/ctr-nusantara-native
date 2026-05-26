#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048960-0x80048a30.
void SelectProfile_AdvPickMode_MenuProc(struct RectMenu *menu)
{
	if (menu->unk1e != 0)
	{
		SelectProfile_Init(menu->drawStyle);
		SelectProfile_DrawAdvProfile(&sdata->advProgress, 0x92, 0x32, 0, 0, 0x10);
		return;
	}

	s16 row = menu->rowSelected;

	if ((row >= 0) && (row < 3))
	{
		SelectProfile_ToggleMode((u16)menu->rowSelected | 0x20);
		sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;
		return;
	}

	if ((row == -1) || (row == 3))
	{
		RECTMENU_Hide(menu);
		SelectProfile_Destroy();
	}
}
