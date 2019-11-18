/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 *  Copyright (C) 2011 Samsung, Inc.
 *  Copyright (C) 2011 Google, Inc.
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

#ifndef __BOARD_BLUETOOTH_BCM4334_H__
#define __BOARD_BLUETOOTH_BCM4334_H__

#include <linux/serial_core.h>

#if defined(CONFIG_MACH_TINY4412)
#define GPIO_BT_EN			EXYNOS4_GPX1(4)
#define GPIO_BT_EN_REV01	EXYNOS4_GPX1(0)		/* NanoPC-T1 */
#undef  GPIO_BT_WAKE
#undef  GPIO_BT_HOST_WAKE

#define GPIO_BT_RXD			EXYNOS4_GPA0(4)
#define GPIO_BT_TXD			EXYNOS4_GPA0(5)
#define GPIO_BT_CTS			EXYNOS4_GPA0(6)
#define GPIO_BT_RTS			EXYNOS4_GPA0(7)
#endif /* !CONFIG_MACH_TINY4412 */

extern void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport);

#endif /*  __BOARD_BLUETOOTH_BCM4334_H__  */
