/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
 * Copyright (C) 2017 Siemens AG
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

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <hwilib.h>
#include <i210.h>
#include <intelblocks/lpc_lib.h>
#include <soc/pci_devs.h>
#include <string.h>
#include <bootstate.h>
#include <timer.h>
#include <timestamp.h>
#include "brd_gpio.h"
#include "ptn3460.h"

#define MAX_PATH_DEPTH		12
#define MAX_NUM_MAPPINGS	10

/** \brief This function can decide if a given MAC address is valid or not.
 *         Currently, addresses filled with 0xff or 0x00 are not valid.
 * @param  mac  Buffer to the MAC address to check
 * @return 0    if address is not valid, otherwise 1
 */
static uint8_t is_mac_adr_valid(uint8_t mac[6])
{
	uint8_t buf[6];

	memset(buf, 0, sizeof(buf));
	if (!memcmp(buf, mac, sizeof(buf)))
		return 0;
	memset(buf, 0xff, sizeof(buf));
	if (!memcmp(buf, mac, sizeof(buf)))
		return 0;
	return 1;
}

/** \brief This function will search for a MAC address which can be assigned
 *         to a MACPHY.
 * @param  dev     pointer to PCI device
 * @param  mac     buffer where to store the MAC address
 * @return cb_err  CB_ERR or CB_SUCCESS
 */
enum cb_err mainboard_get_mac_address(struct device *dev, uint8_t mac[6])
{
	struct bus *parent = dev->bus;
	uint8_t buf[16], mapping[16], i = 0, chain_len = 0;

	memset(buf, 0, sizeof(buf));
	memset(mapping, 0, sizeof(mapping));

	/* The first entry in the tree is the device itself. */
	buf[0] = dev->path.pci.devfn;
	chain_len = 1;
	for (i = 1; i < MAX_PATH_DEPTH && parent->dev->bus->subordinate; i++) {
		buf[i] = parent->dev->path.pci.devfn;
		chain_len++;
		parent = parent->dev->bus;
	}
	if (i == MAX_PATH_DEPTH) {
		/* The path is deeper than MAX_PATH_DEPTH devices, error. */
		printk(BIOS_ERR, "Too many bridges for %s\n", dev_path(dev));
		return CB_ERR;
	}
	/*
	 * Now construct the mapping based on the device chain starting from
	 * root bridge device to the device itself.
	 */
	mapping[0] = 1;
	mapping[1] = chain_len;
	for (i = 0; i < chain_len; i++)
		mapping[i + 4] = buf[chain_len - i - 1];

	/* Open main hwinfo block */
	if (hwilib_find_blocks("hwinfo.hex") != CB_SUCCESS)
		return CB_ERR;
	/* Now try to find a valid MAC address in hwinfo for this mapping.*/
	for (i = 0; i < MAX_NUM_MAPPINGS; i++) {
		if ((hwilib_get_field(XMac1Mapping + i, buf, 16) == 16) &&
			!(memcmp(buf, mapping, chain_len + 4))) {
		/* There is a matching mapping available, get MAC address. */
			if ((hwilib_get_field(XMac1 + i, mac, 6) == 6) &&
			    (is_mac_adr_valid(mac))) {
				return CB_SUCCESS;
			} else {
				return CB_ERR;
			}
		} else
			continue;
	}
	/* No MAC address found for */
	return CB_ERR;
}

static void mainboard_init(void *chip_info)
{
	const struct pad_config *pads;
	size_t num;

	pads = brd_gpio_table(&num);
	gpio_configure_pads(pads, num);
}

static void mainboard_final(void *chip_info)
{
	int status;
	uint16_t cmd = 0;
	device_t dev = NULL;

	/**
	 * Set up the DP2LVDS converter.
	 * ptn3460_init() may only be executed after i2c bus init.
	 */
	status = ptn3460_init("hwinfo.hex");
	if (status)
		printk(BIOS_ERR, "LCD: Set up PTN with status 0x%x\n", status);
	else
		printk(BIOS_INFO, "LCD: Set up PTN was successful.\n");

	/* Enable additional I/O decoding range on LPC for COM 3 */
	lpc_open_pmio_window(0x3e8, 8);

	/* Set Master Enable for on-board PCI device. */
	dev = dev_find_device(PCI_VENDOR_ID_SIEMENS, 0x403f, 0);
	if (dev) {
		cmd = pci_read_config16(dev, PCI_COMMAND);
		cmd |= PCI_COMMAND_MASTER;
		pci_write_config16(dev, PCI_COMMAND, cmd);
	}
}

static void wait_for_legacy_dev(void *unused)
{
	uint32_t legacy_delay, us_since_boot;
	struct stopwatch sw;

	/* Open main hwinfo block. */
	if (hwilib_find_blocks("hwinfo.hex") != CB_SUCCESS)
		return;

	/* Get legacy delay parameter from hwinfo. */
	if (hwilib_get_field(LegacyDelay, (uint8_t *) &legacy_delay,
			      sizeof(legacy_delay)) != sizeof(legacy_delay))
		return;

	us_since_boot = get_us_since_boot();
	/* No need to wait if the time since boot is already long enough.*/
	if (us_since_boot > legacy_delay)
		return;
	stopwatch_init_msecs_expire(&sw, (legacy_delay - us_since_boot) / 1000);
	printk(BIOS_NOTICE, "Wait remaining %d of %d us for legacy devices...",
			legacy_delay - us_since_boot, legacy_delay);
	stopwatch_wait_until_expired(&sw);
	printk(BIOS_NOTICE, "done!\n");
}

BOOT_STATE_INIT_ENTRY(BS_DEV_ENUMERATE, BS_ON_ENTRY, wait_for_legacy_dev, NULL);

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.final = mainboard_final,
};
