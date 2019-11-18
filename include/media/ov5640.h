/*
 * Driver for OV5640 CMOS Image Sensor
 *
 * Copyright (C) 2015 FriendlyARM (www.arm9.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MEDIA_OV5640_H__
#define __MEDIA_OV5640_H__

#include <linux/device.h>
#include <media/v4l2-subdev.h>
#include <media/media-entity.h>

#define OV5640_DEVICE_NAME		"ov5640"

/* Default config */
#define DEFAULT_FMT				V4L2_PIX_FMT_UYVY	/* YUV422 */
#define DEFAULT_PREVIEW_WIDTH	640
#define DEFAULT_PREVIEW_HEIGHT	480

/* Number of pixels and number of lines per frame for different standards */
#define VGA_NUM_ACTIVE_PIXELS	640
#define VGA_NUM_ACTIVE_LINES	480

struct ov5640_platform_data {
	u32 default_width;
	u32 default_height;
	u32 pixelformat;
	u32 freq;		/* MCLK in KHz */

	/* This SoC supports Parallel & CSI-2 */
	u32 is_mipi;	/* set to 1 if mipi */

	int (*s_power)(struct v4l2_subdev *subdev, int on);
	int (*set_xclk)(struct v4l2_subdev *subdev, int hz);
	int (*configure_interface)(struct v4l2_subdev *subdev, u32 pixclk);
};

#define OV5640_CLK_MAX			(54000000)	/* 54MHz */
#define OV5640_CLK_MIN			( 6000000)	/*  6Mhz */

#endif /* __MEDIA_OV5640_H__ */

