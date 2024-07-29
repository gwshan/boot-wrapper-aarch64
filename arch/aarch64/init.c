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

void announce_arch(void)
{
	unsigned char el = mrs(CurrentEl) >> 2;

	print_string("Entered at EL");
	print_char('0' + el);
	print_string("\r\n");
}

static inline bool kernel_is_32bit(void)
{
#ifdef KERNEL_32
	return true;
#else
	return false;
#endif
}

static inline bool cpu_has_pauth(void)
{
	const unsigned long isar1_pauth = ID_AA64ISAR1_EL1_APA |
					  ID_AA64ISAR1_EL1_API |
					  ID_AA64ISAR1_EL1_GPA |
					  ID_AA64ISAR1_EL1_GPI;

	const unsigned long isar2_pauth = ID_AA64ISAR2_EL1_APA3 |
					  ID_AA64ISAR2_EL1_GPA3;

	return (mrs(ID_AA64ISAR1_EL1) & isar1_pauth) ||
	       (mrs(ID_AA64ISAR2_EL1) & isar2_pauth);
}

static inline bool cpu_has_permission_indirection(void)
{
	const unsigned long mask = ID_AA64MMFR3_EL1_S1PIE |
				   ID_AA64MMFR3_EL1_S2PIE |
				   ID_AA64MMFR3_EL1_S1POE |
				   ID_AA64MMFR3_EL1_S2POE;

	return mrs(ID_AA64MMFR3_EL1) & mask;
}

void cpu_init_el3(void)
{
	unsigned long scr = SCR_EL3_RES1 | SCR_EL3_NS | SCR_EL3_HCE;
	unsigned long mdcr = 0;
	unsigned long cptr = 0;
	unsigned long smcr = 0;

	if (cpu_has_pauth())
		scr |= SCR_EL3_APK | SCR_EL3_API;

	if (mrs_field(ID_AA64ISAR0_EL1, TME))
		scr |= SCR_EL3_TME;

	if (mrs_field(ID_AA64MMFR0_EL1, FGT))
		scr |= SCR_EL3_FGTEN;

	if (mrs_field(ID_AA64MMFR0_EL1, ECV) >= 2)
		scr |= SCR_EL3_ECVEN;

	if (mrs_field(ID_AA64MMFR1_EL1, HCX))
		scr |= SCR_EL3_HXEn;

	if (mrs_field(ID_AA64MMFR3_EL1, TCRX)) {
		scr |= SCR_EL3_TCR2EN;
		msr(TCR2_EL2, 0);
		msr(TCR2_EL1, 0);
	}

	if (cpu_has_permission_indirection())
		scr |= SCR_EL3_PIEN;

	if (mrs_field(ID_AA64PFR1_EL1, MTE) >= 2)
		scr |= SCR_EL3_ATA;

	if (!kernel_is_32bit())
		scr |= SCR_EL3_RW;

	if (mrs_field(ID_AA64MMFR3_EL1, SCTLRX)) {
		scr |= SCR_EL3_SCTLR2En;
		msr(SCTLR2_EL2, 0);
		msr(SCTLR2_EL1, 0);
	}

	msr(SCR_EL3, scr);

	msr(CPTR_EL3, cptr);

	if (mrs_field(ID_AA64DFR0_EL1, PMSVER))
		mdcr |= MDCR_EL3_NSPB_NS_NOTRAP;

	if (mrs_field(ID_AA64DFR0_EL1, PMSVER) >= 3)
		mdcr |= MDCR_EL3_ENPMSN;

	if (mrs_field(ID_AA64DFR0_EL1, TRACEBUFFER))
		mdcr |= MDCR_EL3_NSTB_NS_NOTRAP;

	if (mrs_field(ID_AA64DFR0_EL1, BRBE))
		mdcr |= MDCR_EL3_SBRBE_NOTRAP_NOPROHIBIT;

	if (mrs_field(ID_AA64DFR0_EL1, DEBUGVER) >= 11)
		mdcr |= MDCR_EL3_EBWE;

	msr(MDCR_EL3, mdcr);

	if (mrs_field(ID_AA64PFR0_EL1, SVE)) {
		cptr |= CPTR_EL3_EZ;
		msr(CPTR_EL3, cptr);
		isb();
		/*
		 * Write the maximum possible vector length, hardware
		 * will constrain to the actual limit.
		 */
		msr(ZCR_EL3, ZCR_EL3_LEN_MAX);
	}

	if (mrs_field(ID_AA64PFR1_EL1, SME)) {
		cptr |= CPTR_EL3_ESM;
		msr(CPTR_EL3, cptr);
		isb();

		scr |= SCR_EL3_EnTP2;
		msr(SCR_EL3, scr);
		isb();

		/*
		 * Write the maximum possible vector length, hardware
		 * will constrain to the actual limit.
		 */
		smcr = SMCR_EL3_LEN_MAX;

		if (mrs_field(ID_AA64SMFR0_EL1, FA64))
			smcr |= SMCR_EL3_FA64;

		msr(SMCR_EL3, smcr);
	}

	msr(CNTFRQ_EL0, COUNTER_FREQ);
}

#ifdef PSCI
extern char psci_vectors[];

bool cpu_init_psci_arch(void)
{
	if (mrs(CurrentEL) != CURRENTEL_EL3)
		return false;

	msr(VBAR_EL3, (unsigned long)psci_vectors);
	isb();

	return true;
}
#endif
