/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2026, Canaan Bright Sight Co., Ltd
 */

#ifndef __LINUX_SOC_CANAAN_K230_GDMA_H__
#define __LINUX_SOC_CANAAN_K230_GDMA_H__

#include <linux/dma-mapping.h>

#define CANAAN_GDMA_MAX_PLANES 3

struct canaan_gdma_frame {
	dma_addr_t addr[CANAAN_GDMA_MAX_PLANES];
	u32 pitch[CANAAN_GDMA_MAX_PLANES];
	u32 width;
	u32 height;
	u32 fourcc;
};

int canaan_gdma_rotate_sync(struct canaan_gdma_frame *src,
			    struct canaan_gdma_frame *dst,
			    unsigned int rotation,
			    unsigned int timeout_ms);

#endif /* __LINUX_SOC_CANAAN_K230_GDMA_H__ */
