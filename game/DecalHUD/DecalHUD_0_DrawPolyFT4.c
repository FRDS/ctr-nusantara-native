#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022db0-0x80022ec4.
void DecalHUD_DrawPolyFT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale)
{
	if (!icon)
		return;

	POLY_FT4 *p = (POLY_FT4 *)primMem->curr;
	addPolyFT4(ot, p);

	u32 width = icon->texLayout.u1 - icon->texLayout.u0;
	u32 height = icon->texLayout.v2 - icon->texLayout.v0;
	u32 bottomY = posY + FP_Mult(height, scale);
	u32 rightX = posX + FP_Mult(width, scale);

	setXY4CompilerHack(p, posX, posY, rightX, posY, posX, bottomY, rightX, bottomY);
	setIconUV(p, icon);

	// this function doesn't support coloring the primitives
	setShadeTex(p, true);

	if (transparency)
	{
		setTransparency(p, transparency);
	}

	primMem->curr = p + 1;
}
