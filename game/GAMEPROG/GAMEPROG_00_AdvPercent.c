#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800265c0-0x8002689c.
void GAMEPROG_AdvPercent(struct AdvProgress *adv)
{
	int i;
	int percent;
	int oxidePercent;
	int allGoldOrPlatinumRelics;
	int numGems;
	int bitIndex;
	struct MetaDataLEV *mdLev;
	struct GameTracker *gGT;
	gGT = sdata->gGT;
	mdLev = &data.metaDataLEV[0];

	// start counter
	percent = 0;
	oxidePercent = 0;
	allGoldOrPlatinumRelics = 1;
	numGems = 0;

	// erase counters
	for (i = 0; i < 9; i++)
	{
		((int *)&gGT->currAdvProfile.numTrophies)[i] = 0;
	}

	// check all tracks generically
	for (i = 0; i < 18; i++)
	{
		// first bit of blue relic
		bitIndex = 0x16 + i;
		if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
		{
			gGT->currAdvProfile.numRelics++;
		}

		// check 16 trophies
		if (i < 0x10)
		{
			// first bit of trophy
			bitIndex = 6 + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				gGT->currAdvProfile.numTrophies++;
			}

			// first bit of token
			bitIndex = 0x4c + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				// increment number of tokens, based on
				// the tokenID of this level (red, green, blue, etc)
				((int *)&gGT->currAdvProfile.numCtrTokens.red)[mdLev[i].ctrTokenGroupID]++;

				// increment number of total tokens
				gGT->currAdvProfile.numCtrTokens.total++;
			}
		}

		// check 4 keys, and 4 purple tokens
		if (i < 4)
		{
			// first bit of key
			bitIndex = 0x5e + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				gGT->currAdvProfile.numKeys++;
			}

			// first bit of purple tokens
			bitIndex = 0x6f + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				gGT->currAdvProfile.numCtrTokens.purple++;
			}
		}

		// check 5 gems
		if (i < 5)
		{
			// first bit of gem
			bitIndex = 0x6a + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				numGems++;
			}
		}

		// first bit is 2%, second bit upgrades the total Oxide bonus to 3%
		if (i < 2)
		{
			// first bit of beating oxide
			bitIndex = 0x73 + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0)
			{
				oxidePercent = (i == 0) ? 2 : 3;
			}
		}
	}

	// check whether all tracks have gold or platinum relic
	for (i = 0; i < 18; i++)
	{
		// first bit of gold relic
		bitIndex = 0x28 + i;
		if ((allGoldOrPlatinumRelics != 0) && (CHECK_ADV_BIT(adv->rewards, bitIndex) != 0))
		{
			// check next relic
			continue;
		}

		// if relic is not unlocked,
		// then extra 1% is not earned
		allGoldOrPlatinumRelics = 0;
	}

	percent += gGT->currAdvProfile.numRelics * 2 + gGT->currAdvProfile.numTrophies * 2 + gGT->currAdvProfile.numKeys + gGT->currAdvProfile.numCtrTokens.total +
	           gGT->currAdvProfile.numCtrTokens.purple + numGems + oxidePercent + allGoldOrPlatinumRelics;

	gGT->currAdvProfile.completionPercent = percent;
}
