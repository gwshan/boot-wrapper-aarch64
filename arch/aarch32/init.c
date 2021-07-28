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

static const char *mode_string(void)
{
	switch (read_cpsr_mode()) {
	case PSR_MON:
		return "PL1";
	case PSR_HYP:
		return "PL2 (Non-secure)";
	default:
		return "<UNKNOWN MODE>";
	}
}

void announce_arch(void)
{
	print_string("Entered at ");
	print_string(mode_string());
	print_string("\r\n");
}
