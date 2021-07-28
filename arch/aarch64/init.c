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

void announce_arch(void)
{
	unsigned char el = mrs(CurrentEl) >> 2;

	print_string("Entered at EL");
	print_char('0' + el);
	print_string("\r\n");
}
