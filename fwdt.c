/*
 * FWDT driver
 *
 * Copyright(C) 2012 Canonical Ltd.
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
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_AUTHOR("Alex Hung");
MODULE_DESCRIPTION("FWDT EFI Driver");
MODULE_LICENSE("GPL");

static int __init fwdt_init(void)
{
	pr_info("initializing fwdt module\n");

	return 0;
}

static void __exit fwdt_exit(void)
{
	pr_info("exiting fwdt module\n");
}

module_init(fwdt_init);
module_exit(fwdt_exit);
