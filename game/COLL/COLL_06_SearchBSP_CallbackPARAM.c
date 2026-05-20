#include <common.h>

static int DECOMP_COLL_SearchBSP_CallbackPARAM_Overlaps(struct BSP *node, short minX, short minY, short minZ, short maxX, short maxY, short maxZ)
{
	return ((node->box.min[1] <= maxY) && (node->box.min[0] <= maxX) && (minX <= node->box.max[0]) && (node->box.min[2] <= maxZ) &&
	        (minZ <= node->box.max[2]) && (minY <= node->box.max[1]));
}

static void DECOMP_COLL_SearchBSP_CallbackPARAM_PushChild(struct BSP *root, unsigned short childId, short minX, short minY, short minZ, short maxX, short maxY,
                                                          short maxZ, u_short **stackTop)
{
	struct BSP *child;

	if (childId == 0xffff)
		return;

	child = &root[childId & 0x3fff];
	if (!DECOMP_COLL_SearchBSP_CallbackPARAM_Overlaps(child, minX, minY, minZ, maxX, maxY, maxZ))
		return;

	**stackTop = childId;
	(*stackTop)++;
}

static void DECOMP_COLL_SearchBSP_CallbackPARAM_PushChildren(struct BSP *root, struct BSP *node, short minX, short minY, short minZ, short maxX, short maxY,
                                                             short maxZ, u_short **stackTop)
{
	// Retail pushes child 0 then child 1; the scratchpad stack pops child 1 first.
	DECOMP_COLL_SearchBSP_CallbackPARAM_PushChild(root, (u_short)node->data.branch.childID[0], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
	DECOMP_COLL_SearchBSP_CallbackPARAM_PushChild(root, (u_short)node->data.branch.childID[1], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ebec-0x8001ede4
void DECOMP_COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, void (*callback)(struct BSP *, struct ScratchpadStruct *),
                                         struct ScratchpadStruct *param)
{
	u_short *stackBase;
	u_short *stackTop;

	if (root == NULL)
		return;

	short minX = bbox->min[0];
	short minY = bbox->min[1];
	short minZ = bbox->min[2];
	short maxX = bbox->max[0];
	short maxY = bbox->max[1];
	short maxZ = bbox->max[2];

	// Retail stores pending child IDs at scratchpad 0x1f800070 and pops them
	// LIFO, preserving the original BSP traversal order without host recursion.
	stackBase = CTR_SCRATCHPAD_PTR(u_short, 0x70);
	stackTop = stackBase;

	DECOMP_COLL_SearchBSP_CallbackPARAM_PushChildren(root, root, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);

	while (stackTop != stackBase)
	{
		u_short childId;
		struct BSP *child;

		stackTop--;
		childId = *stackTop;
		child = &root[childId & 0x3fff];

		if ((childId & 0x4000) != 0)
		{
			callback(child, param);
			continue;
		}

		DECOMP_COLL_SearchBSP_CallbackPARAM_PushChildren(root, child, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);
	}
}
