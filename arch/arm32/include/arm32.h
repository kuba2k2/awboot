#ifndef __ARM32_H__
#define __ARM32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "board.h"
#include "main.h"

extern void v7_flush_dcache_all(void);

static inline uint32_t arm32_read_p15_c1(void) {
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r"(value) : : "memory");

	return value;
}

static inline void arm32_write_p15_c1(uint32_t value) {
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r"(value) : "memory");
	arm32_read_p15_c1();
}

static inline void arm32_interrupt_enable(void) {
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "bic %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

static inline void arm32_interrupt_disable(void) {
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "orr %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

static inline void arm32_mmu_enable(const uint32_t dram_base, uint32_t dram_size) {
	uint32_t mmu_base;

	/* use dram high 16M */
	if (dram_size > 2048)
		dram_size = 2048;

	uint32_t *mmu_base_addr = (uint32_t *)(dram_base + ((dram_size - 1) << 20));
	uint32_t *page_table	= mmu_base_addr;

	int i;
	uint32_t reg;

	/* the front 1M contain BROM/SRAM */
#ifdef CONFIG_USE_DCACHE
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
#else
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;
#endif

	/* the front 1G of memory(treated as 4G for all) is set up as none cacheable */
	for (i = 1; i < (dram_base >> 20); i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2;

	/* Set up as write back and buffered for other 3GB, rw for everyone */
	for (i = (dram_base >> 20); i < 4096; i++)
#ifdef CONFIG_USE_DCACHE
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
#else
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;
#endif
	/* flush tlb */
	asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0));
	/* Copy the page table address to cp15 */
	mmu_base = (uint32_t)mmu_base_addr;
	mmu_base |= (1 << 0) | (1 << 1) | (2 << 3);
	asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(mmu_base) : "memory");
	asm volatile("mcr p15, 0, %0, c2, c0, 1" : : "r"(mmu_base) : "memory");

	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(0x55555555)); /* modified, origin value is (~0) */

	asm volatile("isb");
#ifdef CONFIG_USE_DCACHE
	/* enable smp for dcache */
	/* SMP status is controlled by bit 6 of the CP15 Aux Ctrl Reg:ACTLR */
	asm volatile("MRC     p15, 0, r0, c1, c0, 1");
	asm volatile("ORR     r0, r0, #0x040");
	asm volatile("MCR     p15, 0, r0, c1, c0, 1");

#endif

	/* and enable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r"(reg) : : "cc");

	udelay(100);
	/* enable mmu, icache */
	reg |= ((1 << 0) | (1 << 12));
	/* disable dcache */
	reg &= ~(1 << 2);
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR" : : "r"(reg) : "cc");
	asm volatile("isb");
}

static inline void arm32_mmu_disable(void) {
	uint32_t reg;
#ifdef CONFIG_USE_DCACHE
	/* flush dcache */
	asm volatile("blx %0" : : "r"(v7_flush_dcache_all));
#endif

	/* and disable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0 @ get CR" : "=r"(reg) : : "cc");
	udelay(100);
	reg &= ~((7 << 0) | (1 << 12)); // disable mmu, icache, dcache
	asm volatile("mcr p15, 0, %0, c1, c0, 0 @ set CR" : : "r"(reg) : "cc");
	asm volatile("isb");
	/*
	 * Invalidate all instruction caches to PoU.
	 * Also flushes branch target cache.
	 */
	asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0));
	/* Invalidate entire branch predictor array */
	asm volatile("mcr p15, 0, %0, c7, c5, 6" : : "r"(0));
	/* Full system DSB - make sure that the invalidation is complete */
	asm volatile("dsb");
	/* ISB - make sure the instruction stream sees it */
	asm volatile("isb");
}

static inline void arm32_dcache_enable(void) {
	u32 reg;
	/* and enable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r"(reg) : : "cc");

	udelay(100);
	reg |= (1 << 2); // enable  dcache
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR" : : "r"(reg) : "cc");
	asm volatile("isb");
}

static inline void arm32_dcache_disable(void) {
	u32 reg;
	asm volatile("blx %0" : : "r"(v7_flush_dcache_all));
	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r"(reg) : : "cc");
	udelay(100);
	reg &= ~((1 << 2)); // disable dcache
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR" : : "r"(reg) : "cc");
	asm volatile("isb");
}

static inline void arm32_icache_enable(void) {
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value | (1 << 12));
}

static inline void arm32_icache_disable(void) {
	uint32_t value = arm32_read_p15_c1();
	arm32_write_p15_c1(value & ~(1 << 12));
}

#ifdef __cplusplus
}
#endif

#endif /* __ARM32_H__ */
