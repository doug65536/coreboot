/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC
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

#include "PlatformGnbPcieComplex.h"

#include <AGESA.h>
#include <northbridge/amd/agesa/state_machine.h>
#include <PlatformMemoryConfiguration.h>

static const PCIe_PORT_DESCRIPTOR PortList[] = {
	// Initialize Port descriptor (PCIe port, Lanes 4, PCI Device Number 4, ...)
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 4, 4),
		PCIE_PORT_DATA_INITIALIZER(GNB_GPP_PORT4_PORT_PRESENT, GNB_GPP_PORT4_CHANNEL_TYPE, 4, GNB_GPP_PORT4_HOTPLUG_SUPPORT, GNB_GPP_PORT4_SPEED_MODE, GNB_GPP_PORT4_SPEED_MODE, GNB_GPP_PORT4_LINK_ASPM, 4)
	},
	// Initialize Port descriptor (PCIe port, Lanes 5, PCI Device Number 5, ...)
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 5, 5),
		PCIE_PORT_DATA_INITIALIZER(GNB_GPP_PORT5_PORT_PRESENT, GNB_GPP_PORT5_CHANNEL_TYPE, 5, GNB_GPP_PORT5_HOTPLUG_SUPPORT, GNB_GPP_PORT5_SPEED_MODE, GNB_GPP_PORT5_SPEED_MODE, GNB_GPP_PORT5_LINK_ASPM, 5)
	},
	// Initialize Port descriptor (PCIe port, Lanes 6, PCI Device Number 6, ...)
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 6, 6),
		PCIE_PORT_DATA_INITIALIZER(GNB_GPP_PORT6_PORT_PRESENT, GNB_GPP_PORT6_CHANNEL_TYPE, 6, GNB_GPP_PORT6_HOTPLUG_SUPPORT, GNB_GPP_PORT6_SPEED_MODE, GNB_GPP_PORT6_SPEED_MODE, GNB_GPP_PORT6_LINK_ASPM, 6)
	},
	// Initialize Port descriptor (PCIe port, Lanes 7, PCI Device Number 7, ...)
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 7, 7),
		PCIE_PORT_DATA_INITIALIZER(GNB_GPP_PORT7_PORT_PRESENT, GNB_GPP_PORT7_CHANNEL_TYPE, 7, GNB_GPP_PORT7_HOTPLUG_SUPPORT, GNB_GPP_PORT7_SPEED_MODE, GNB_GPP_PORT7_SPEED_MODE, GNB_GPP_PORT7_LINK_ASPM, 7)
	},
	// Initialize Port descriptor (PCIe port, Lanes 8, PCI Device Number 8, ...)
	{
		DESCRIPTOR_TERMINATE_LIST,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 0, 3),
		PCIE_PORT_DATA_INITIALIZER(GNB_GPP_PORT8_PORT_PRESENT, GNB_GPP_PORT8_CHANNEL_TYPE, 8, GNB_GPP_PORT8_HOTPLUG_SUPPORT, GNB_GPP_PORT8_SPEED_MODE, GNB_GPP_PORT8_SPEED_MODE, GNB_GPP_PORT8_LINK_ASPM, 0)
	}
};

static const PCIe_COMPLEX_DESCRIPTOR PcieComplex = {
	.Flags        = DESCRIPTOR_TERMINATE_LIST,
	.SocketId     = 0,
	.PciePortList = PortList,
	.DdiLinkList  = NULL,
};

void board_BeforeInitEarly(struct sysinfo *cb, AMD_EARLY_PARAMS *InitEarly)
{
	InitEarly->GnbConfig.PcieComplexList = &PcieComplex;
	InitEarly->GnbConfig.PsppPolicy		= 0;
}

/*----------------------------------------------------------------------------------------
 *						CUSTOMER OVERIDES MEMORY TABLE
 *----------------------------------------------------------------------------------------
 */

/*
 *	Platform Specific Overriding Table allows IBV/OEM to pass in platform information to AGESA
 *	(e.g. MemClk routing, the number of DIMM slots per channel,...). If PlatformSpecificTable
 *	is populated, AGESA will base its settings on the data from the table. Otherwise, it will
 *	use its default conservative settings.
 */
static CONST PSO_ENTRY ROMDATA PlatformMemoryTable[] = {

	NUMBER_OF_DIMMS_SUPPORTED(ANY_SOCKET, ANY_CHANNEL, TWO_DIMM),
	NUMBER_OF_CHANNELS_SUPPORTED(ANY_SOCKET, ONE_DIMM),

	// APU soldered down memory uses memory CLK0 and CLK1 on CS0
	MEMCLK_DIS_MAP(ANY_SOCKET, ANY_CHANNEL, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

	// APU soldered down memory requires different seeds
#define WLSEED 0x08
#define RXSEED 0x40
	WRITE_LEVELING_SEED(ANY_SOCKET, ANY_CHANNEL, WLSEED, WLSEED, WLSEED, WLSEED, WLSEED, WLSEED, WLSEED, WLSEED, WLSEED),
	HW_RXEN_SEED( ANY_SOCKET, ANY_CHANNEL, RXSEED, RXSEED, RXSEED, RXSEED, RXSEED, RXSEED, RXSEED, RXSEED, RXSEED),

	PSO_END
};

void board_BeforeInitPost(struct sysinfo *cb, AMD_POST_PARAMS *InitPost)
{
	InitPost->MemConfig.PlatformMemoryConfiguration = (PSO_ENTRY *)PlatformMemoryTable;
}
