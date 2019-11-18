/*
 *  Copyright (C) 2015 FriendlyARM (www.arm9.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __BOARD_WLAN_H__
#define __BOARD_WLAN_H__
#include <mach/board-revision.h>

#define GPIO_WLAN_EN		EXYNOS4_GPK3(2)
#define GPIO_WLAN_EN_AF    	1

/* EINT for WLAN to wake-up HOST */
#define GPIO_WLAN_HOST_WAKE			EXYNOS4_GPX1(5)
#define GPIO_WLAN_HOST_WAKE_REV01	EXYNOS4_GPX1(1)
#define GPIO_WLAN_HOST_WAKE_AF	0xF

static inline int brcm_gpio_host_wake(void)
{
	if (is_board_rev_B())
		return GPIO_WLAN_HOST_WAKE_REV01;
	else
		return GPIO_WLAN_HOST_WAKE;
}

#define GPIO_WLAN_SDIO_CLK	EXYNOS4_GPK3(0)
#define GPIO_WLAN_SDIO_CLK_AF	2
#define GPIO_WLAN_SDIO_CMD	EXYNOS4_GPK3(1)
#define GPIO_WLAN_SDIO_CMD_AF	2
#define GPIO_WLAN_SDIO_D0	EXYNOS4_GPK3(3)
#define GPIO_WLAN_SDIO_D0_AF	2
#define GPIO_WLAN_SDIO_D1	EXYNOS4_GPK3(4)
#define GPIO_WLAN_SDIO_D1_AF	2
#define GPIO_WLAN_SDIO_D2	EXYNOS4_GPK3(5)
#define GPIO_WLAN_SDIO_D2_AF	2
#define GPIO_WLAN_SDIO_D3	EXYNOS4_GPK3(6)
#define GPIO_WLAN_SDIO_D3_AF	2

extern int brcm_wlan_init(void);

#endif /*  __BOARD_WLAN_H__  */
