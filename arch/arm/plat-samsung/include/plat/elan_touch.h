/*
 * include/plat/elan_touch.h
 *
 * Copyright (C) 2012 FriendlyARM (www.arm9.net)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __PLAT_ELAN_TOUCH_H__
#define __PLAT_ELAN_TOUCH_H__


struct elan_i2c_platform_data {
	int intr_gpio;
	int rst_gpio;
	int irq_num;
	uint16_t screen_coordinate_x;
	uint16_t screen_coordinate_y;
};

#endif	// __PLAT_FT5X0X_TOUCH_H__

