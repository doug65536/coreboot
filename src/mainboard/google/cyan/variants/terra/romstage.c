/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <soc/romstage.h>
#include <chip.h>
#include <mainboard/google/cyan/spd/spd_util.h>

void mainboard_memory_init_params(struct romstage_params *params,
	MEMORY_INIT_UPD *memory_params)
{
	int ram_id = get_ramid();

	/*
	 *  RAMID = 3  - 2GiB Micron MT52L256M32D1PF-107
	 *  RAMID = 11 - 4GiB Micron MT52L256M32D1PF-107
	 */
	if (ram_id == 3 || ram_id == 11) {

		/*
		 * For new micron part, it requires read/receive
		 * enable training before sending cmds to get MR8.
		 * To override dram geometry settings as below:
		 *
		 * PcdDramWidth = x32
		 * PcdDramDensity = 8Gb
		 * PcdDualRankDram = disable
		 */
		memory_params->PcdRxOdtLimitChannel0 = 1;
		memory_params->PcdRxOdtLimitChannel1 = 1;
		memory_params->PcdDisableAutoDetectDram = 1;
		memory_params->PcdDramWidth = 2;
		memory_params->PcdDramDensity = 3;
		memory_params->PcdDualRankDram = 0;
	}

	/* Update SPD data */
	memory_params->PcdMemoryTypeEnable = MEM_LPDDR3;
	memory_params->PcdMemorySpdPtr = (u32)params->pei_data->spd_data_ch0;
	memory_params->PcdMemChannel0Config = params->pei_data->spd_ch0_config;
	memory_params->PcdMemChannel1Config = params->pei_data->spd_ch1_config;
}
