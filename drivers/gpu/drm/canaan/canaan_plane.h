/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022, Canaan Bright Sight Co., Ltd
 *
 * All enquiries to https://www.canaan-creative.com/
 *
 */

#ifndef __CANAAN_PLANE_H__
#define __CANAAN_PLANE_H__

#include <linux/dma-mapping.h>
#include <linux/mutex.h>

#include <drm/drm_fourcc.h>

#define CANAAN_ROTATE_BUFFER_COUNT 3

struct drm_plane;
struct drm_plane_state;
struct drm_gem_dma_object;

struct canaan_plane_config {
	char *name;
	uint32_t id;
	const uint32_t supported_rotations;
	uint32_t possible_crtcs;
	uint32_t num_formats;
	const uint32_t *formats;
	enum drm_plane_type plane_type;

	uint32_t plane_offset;
	uint32_t plane_enable_bit;
	uint32_t xctl_reg_offset;
	uint32_t yctl_reg_offset;
};

struct canaan_plane {
	struct drm_plane base;
	struct canaan_vo *vo;
	struct canaan_plane_config *config;
	uint32_t id;

	struct mutex rotate_lock;
	struct drm_gem_dma_object *rotate_pool[CANAAN_ROTATE_BUFFER_COUNT];
	size_t rotate_pool_size;
	u32 rotate_pool_width;
	u32 rotate_pool_height;
	u32 rotate_pool_fourcc;
};

struct canaan_plane_state {
	struct drm_plane_state base;
	struct drm_gem_dma_object *rotated_obj;
	dma_addr_t addr[DRM_FORMAT_MAX_PLANES];
	u32 pitch[DRM_FORMAT_MAX_PLANES];
	u32 width;
	u32 height;
};

static inline struct canaan_plane *to_canaan_plane(struct drm_plane *plane)
{
	return container_of(plane, struct canaan_plane, base);
}

static inline struct canaan_plane_state *
to_canaan_plane_state(struct drm_plane_state *state)
{
	return container_of(state, struct canaan_plane_state, base);
}

struct canaan_plane *canaan_plane_create(struct drm_device *drm_dev,
					 struct canaan_plane_config *config,
					 struct canaan_vo *vo);
#endif /* __CANAAN_PLANE_H__ */
