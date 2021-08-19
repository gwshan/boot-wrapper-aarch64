/*
 * arch/aarch32/psci.S - basic PSCI implementation
 *
 * Copyright (C) 2015 ARM Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */

#include <cpu.h>
#include <linkage.h>
#include <psci.h>

#include "common.S"

	.section .vectors
	.align 6
smc_vectors:
	b	err_exception			@ Reset
	b	err_exception			@ Undef
	b	handle_smc			@ SMC
	b	err_exception			@ Prefetch abort
	b	err_exception			@ Data abort
	b	.				@ Unused
	b	err_exception			@ IRQ
	b	err_exception			@ FIQ

	.text
err_exception:
	b	.

handle_smc:
	@ Follow the SMC32 calling convention: preserve r4 - r14
	push	{r4 - r12, lr}

	blx	psci_call

	pop	{r4 - r12, lr}
	movs	pc, lr

ASM_FUNC(start_el3)
	ldr	r0, =smc_vectors
	blx	setup_vector
	/* pass through */

ASM_FUNC(start_no_el3)
	cpuid	r0, r1
	blx	find_logical_id
	b	psci_first_spin
