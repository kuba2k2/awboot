/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015 Roy Spliet <rspliet@ultimaker.com>
 */

#include "types.h"

#ifndef _SUNXI_DMA_H
#define _SUNXI_DMA_H

#define SUNXI_DMA_BASE		   0x03002000
#define SUNXI_DMA_CHANNEL_BASE (SUNXI_DMA_BASE + 0x100)
#define DMA_AUTO_GATE_REG	   (SUNXI_DMA_BASE + 0x28)

#define SUNXI_DMA_CHANNEL_SIZE (0x40)
#define SUNXI_DMA_LINK_NULL	   (0xfffff800)

#define DMAC_DMATYPE_NORMAL 0
#define DMAC_CFG_TYPE_DRAM	(1)
#define DMAC_CFG_TYPE_SRAM	(0)

#define DMAC_CFG_TYPE_SPI0	   (22)
#define DMAC_CFG_TYPE_SHMC0	   (20)
#define DMAC_CFG_SRC_TYPE_NAND (5)

/* DMA base config  */
#define DMAC_CFG_CONTINUOUS_ENABLE	(0x01)
#define DMAC_CFG_CONTINUOUS_DISABLE (0x00)

/* ----------DMA dest config-------------------- */
/* DMA dest width config */
#define DMAC_CFG_DEST_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_DEST_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_DEST_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_DEST_DATA_WIDTH_64BIT (0x03)

/* DMA dest bust config */
#define DMAC_CFG_DEST_1_BURST  (0x00)
#define DMAC_CFG_DEST_4_BURST  (0x01)
#define DMAC_CFG_DEST_8_BURST  (0x02)
#define DMAC_CFG_DEST_16_BURST (0x03)

#define DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_DEST_ADDR_TYPE_IO_MODE		(0x01)

/* ----------DMA src config -------------------*/
#define DMAC_CFG_SRC_DATA_WIDTH_8BIT  (0x00)
#define DMAC_CFG_SRC_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_SRC_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_SRC_DATA_WIDTH_64BIT (0x03)

#define DMAC_CFG_SRC_1_BURST  (0x00)
#define DMAC_CFG_SRC_4_BURST  (0x01)
#define DMAC_CFG_SRC_8_BURST  (0x02)
#define DMAC_CFG_SRC_16_BURST (0x03)

#define DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_SRC_ADDR_TYPE_IO_MODE	   (0x01)

/*dma int config*/
#define DMA_PKG_HALF_INT  (1 << 0)
#define DMA_PKG_END_INT	  (1 << 1)
#define DMA_QUEUE_END_INT (1 << 2)

typedef struct {
	volatile u32 config;
	volatile u32 source_addr;
	volatile u32 dest_addr;
	volatile u32 byte_count;
	volatile u32 commit_para;
	volatile u32 link;
	volatile u32 reserved[2];
} dma_desc_t;

typedef struct {
	volatile u32 src_drq_type : 6;
	volatile u32 src_burst_length : 2;
	volatile u32 src_addr_mode : 1;
	volatile u32 src_data_width : 2;
	volatile u32 reserved0 : 5;
	volatile u32 dst_drq_type : 6;
	volatile u32 dst_burst_length : 2;
	volatile u32 dst_addr_mode : 1;
	volatile u32 dst_data_width : 2;
	volatile u32 reserved1 : 5;
} dma_channel_config_t;

typedef struct {
	dma_channel_config_t channel_cfg;
	u32 loop_mode;
	u32 data_block_size;
	u32 wait_cyc;
} dma_set_t;

typedef struct {
	void *m_data;
	// void (*m_func)(void *data);
	void (*m_func)(void);
} dma_irq_handler_t;

typedef struct {
	volatile u32 enable;
	volatile u32 pause;
	volatile u32 desc_addr;
	volatile u32 config;
	volatile u32 cur_src_addr;
	volatile u32 cur_dst_addr;
	volatile u32 left_bytes;
	volatile u32 parameters;
	volatile u32 mode;
	volatile u32 fdesc_addr;
	volatile u32 pkg_num;
	volatile u32 res[5];
} dma_channel_reg_t;

typedef struct {
	volatile u32 irq_en0; /* 0x0 dma irq enable register 0 */
	volatile u32 irq_en1; /* 0x4 dma irq enable register 1 */
	volatile u32 reserved0[2];
	volatile u32 irq_pending0; /* 0x10 dma irq pending register 0 */
	volatile u32 irq_pending1; /* 0x14 dma irq pending register 1 */
	volatile u32 reserved1[2];
	volatile u32 security; /* 0x20 dma security register */
	volatile u32 reserved3[1];
	volatile u32 auto_gate; /* 0x28 dma auto gating register */
	volatile u32 reserved4[1];
	volatile u32 status; /* 0x30 dma status register */
	volatile u32 reserved5[3];
	volatile u32 version; /* 0x40 dma Version register */
	volatile u32 reserved6[47];
	dma_channel_reg_t channel[16]; /* 0x100 dma channel register */
} dma_reg_t;

typedef struct {
	u32 used;
	u32 channel_count;
	dma_channel_reg_t *channel;
	u32 reserved;
	dma_desc_t *desc;
	dma_irq_handler_t dma_func;
} dma_source_t;

#define DMA_RST_OFS	   16
#define DMA_GATING_OFS 0

void dma_init(void);
void dma_exit(void);

u32 dma_request(u32 dmatype);
u32 dma_request_from_last(u32 dmatype);
int dma_release(u32 hdma);
int dma_setting(u32 hdma, dma_set_t *cfg);
int dma_start(u32 hdma, u32 saddr, u32 daddr, u32 bytes);
int dma_stop(u32 hdma);
int dma_querystatus(u32 hdma);

int dma_test();

#endif /* _SUNXI_DMA_H */
