/*
 * arch/aarch64/boot.S - simple register setup code for stand-alone Linux booting
 *
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */
#include <linkage.h>

#include "common.S"

	.section .init

	/*
	 * The boot-wrapper must be entered from the reset vector at the
	 * highest implemented exception level. The boot-wrapper only supports
	 * being entered at the following exception levels:
	 *
	 * - EL3 (Secure)
	 *   Entering at EL3 is strongly recommended.
	 *   EL2 must be implemented.
	 *
	 * - EL2 (Non-secure)
	 *   Entering at EL2 is partially supported.
	 *   PSCI is not supported when entered in this exception level.
	 */
ASM_FUNC(_start)
	mrs	x0, CurrentEL
	cmp	x0, #CURRENTEL_EL3
	b.eq	reset_at_el3
	cmp	x0, #CURRENTEL_EL2
	b.eq	reset_at_el2
	cmp	x0, #CURRENTEL_EL1
	b.eq	reset_at_el1

	/* Booting at EL0 is not supported */
	b	.

	/*
	 * EL3 initialisation
	 */
reset_at_el3:
	mov_64	x0, SCTLR_EL3_RESET
	msr	sctlr_el3, x0
	isb

	mov	x0, #0x30			// RES1
	orr	x0, x0, #(1 << 0)		// Non-secure EL1
	orr	x0, x0, #(1 << 8)		// HVC enable

	/* Enable pointer authentication if present */
	mrs	x1, id_aa64isar1_el1
	/* We check for APA+API and GPA+GPI */
	ldr	x2, =((0xff << 24) | (0xff << 4))
	and	x1, x1, x2
	cbz	x1, 1f

	orr	x0, x0, #(1 << 16)		// AP key enable
	orr	x0, x0, #(1 << 17)		// AP insn enable
1:
	/* Enable TME if present */
	mrs	x1, id_aa64isar0_el1
	ubfx	x1, x1, #24, #4
	cbz	x1, 1f

	orr	x0, x0, #(1 << 34)		// TME enable
1:
	/* Enable FGT if present */
	mrs	x1, id_aa64mmfr0_el1
	ubfx	x1, x1, #56, #4
	cbz	x1, 1f

	orr	x0, x0, #(1 << 27)		// FGT enable
1:
	/* Enable ECV2 if present (allows CNTPOFF_EL2) */
	mrs	x1, id_aa64mmfr0_el1
	ubfx	x1, x1, #60, #4
	cmp	x1, #2
	b.lt	1f

	orr	x0, x0, #(1 << 28)		// ECV enable
1:
	/* Enable MTE if present */
	mrs	x10, id_aa64pfr1_el1
	ubfx	x10, x10, #8, #4
	cmp	x10, #2
	b.lt	1f

	orr	x0, x0, #(1 << 26)		// ATA enable
1:
#ifndef KERNEL_32
	orr	x0, x0, #(1 << 10)		// 64-bit EL2
#endif
	msr	scr_el3, x0

	msr	cptr_el3, xzr			// Disable copro. traps to EL3

	mov	x0, xzr
	mrs	x1, id_aa64dfr0_el1
	ubfx	x1, x1, #32, #4
	cbz	x1, 1f

	// Enable SPE for the non-secure world.
	orr	x0, x0, #(0x3 << 12)

	// Do not trap PMSNEVFR_EL1 if present
	cmp	x1, #3
	b.lt	1f
	orr	x0, x0, #(1 << 36)

1:	mrs	x1, id_aa64dfr0_el1
	ubfx	x1, x1, #44, #4
	cbz	x1, 1f

	// Enable TRBE for the non-secure world.
	ldr	x1, =(0x3 << 24)
	orr	x0, x0, x1

1:	mrs     x1, id_aa64dfr0_el1
	ubfx    x1, x1, #52, #4
	cbz     x1, 1f

	// Enable BRBE for the non-secure world.
	ldr     x1, =(0x3 << 32)
	orr     x0, x0, x1

1:	msr	mdcr_el3, x0			// Disable traps to EL3

	mrs	x0, id_aa64pfr0_el1
	ubfx	x0, x0, #32, #4			// SVE present?
	cbz	x0, 1f				// Skip SVE init if not

	mrs	x0, cptr_el3
	orr	x0, x0, #CPTR_EL3_EZ		// enable SVE
	msr	cptr_el3, x0
	isb

	mov	x0, #ZCR_EL3_LEN_MAX		// SVE: Enable full vector len
	msr	ZCR_EL3, x0			// for EL2.

1:
	ldr	x0, =COUNTER_FREQ
	msr	cntfrq_el0, x0

	cpuid	x0, x1
	bl	find_logical_id
	cmp	x0, #MPIDR_INVALID
	b.eq	err_invalid_id
	bl	setup_stack

	bl	gic_secure_init

	b	start_el3

	/*
	 * EL2 initialization
	 */
reset_at_el2:
	// Ensure E2H is not in use
	mov_64	x0, HCR_EL2_RESET
	msr	hcr_el2, x0
	isb

	mov_64	x0, SCTLR_EL2_RESET
	msr	sctlr_el2, x0
	isb

	b	reset_no_el3

	/*
	 * EL1 initialization
	 */
reset_at_el1:
	mov_64	x0, SCTLR_EL1_RESET
	msr	sctlr_el1, x0
	isb

	b	reset_no_el3

reset_no_el3:
	cpuid	x0, x1
	bl	find_logical_id
	cmp	x0, #MPIDR_INVALID
	b.eq	err_invalid_id
	bl	setup_stack

	mov	w0, #1
	ldr	x1, =flag_no_el3
	str	w0, [x1]

	b	start_no_el3

err_invalid_id:
	b	.

	/*
	 * Drop to the kernel
	 * x0:		entry address
	 * x1-x4:	arguments
	 */
ASM_FUNC(jump_kernel)
	mov	x19, x0
	mov	x20, x1
	mov	x21, x2
	mov	x22, x3
	mov	x23, x4

	ldr	x0, =SCTLR_EL1_KERNEL
	msr	sctlr_el1, x0

	ldr	x0, =SCTLR_EL2_KERNEL
	msr	sctlr_el2, x0

	cpuid	x0, x1
	bl	find_logical_id
	bl	setup_stack		// Reset stack pointer

	ldr	w0, flag_no_el3
	cmp	w0, #0			// Prepare Z flag

	mov	x0, x20
	mov	x1, x21
	mov	x2, x22
	mov	x3, x23

	b.eq	1f
	br	x19			// No EL3

1:	mov	x4, #SPSR_KERNEL

	/*
	 * If bit 0 of the kernel address is set, we're entering in AArch32
	 * thumb mode. Set SPSR.T accordingly.
	 */
	bfi	x4, x19, #5, #1

	msr	elr_el3, x19
	msr	spsr_el3, x4
	eret

	.ltorg

	.data
	.align 3
flag_no_el3:
	.long 0
