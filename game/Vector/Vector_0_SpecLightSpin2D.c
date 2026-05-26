#include <common.h>

static void Vector_SpecLightSpin2D_RotMatrixMul(MATRIX *matrix, const SVec3 *input, VECTOR *mac, SVec3 *output)
{
	gte_SetRotMatrix(matrix);
	gte_ldv0((SVECTOR *)input);
	gte_rtv0();
	gte_stlvnl(mac);

	output->x = (s16)mac->vx;
	output->y = (s16)mac->vy;
	output->z = (s16)mac->vz;
}

void Vector_SpecLightSpin2D(struct Instance *inst, s16 *rot, s16 *lightDir)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800572d0-0x8005741c.
	MATRIX rotMatrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 light = {.x = lightDir[0], .y = lightDir[1], .z = lightDir[2]};
	SVec3 lightLocal;
	SVec3 view = {.x = 0, .y = 0, .z = 0x1000};
	SVec3 viewLocal;
	SVec3 halfVector;
	struct GameTracker *gGT = sdata->gGT;
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

	ConvertRotToMatrix_Transpose(&rotMatrix, rot);
	Vector_SpecLightSpin2D_RotMatrixMul(&rotMatrix, &light, &lightMac, &lightLocal);

	inst->unk53 = (char)lightMac.vx;
	inst->reflectionRGBA = (u32)lightMac.vz;

	Vector_SpecLightSpin2D_RotMatrixMul(&rotMatrix, &view, &viewMac, &viewLocal);

	halfVector.x = lightLocal.x + viewLocal.x;
	halfVector.y = lightLocal.y + viewLocal.y;
	halfVector.z = lightLocal.z + viewLocal.z;
	MATH_VectorNormalize(&halfVector);

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		idpp[i].specLight[0] = halfVector.x;
		idpp[i].specLight[1] = halfVector.y;
		idpp[i].specLight[2] = halfVector.z;
	}
}
