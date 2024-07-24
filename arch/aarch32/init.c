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
#include <stdbool.h>

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

static void cpu_init_monitor(void)
{
	unsigned long scr = SCR_NS | SCR_HCE;
	unsigned long nsacr = NSACR_CP10 | NSACR_CP11;

	mcr(SCR, scr);

	mcr(NSACR, nsacr);
}

#ifdef PSCI
extern char psci_vectors[];

bool cpu_init_psci_arch(void)
{
	if (read_cpsr_mode() != PSR_MON)
		return false;

	mcr(MVBAR, (unsigned long)psci_vectors);
	isb();

	return true;
}
#endif

void cpu_init_arch(void)
{
	if (read_cpsr_mode() == PSR_MON)
		cpu_init_monitor();

	mcr(CNTFRQ, COUNTER_FREQ);
}
