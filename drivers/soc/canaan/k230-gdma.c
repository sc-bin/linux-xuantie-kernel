// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026, Canaan Bright Sight Co., Ltd
 */

#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <drm/drm_blend.h>
#include <drm/drm_fourcc.h>

#include <linux/soc/canaan/k230-gdma.h>

#define K230_GDMA_DONE_INT			BIT(18)
#define K230_GDMA_INT_MASK			GENMASK(18, 16)
#define K230_GDMA_CH_EN				BIT(4)
#define K230_GDMA_LLT_ENTRY_SIZE		24
#define K230_GDMA_MAX_LLT_SIZE			(3 * K230_GDMA_LLT_ENTRY_SIZE)
#define K230_GDMA_DEFAULT_TIMEOUT_MS		100
#define K230_GDMA_DMA_CFG			((7 << 8) | 0xff)
#define K230_GDMA_DMA_WEIGHT			0x444210

enum k230_gdma_pixel_bits {
	K230_GDMA_BIT_8 = 0,
	K230_GDMA_BIT_16,
	K230_GDMA_BIT_24,
	K230_GDMA_BIT_32,
};

struct k230_gdma_plane_info {
	u8 pixel_bits;
	u16 width;
	u16 height;
};

struct k230_gdma_format_desc {
	u8 plane_count;
	struct k230_gdma_plane_info plane[3];
};

struct k230_gdma_llt {
	u32 rotation_mode:2;
	u32 x_mirror:1;
	u32 y_mirror:1;
	u32 reserved0:4;
	u32 pixel_width:2;
	u32 reserved1:6;
	u32 channel:3;
	u32 reserved2:10;
	u32 pause:1;
	u32 node_intr:1;
	u32 reserved3:1;
	u32 src_addr;
	u32 width:16;
	u32 height:16;
	u32 src_stride:16;
	u32 dst_stride:16;
	u32 dst_addr;
	u32 next_llt_addr;
} __packed;
static_assert(sizeof(struct k230_gdma_llt) == K230_GDMA_LLT_ENTRY_SIZE);

struct k230_gdma_regs {
	u32 dma_ch_en;
	u32 dma_int_mask;
	u32 dma_int_stat;
	u32 dma_cfg;
	u32 gdma_ctrl;
	u32 dma_gdmalli_base;
	u32 gdma_ch_cnt[8];
	u32 gdma_current_llt;
	u32 gdma_current_size;
	u32 gdma_current_saddr;
	u32 gdma_current_daddr;
	u32 dma_weight;
	u32 reserved0;
};

struct k230_gdma_dev {
	struct device *dev;
	void __iomem *base;
	int irq;
	struct mutex lock;
	struct completion done;
	void *llt_cpu;
	dma_addr_t llt_dma;
};

static struct k230_gdma_dev *k230_gdma_global;

static void k230_gdma_clear_irqs(struct k230_gdma_dev *gdma)
{
	writel(0xffffffff, gdma->base + offsetof(struct k230_gdma_regs, dma_int_stat));
}

static void k230_gdma_enable(struct k230_gdma_dev *gdma, bool enable)
{
	u32 val;

	val = readl(gdma->base + offsetof(struct k230_gdma_regs, dma_ch_en));
	if (enable)
		val |= K230_GDMA_CH_EN;
	else
		val &= ~K230_GDMA_CH_EN;

	writel(val, gdma->base + offsetof(struct k230_gdma_regs, dma_ch_en));
}

static void k230_gdma_abort(struct k230_gdma_dev *gdma)
{
	k230_gdma_enable(gdma, false);
	k230_gdma_clear_irqs(gdma);
	k230_gdma_enable(gdma, true);
}

static int k230_gdma_rotation_to_hw(unsigned int rotation)
{
	switch (rotation & DRM_MODE_ROTATE_MASK) {
	case DRM_MODE_ROTATE_0:
		return 0;
	case DRM_MODE_ROTATE_90:
		return 1;
	case DRM_MODE_ROTATE_180:
		return 2;
	case DRM_MODE_ROTATE_270:
		return 3;
	default:
		return -EINVAL;
	}
}

static bool k230_gdma_rotation_valid(unsigned int rotation)
{
	unsigned int rotate = rotation & DRM_MODE_ROTATE_MASK;

	if (!rotate || (rotate & (rotate - 1)))
		return false;

	return !(rotation & ~(DRM_MODE_ROTATE_MASK | DRM_MODE_REFLECT_MASK));
}

static bool k230_gdma_addr_valid(dma_addr_t addr)
{
	return !upper_32_bits(addr);
}

static int k230_gdma_validate_frame(const struct canaan_gdma_frame *frame,
				    const struct k230_gdma_format_desc *desc)
{
	int i;

	if (!frame->width || !frame->height ||
	    frame->width > U16_MAX || frame->height > U16_MAX)
		return -EINVAL;

	for (i = 0; i < desc->plane_count; i++) {
		if (!frame->pitch[i] || frame->pitch[i] > U16_MAX)
			return -EINVAL;

		if (!k230_gdma_addr_valid(frame->addr[i]))
			return -ERANGE;
	}

	return 0;
}

static int k230_gdma_validate_dst_size(const struct canaan_gdma_frame *src,
				       const struct canaan_gdma_frame *dst,
				       unsigned int rotation)
{
	bool rotate_90_270 = drm_rotation_90_or_270(rotation);
	u32 expected_width = rotate_90_270 ? src->height : src->width;
	u32 expected_height = rotate_90_270 ? src->width : src->height;

	if (dst->width != expected_width || dst->height != expected_height)
		return -EINVAL;

	return 0;
}

static int k230_gdma_build_format_desc(const struct canaan_gdma_frame *frame,
				       struct k230_gdma_format_desc *desc)
{
	const struct drm_format_info *info;
	int i;

	info = drm_format_info(frame->fourcc);
	if (!info)
		return -EINVAL;

	memset(desc, 0, sizeof(*desc));

	switch (frame->fourcc) {
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_RGB565:
		desc->plane_count = 1;
		desc->plane[0].pixel_bits = K230_GDMA_BIT_16;
		break;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		desc->plane_count = 1;
		desc->plane[0].pixel_bits = K230_GDMA_BIT_24;
		break;
	case DRM_FORMAT_ARGB8888:
		desc->plane_count = 1;
		desc->plane[0].pixel_bits = K230_GDMA_BIT_32;
		break;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		desc->plane_count = 2;
		desc->plane[0].pixel_bits = K230_GDMA_BIT_8;
		desc->plane[1].pixel_bits = K230_GDMA_BIT_16;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < desc->plane_count; i++) {
		desc->plane[i].width = drm_format_info_plane_width(info,
								   frame->width,
								   i);
		desc->plane[i].height = drm_format_info_plane_height(info,
								     frame->height,
								     i);
	}

	return 0;
}

static int k230_gdma_build_llt(struct k230_gdma_dev *gdma,
			       const struct canaan_gdma_frame *src,
			       const struct canaan_gdma_frame *dst,
			       unsigned int rotation)
{
	struct k230_gdma_format_desc desc;
	struct k230_gdma_llt *llt = gdma->llt_cpu;
	int rotate_hw;
	int ret;
	int i;

	if (!k230_gdma_rotation_valid(rotation))
		return -EINVAL;

	rotate_hw = k230_gdma_rotation_to_hw(rotation);

	ret = k230_gdma_build_format_desc(src, &desc);
	if (ret)
		return ret;

	ret = k230_gdma_validate_frame(src, &desc);
	if (ret)
		return ret;

	ret = k230_gdma_validate_frame(dst, &desc);
	if (ret)
		return ret;

	ret = k230_gdma_validate_dst_size(src, dst, rotation);
	if (ret)
		return ret;

	memset(llt, 0, K230_GDMA_MAX_LLT_SIZE);

	for (i = 0; i < desc.plane_count; i++) {
		llt[i].rotation_mode = rotate_hw;
		llt[i].x_mirror = !!(rotation & DRM_MODE_REFLECT_X);
		llt[i].y_mirror = !!(rotation & DRM_MODE_REFLECT_Y);
		llt[i].pixel_width = desc.plane[i].pixel_bits;
		llt[i].channel = 0;
		llt[i].node_intr = (i == desc.plane_count - 1);
		llt[i].src_addr = lower_32_bits(src->addr[i]);
		llt[i].width = desc.plane[i].width;
		llt[i].height = desc.plane[i].height;
		llt[i].src_stride = src->pitch[i];
		llt[i].dst_stride = dst->pitch[i];
		llt[i].dst_addr = lower_32_bits(dst->addr[i]);
		if (i != desc.plane_count - 1)
			llt[i].next_llt_addr = lower_32_bits(gdma->llt_dma +
							       (i + 1) * K230_GDMA_LLT_ENTRY_SIZE);
	}

	return 0;
}

static irqreturn_t k230_gdma_irq(int irq, void *data)
{
	struct k230_gdma_dev *gdma = data;
	u32 stat;

	stat = readl(gdma->base + offsetof(struct k230_gdma_regs, dma_int_stat));
	if (!(stat & K230_GDMA_DONE_INT))
		return IRQ_NONE;

	writel(stat, gdma->base + offsetof(struct k230_gdma_regs, dma_int_stat));
	complete(&gdma->done);

	return IRQ_HANDLED;
}

int canaan_gdma_rotate_sync(struct canaan_gdma_frame *src,
			    struct canaan_gdma_frame *dst,
			    unsigned int rotation,
			    unsigned int timeout_ms)
{
	struct k230_gdma_dev *gdma = k230_gdma_global;
	unsigned long timeout;
	int ret;
	u32 val;

	if (!gdma || !src || !dst)
		return -ENODEV;

	if (src->fourcc != dst->fourcc)
		return -EINVAL;

	if (!timeout_ms)
		timeout_ms = K230_GDMA_DEFAULT_TIMEOUT_MS;

	mutex_lock(&gdma->lock);

	reinit_completion(&gdma->done);

	ret = k230_gdma_build_llt(gdma, src, dst, rotation);
	if (ret)
		goto out_unlock;

	k230_gdma_clear_irqs(gdma);
	writel(lower_32_bits(gdma->llt_dma),
	       gdma->base + offsetof(struct k230_gdma_regs, dma_gdmalli_base));
	writel(0x1, gdma->base + offsetof(struct k230_gdma_regs, gdma_ctrl));

	timeout = wait_for_completion_timeout(&gdma->done,
					      msecs_to_jiffies(timeout_ms));
	if (!timeout) {
		k230_gdma_abort(gdma);
		dev_err_ratelimited(gdma->dev, "rotation timed out\n");
		ret = -ETIMEDOUT;
		goto out_unlock;
	}

	val = readl(gdma->base + offsetof(struct k230_gdma_regs, dma_int_stat));
	writel(val, gdma->base + offsetof(struct k230_gdma_regs, dma_int_stat));
	ret = 0;

out_unlock:
	mutex_unlock(&gdma->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(canaan_gdma_rotate_sync);

static int k230_gdma_probe(struct platform_device *pdev)
{
	struct k230_gdma_dev *gdma;
	struct resource *res;
	u32 val;
	int ret;

	gdma = devm_kzalloc(&pdev->dev, sizeof(*gdma), GFP_KERNEL);
	if (!gdma)
		return -ENOMEM;

	gdma->dev = &pdev->dev;
	mutex_init(&gdma->lock);
	init_completion(&gdma->done);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gdma->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gdma->base))
		return PTR_ERR(gdma->base);

	gdma->irq = platform_get_irq(pdev, 0);
	if (gdma->irq < 0)
		return gdma->irq;

	ret = devm_request_irq(&pdev->dev, gdma->irq, k230_gdma_irq, 0,
			       dev_name(&pdev->dev), gdma);
	if (ret)
		return ret;

	gdma->llt_cpu = dmam_alloc_coherent(&pdev->dev, K230_GDMA_MAX_LLT_SIZE,
					    &gdma->llt_dma, GFP_KERNEL);
	if (!gdma->llt_cpu)
		return -ENOMEM;

	k230_gdma_clear_irqs(gdma);
	writel(0xffffffff, gdma->base + offsetof(struct k230_gdma_regs, dma_int_mask));

	k230_gdma_enable(gdma, true);

	val = readl(gdma->base + offsetof(struct k230_gdma_regs, dma_int_mask));
	writel(val & ~K230_GDMA_INT_MASK,
	       gdma->base + offsetof(struct k230_gdma_regs, dma_int_mask));

	writel(K230_GDMA_DMA_CFG, gdma->base + offsetof(struct k230_gdma_regs, dma_cfg));
	writel(K230_GDMA_DMA_WEIGHT,
	       gdma->base + offsetof(struct k230_gdma_regs, dma_weight));

	platform_set_drvdata(pdev, gdma);
	k230_gdma_global = gdma;

	return 0;
}

static int k230_gdma_remove(struct platform_device *pdev)
{
	struct k230_gdma_dev *gdma = platform_get_drvdata(pdev);
	u32 val;

	val = readl(gdma->base + offsetof(struct k230_gdma_regs, dma_int_mask));
	writel(val | K230_GDMA_INT_MASK,
	       gdma->base + offsetof(struct k230_gdma_regs, dma_int_mask));

	k230_gdma_enable(gdma, false);

	if (k230_gdma_global == gdma)
		k230_gdma_global = NULL;

	return 0;
}

static const struct of_device_id k230_gdma_of_match[] = {
	{ .compatible = "canaan,gdma" },
	{ }
};
MODULE_DEVICE_TABLE(of, k230_gdma_of_match);

static struct platform_driver canaan_gdma_driver = {
	.probe = k230_gdma_probe,
	.remove = k230_gdma_remove,
	.driver = {
		.name = "k230-gdma",
		.of_match_table = k230_gdma_of_match,
	},
};
module_platform_driver(canaan_gdma_driver);

MODULE_DESCRIPTION("Canaan K230 GDMA driver");
MODULE_LICENSE("GPL");
