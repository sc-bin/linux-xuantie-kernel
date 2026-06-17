// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022, Canaan Bright Sight Co., Ltd
 *
 * All enquiries to https://www.canaan-creative.com/
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/component.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_vblank.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_print.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_framebuffer.h>

#include <linux/dma-fence.h>
#include <linux/kref.h>

#include <linux/soc/canaan/k230-gdma.h>

#include "canaan_vo.h"
#include "canaan_crtc.h"
#include "canaan_plane.h"

#define CANAAN_ROTATE_PITCH_ALIGN	8

static void canaan_plane_clear_scanout(struct canaan_plane_state *canaan_state)
{
	memset(canaan_state->addr, 0, sizeof(canaan_state->addr));
	memset(canaan_state->pitch, 0, sizeof(canaan_state->pitch));
	canaan_state->width = 0;
	canaan_state->height = 0;
}

static u32 canaan_plane_min_pitch(const struct drm_format_info *info,
				  int plane, u32 width)
{
	u32 plane_w = drm_format_info_plane_width(info, width, plane);

	return drm_format_info_min_pitch(info, plane, plane_w);
}

static u32 canaan_plane_rotate_pitch(const struct drm_format_info *info,
				     int plane, u32 width)
{
	return ALIGN(canaan_plane_min_pitch(info, plane, width),
		     CANAAN_ROTATE_PITCH_ALIGN);
}

static void
canaan_plane_rotate_pool_release_locked(struct canaan_plane *canaan_plane)
{
	int i;

	for (i = 0; i < CANAAN_ROTATE_BUFFER_COUNT; i++) {
		if (canaan_plane->rotate_pool[i]) {
			drm_gem_object_put(&canaan_plane->rotate_pool[i]->base);
			canaan_plane->rotate_pool[i] = NULL;
		}
	}

	canaan_plane->rotate_pool_size = 0;
	canaan_plane->rotate_pool_width = 0;
	canaan_plane->rotate_pool_height = 0;
	canaan_plane->rotate_pool_fourcc = 0;
}

static void canaan_plane_rotate_pool_release(struct canaan_plane *canaan_plane)
{
	mutex_lock(&canaan_plane->rotate_lock);
	canaan_plane_rotate_pool_release_locked(canaan_plane);
	mutex_unlock(&canaan_plane->rotate_lock);
}

static bool
canaan_plane_rotate_pool_matches(struct canaan_plane *canaan_plane,
				 const struct drm_format_info *info,
				 u32 width, u32 height, size_t size)
{
	return canaan_plane->rotate_pool[0] &&
	       canaan_plane->rotate_pool_size == size &&
	       canaan_plane->rotate_pool_width == width &&
	       canaan_plane->rotate_pool_height == height &&
	       canaan_plane->rotate_pool_fourcc == info->format;
}

static int
canaan_plane_rotate_pool_create_locked(struct canaan_plane *canaan_plane,
				       const struct drm_format_info *info,
				       u32 width, u32 height, size_t size)
{
	int i;

	for (i = 0; i < CANAAN_ROTATE_BUFFER_COUNT; i++) {
		struct drm_gem_dma_object *obj;

		obj = drm_gem_dma_create(canaan_plane->base.dev, size);
		if (IS_ERR(obj)) {
			canaan_plane_rotate_pool_release_locked(canaan_plane);
			return PTR_ERR(obj);
		}

		canaan_plane->rotate_pool[i] = obj;
	}

	canaan_plane->rotate_pool_size = size;
	canaan_plane->rotate_pool_width = width;
	canaan_plane->rotate_pool_height = height;
	canaan_plane->rotate_pool_fourcc = info->format;

	return 0;
}

static struct drm_gem_dma_object *
canaan_plane_rotate_pool_get(struct canaan_plane *canaan_plane,
			     const struct drm_format_info *info,
			     u32 width, u32 height, size_t size)
{
	struct drm_gem_dma_object *obj;
	int ret;
	int i;

	mutex_lock(&canaan_plane->rotate_lock);

	if (!canaan_plane_rotate_pool_matches(canaan_plane, info,
					      width, height, size)) {
		canaan_plane_rotate_pool_release_locked(canaan_plane);

		ret = canaan_plane_rotate_pool_create_locked(canaan_plane,
							     info, width,
							     height, size);
		if (ret) {
			mutex_unlock(&canaan_plane->rotate_lock);
			return ERR_PTR(ret);
		}
	}

	for (i = 0; i < CANAAN_ROTATE_BUFFER_COUNT; i++) {
		obj = canaan_plane->rotate_pool[i];
		if (kref_read(&obj->base.refcount) != 1)
			continue;

		drm_gem_object_get(&obj->base);
		mutex_unlock(&canaan_plane->rotate_lock);
		return obj;
	}

	mutex_unlock(&canaan_plane->rotate_lock);

	return ERR_PTR(-EBUSY);
}

static int canaan_plane_fill_scanout_from_fb(struct drm_plane_state *plane_state,
					     struct canaan_plane_state *canaan_state)
{
	struct drm_framebuffer *fb = plane_state->fb;
	const struct drm_format_info *info = fb->format;
	int i;

	canaan_plane_clear_scanout(canaan_state);

	for (i = 0; i < info->num_planes; i++) {
		struct drm_gem_dma_object *obj = drm_fb_dma_get_gem_obj(fb, i);

		if (!obj)
			return -EINVAL;

		canaan_state->addr[i] = obj->dma_addr + fb->offsets[i];
		canaan_state->pitch[i] = fb->pitches[i];
	}

	canaan_state->width = plane_state->src_w >> 16;
	canaan_state->height = plane_state->src_h >> 16;
	canaan_state->rotated_obj = NULL;

	return 0;
}

static size_t canaan_plane_rotated_size(const struct drm_format_info *info,
					u32 width, u32 height)
{
	size_t size = 0;
	int i;

	for (i = 0; i < info->num_planes; i++) {
		u32 plane_h = drm_format_info_plane_height(info, height, i);

		size += canaan_plane_rotate_pitch(info, i, width) * plane_h;
	}

	return PAGE_ALIGN(size);
}

static bool canaan_plane_is_semiplanar_yuv(u32 fourcc)
{
	switch (fourcc) {
	case DRM_FORMAT_NV12:
#if 0
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
#endif
		return true;
	default:
		return false;
	}
}

static bool canaan_plane_uses_v4l2_yuv_layout(struct drm_plane_state *plane_state,
					      u32 width, u32 height)
{
	struct drm_framebuffer *fb = plane_state->fb;

	/* V4L2 userspace may allocate the DRM FB as the rotated size. */
	return canaan_plane_is_semiplanar_yuv(fb->format->format) &&
	       drm_rotation_90_or_270(plane_state->rotation) &&
	       plane_state->crtc_w == width &&
	       plane_state->crtc_h == height;
}

static bool canaan_plane_rotation_valid(unsigned int rotation)
{
	unsigned int rotate = rotation & DRM_MODE_ROTATE_MASK;

	if (!rotate || (rotate & (rotate - 1)))
		return false;

	return !(rotation & ~(DRM_MODE_ROTATE_MASK | DRM_MODE_REFLECT_MASK));
}

static int canaan_plane_fill_gdma_src(struct drm_plane_state *plane_state,
				      struct canaan_gdma_frame *src,
				      u32 width, u32 height,
				      bool use_min_pitch)
{
	struct drm_framebuffer *fb = plane_state->fb;
	const struct drm_format_info *info = fb->format;
	int i;

	src->width = width;
	src->height = height;
	src->fourcc = info->format;

	for (i = 0; i < info->num_planes; i++) {
		struct drm_gem_dma_object *obj = drm_fb_dma_get_gem_obj(fb, i);

		if (!obj)
			return -EINVAL;

		src->addr[i] = obj->dma_addr + fb->offsets[i];
		src->pitch[i] = use_min_pitch ?
				canaan_plane_min_pitch(info, i, width) :
				fb->pitches[i];
	}

	return 0;
}

static void canaan_plane_fill_gdma_dst(struct drm_plane_state *plane_state,
				       struct canaan_plane_state *canaan_state,
				       struct canaan_gdma_frame *dst,
				       struct drm_gem_dma_object *obj,
				       u32 width, u32 height)
{
	struct drm_framebuffer *fb = plane_state->fb;
	const struct drm_format_info *info = fb->format;
	dma_addr_t base = obj->dma_addr;
	int i;

	canaan_plane_clear_scanout(canaan_state);

	dst->width = width;
	dst->height = height;
	dst->fourcc = info->format;

	for (i = 0; i < info->num_planes; i++) {
		u32 plane_h = drm_format_info_plane_height(info, height, i);
		u32 pitch = canaan_plane_rotate_pitch(info, i, width);

		canaan_state->addr[i] = base;
		canaan_state->pitch[i] = pitch;
		dst->addr[i] = base;
		dst->pitch[i] = pitch;
		base += pitch * plane_h;
	}

	canaan_state->width = width;
	canaan_state->height = height;
	canaan_state->rotated_obj = obj;
}

static int canaan_plane_prepare_fb(struct drm_plane *plane,
				   struct drm_plane_state *new_state)
{
	struct canaan_plane *canaan_plane = to_canaan_plane(plane);
	struct canaan_plane_state *canaan_state = to_canaan_plane_state(new_state);
	struct drm_framebuffer *fb = new_state->fb;
	struct drm_gem_dma_object *rotated_obj;
	struct canaan_gdma_frame src = { 0 };
	struct canaan_gdma_frame dst = { 0 };
	unsigned int src_width, src_height;
	unsigned int dst_width, dst_height;
	unsigned int fb_width, fb_height;
	size_t rotated_size;
	bool use_v4l2_layout;
	bool rotate_90_270;
	int ret;

	if (!fb) {
		canaan_plane_clear_scanout(canaan_state);
		canaan_plane_rotate_pool_release(canaan_plane);
		return 0;
	}

	ret = drm_gem_plane_helper_prepare_fb(plane, new_state);
	if (ret)
		return ret;

	if (new_state->rotation == DRM_MODE_ROTATE_0) {
		canaan_plane_rotate_pool_release(canaan_plane);
		return canaan_plane_fill_scanout_from_fb(new_state, canaan_state);
	}

	if (!canaan_plane_rotation_valid(new_state->rotation))
		return -EINVAL;

	if (new_state->fence) {
		ret = dma_fence_wait(new_state->fence, true);
		if (ret < 0)
			return ret;

		dma_fence_put(new_state->fence);
		new_state->fence = NULL;
	}

	fb_width = new_state->src_w >> 16;
	fb_height = new_state->src_h >> 16;
	rotate_90_270 = drm_rotation_90_or_270(new_state->rotation);
	use_v4l2_layout = canaan_plane_uses_v4l2_yuv_layout(new_state,
							     fb_width,
							     fb_height);

	src_width = use_v4l2_layout ? fb_height : fb_width;
	src_height = use_v4l2_layout ? fb_width : fb_height;
	dst_width = rotate_90_270 && !use_v4l2_layout ? fb_height : fb_width;
	dst_height = rotate_90_270 && !use_v4l2_layout ? fb_width : fb_height;

	rotated_size = canaan_plane_rotated_size(fb->format, dst_width,
						 dst_height);
	rotated_obj = canaan_plane_rotate_pool_get(canaan_plane, fb->format,
						   dst_width, dst_height,
						   rotated_size);
	if (IS_ERR(rotated_obj))
		return PTR_ERR(rotated_obj);

	ret = canaan_plane_fill_gdma_src(new_state, &src, src_width,
					 src_height, use_v4l2_layout);
	if (ret)
		goto err_put_obj;

	canaan_plane_fill_gdma_dst(new_state, canaan_state, &dst, rotated_obj,
				   dst_width, dst_height);

	ret = canaan_gdma_rotate_sync(&src, &dst, new_state->rotation, 100);
	if (ret)
		goto err_cleanup_state;

	return 0;

err_cleanup_state:
	canaan_state->rotated_obj = NULL;
	canaan_plane_clear_scanout(canaan_state);
err_put_obj:
	drm_gem_object_put(&rotated_obj->base);
	return ret;
}

static void canaan_plane_cleanup_fb(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct canaan_plane_state *canaan_state = to_canaan_plane_state(old_state);

	if (canaan_state->rotated_obj) {
		drm_gem_object_put(&canaan_state->rotated_obj->base);
		canaan_state->rotated_obj = NULL;
	}

	canaan_plane_clear_scanout(canaan_state);
}

static void canaan_plane_reset(struct drm_plane *plane)
{
	struct canaan_plane_state *state;

	if (plane->state) {
		state = to_canaan_plane_state(plane->state);
		if (state->rotated_obj)
			drm_gem_object_put(&state->rotated_obj->base);

		__drm_atomic_helper_plane_destroy_state(plane->state);
		memset(state, 0, sizeof(*state));
	} else {
		state = kzalloc(sizeof(*state), GFP_KERNEL);
		if (!state)
			return;
	}

	__drm_atomic_helper_plane_reset(plane, &state->base);
}

static struct drm_plane_state *
canaan_plane_duplicate_state(struct drm_plane *plane)
{
	struct canaan_plane_state *old_state = to_canaan_plane_state(plane->state);
	struct canaan_plane_state *state;

	state = kmalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &state->base);
	memcpy(state->addr, old_state->addr, sizeof(state->addr));
	memcpy(state->pitch, old_state->pitch, sizeof(state->pitch));
	state->width = old_state->width;
	state->height = old_state->height;
	state->rotated_obj = NULL;

	return &state->base;
}

static void canaan_plane_destroy_state(struct drm_plane *plane,
				       struct drm_plane_state *state)
{
	struct canaan_plane_state *canaan_state = to_canaan_plane_state(state);

	if (canaan_state->rotated_obj)
		drm_gem_object_put(&canaan_state->rotated_obj->base);

	__drm_atomic_helper_plane_destroy_state(state);
	kfree(canaan_state);
}

static void canaan_plane_destroy(struct drm_plane *plane)
{
	struct canaan_plane *canaan_plane = to_canaan_plane(plane);

	canaan_plane_rotate_pool_release(canaan_plane);
	drm_plane_cleanup(plane);
}

static int canaan_plane_atomic_check(struct drm_plane *plane,
				     struct drm_atomic_state *state)
{
	struct drm_plane_state *plane_state =
		drm_atomic_get_new_plane_state(state, plane);
	struct canaan_plane *canaan_plane = to_canaan_plane(plane);
	struct canaan_vo *vo = canaan_plane->vo;

	if (!plane_state->crtc || !plane_state->fb) {
		DRM_DEBUG_DRIVER("crtc or fb NULL\n");
		return 0;
	}

	DRM_DEBUG_DRIVER("Check plane:%d\n", plane->base.id);
	DRM_DEBUG_DRIVER("(%d,%d)@(%d,%d) -> (%d,%d)@(%d,%d)\n",
			 plane_state->src_w >> 16, plane_state->src_h >> 16,
			 plane_state->src_x >> 16, plane_state->src_y >> 16,
			 plane_state->crtc_w, plane_state->crtc_h,
			 plane_state->crtc_x, plane_state->crtc_y);

	if (!canaan_plane_rotation_valid(plane_state->rotation))
		return -EINVAL;

	return canaan_vo_check_plane(vo, canaan_plane, plane_state);
}

static void canaan_plane_atomic_update(struct drm_plane *plane,
				       struct drm_atomic_state *state)
{
	struct drm_plane_state *plane_state = plane->state;
	struct canaan_plane *canaan_plane = to_canaan_plane(plane);
	struct canaan_vo *vo = canaan_plane->vo;

	if (!plane_state->crtc || !plane_state->fb) {
		DRM_DEBUG_DRIVER("crtc or fb NULL\n");
		return;
	}

	DRM_DEBUG_DRIVER("Update plane:%d\n", plane->base.id);
	canaan_vo_update_plane(vo, canaan_plane, plane_state);
}

static void canaan_plane_atomic_disable(struct drm_plane *plane,
					struct drm_atomic_state *state)
{
	struct canaan_plane *canaan_plane = to_canaan_plane(plane);
	struct canaan_vo *vo = canaan_plane->vo;

	DRM_DEBUG_DRIVER("Disable plane:%d\n", plane->base.id);
	canaan_vo_disable_plane(vo, canaan_plane);
}

static const struct drm_plane_funcs canaan_plane_funcs = {
	.reset = canaan_plane_reset,
	.destroy = canaan_plane_destroy,
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.atomic_duplicate_state = canaan_plane_duplicate_state,
	.atomic_destroy_state = canaan_plane_destroy_state,
};

static const struct drm_plane_helper_funcs canaan_plane_helper_funcs = {
	.atomic_check = canaan_plane_atomic_check,
	.atomic_update = canaan_plane_atomic_update,
	.atomic_disable = canaan_plane_atomic_disable,
	.prepare_fb = canaan_plane_prepare_fb,
	.cleanup_fb = canaan_plane_cleanup_fb,
};

struct canaan_plane *canaan_plane_create(struct drm_device *drm_dev,
					 struct canaan_plane_config *config,
					 struct canaan_vo *vo)
{
	int ret = 0;
	struct canaan_plane *canaan_plane = NULL;
	struct drm_plane *plane = NULL;
	struct device *dev = vo->dev;

	canaan_plane = devm_kzalloc(dev, sizeof(*canaan_plane), GFP_KERNEL);
	if (!canaan_plane)
		return ERR_PTR(-ENOMEM);
	mutex_init(&canaan_plane->rotate_lock);
	plane = &canaan_plane->base;

	ret = drm_universal_plane_init(drm_dev, plane, config->possible_crtcs,
				       &canaan_plane_funcs, config->formats,
				       config->num_formats, NULL,
				       config->plane_type, NULL);
	if (ret) {
		DRM_DEV_ERROR(dev, "Failed to init Plane\n");
		return ERR_PTR(ret);
	}

	if (config->supported_rotations) {
		drm_plane_create_rotation_property(&canaan_plane->base, DRM_MODE_ROTATE_0,
						   DRM_MODE_ROTATE_0 |
						   config->supported_rotations);
	}

	drm_plane_helper_add(plane, &canaan_plane_helper_funcs);
	canaan_plane->id = config->id;
	canaan_plane->config = config;
	canaan_plane->vo = vo;
	DRM_DEBUG_DRIVER("Create plane:%d\n", plane->base.id);

	return canaan_plane;
}
