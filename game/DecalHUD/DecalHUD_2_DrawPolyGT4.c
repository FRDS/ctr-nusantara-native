#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80023054-0x80023190.
void DecalHUD_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale)
{
#if BUILD > SepReview
	if (!icon)
		return;
#endif

	// setInt32RGB4 needs to go before addPolyGT4
	// for more information check "include/gpu.h"
	POLY_GT4 *p = (POLY_GT4 *)primMem->curr;
	setInt32RGB4(p, color0, color1, color2, color3);
	addPolyGT4(ot, p);

	u32 width = icon->texLayout.u1 - icon->texLayout.u0;
	u32 height = icon->texLayout.v2 - icon->texLayout.v0;
	u32 bottomY = posY + FP_Mult(height, scale);
	u32 rightX = (u16)posX + FP_Mult(width, scale);
	setXY4CompilerHack(p, (u16)posX, posY, rightX, posY, (u16)posX, bottomY, rightX, bottomY);
	setIconUV(p, icon);

	if (transparency)
	{
		setTransparency(p, transparency);
	}

	primMem->curr = p + 1;
}
