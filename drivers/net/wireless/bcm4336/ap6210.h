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

#ifndef __AP6210_H__
#define __AP6210_H__

#ifdef CONFIG_MACH_TINY4412
#include <linux/gpio.h>

#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/devs.h>				
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <mach/board-wlan.h>

#define	sdmmc_channel	s3c_device_hsmmc3
extern void mmc_force_presence_change_onoff(struct platform_device *pdev, int val);

#else

/* Stubs */
#define mmc_force_presence_change_onoff(pdev, val)	\
	do { } while (0)

#endif /* CONFIG_MACH_TINY4412 */

#endif /* __AP6210_H__ */
