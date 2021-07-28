/*
 * init.c - common boot-wrapper initialization
 *
 * Copyright (C) 2021 ARM Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */
#include <cpu.h>
#include <platform.h>

static void announce_bootwrapper(void)
{
	print_string("Boot-wrapper v0.2\r\n");
}

void announce_arch(void);

void cpu_init_bootwrapper(void)
{
	if (this_cpu_logical_id() == 0) {
		init_uart();
		announce_bootwrapper();
		announce_arch();
		print_string("\r\n");
		init_platform();
	}
}
