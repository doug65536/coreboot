/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

#include <bootstate.h>
#include <device/device.h>
#include <device/pci.h>
#include <console/console.h>
#include <arch/io.h>
#include <cpu/cpu.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/smm.h>
#include <string.h>
#include <soc/iomap.h>
#include <soc/pch.h>
#include <soc/pm.h>
#include <soc/smm.h>

void southbridge_smm_clear_state(void)
{
	u32 smi_en;

	printk(BIOS_DEBUG, "Initializing Southbridge SMI...");
	printk(BIOS_SPEW, " ... pmbase = 0x%04x\n", ACPI_BASE_ADDRESS);

	smi_en = inl(ACPI_BASE_ADDRESS + SMI_EN);
	if (smi_en & APMC_EN) {
		printk(BIOS_INFO, "SMI# handler already enabled?\n");
		return;
	}

	printk(BIOS_DEBUG, "\n");

	/* Dump and clear status registers */
	clear_smi_status();
	clear_pm1_status();
	clear_tco_status();
	clear_gpe_status();
}

void southbridge_smm_enable_smi(void)
{
	printk(BIOS_DEBUG, "Enabling SMIs.\n");
	/* Configure events */
	enable_pm1(GBL_EN);
	disable_gpe(PME_B0_EN);

	/*
	 * Enable SMI generation:
	 *  - on APMC writes (io 0xb2)
	 *  - on writes to SLP_EN (sleep states)
	 *  - on writes to GBL_RLS (bios commands)
	 *  - on eSPI events (does nothing on LPC systems)
	 * No SMIs:
	 *  - on microcontroller writes (io 0x62/0x66)
	 *  - on TCO events
	 */
	enable_smi(APMC_EN | SLP_SMI_EN | GBL_SMI_EN | ESPI_SMI_EN | EOS);
}

void smm_setup_structures(void *gnvs, void *tcg, void *smi1)
{
	/*
	 * Issue SMI to set the gnvs pointer in SMM.
	 * tcg and smi1 are unused.
	 *
	 * EAX = APM_CNT_GNVS_UPDATE
	 * EBX = gnvs pointer
	 * EDX = APM_CNT
	 */
	asm volatile (
		"outb %%al, %%dx\n\t"
		: /* ignore result */
		: "a" (APM_CNT_GNVS_UPDATE),
		  "b" ((u32)gnvs),
		  "d" (APM_CNT)
	);
}

static void pm1_enable_pwrbtn_smi(void *unused)
{
	/*
	 * Enable power button SMI only before jumping to payload. This ensures
	 * that:
	 * 1. Power button SMI is enabled only after coreboot is done.
	 * 2. On resume path, power button SMI is not enabled and thus avoids
	 * any shutdowns because of power button presses due to power button
	 * press in resume path.
	 */
	update_pm1_enable(PWRBTN_EN);
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, pm1_enable_pwrbtn_smi, NULL);
