// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 sc-bin
 *
 * K230 Chip ID driver - provides chip ID through /sys/class/chipid/chipid
 *
 * This driver reads 32 bytes of chip identification information
 * from a fixed SoC register address (0x91213300) on the Canaan
 * Kendryte K230 platform and exposes them via sysfs.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/string.h>

#define CHIPID_PHYS_ADDR	0x91213300UL
#define CHIPID_SIZE		32

static void __iomem *chipid_base;
static struct class *chipid_class;

static ssize_t chipid_show(const struct class *class,
			   const struct class_attribute *attr, char *buf)
{
	uint8_t chip_id[CHIPID_SIZE];
	int i, pos = 0;

	if (!chipid_base)
		return -EIO;

	for (i = 0; i < CHIPID_SIZE; i++)
		chip_id[i] = readb(chipid_base + i);

	for (i = 0; i < CHIPID_SIZE; i++)
		pos += sprintf(buf + pos, "%02x", chip_id[i]);
	buf[pos++] = '\n';
	buf[pos] = '\0';

	return pos;
}
static CLASS_ATTR_RO(chipid);

static int __init k230_chipid_init(void)
{
	int ret;

	chipid_base = ioremap(CHIPID_PHYS_ADDR, CHIPID_SIZE);
	if (!chipid_base) {
		pr_err("k230-chipid: ioremap failed\n");
		return -ENOMEM;
	}

	chipid_class = class_create("chipid");
	if (IS_ERR(chipid_class)) {
		pr_err("k230-chipid: failed to create chipid class\n");
		ret = PTR_ERR(chipid_class);
		goto err_iounmap;
	}

	ret = class_create_file(chipid_class, &class_attr_chipid);
	if (ret) {
		pr_err("k230-chipid: failed to create class attribute\n");
		goto err_destroy_class;
	}

	pr_info("k230-chipid: driver initialized\n");

	return 0;

err_destroy_class:
	class_destroy(chipid_class);
err_iounmap:
	iounmap(chipid_base);
	return ret;
}

static void __exit k230_chipid_exit(void)
{
	class_remove_file(chipid_class, &class_attr_chipid);
	class_destroy(chipid_class);
	iounmap(chipid_base);
}

module_init(k230_chipid_init);
module_exit(k230_chipid_exit);

MODULE_LICENSE("GPL");
