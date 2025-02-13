ARCH := arch/arm32
SUNXI:=$(ARCH)/sunxi

CPU:=$(SUNXI)/$(SOC)

CFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access
CFLAGS += -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += -I $(ARCH)/include -I $(CPU) -I $(SUNXI) -I ./

ASRCS	+=  $(SUNXI)/start.S

SRCS	+=  $(ARCH)/arch_timer.c
SRCS	+=  $(ARCH)/irq.c
ASRCS	+=  $(ARCH)/cache.S

SRCS	+=  $(SUNXI)/sunxi_usart.c
SRCS	+=  $(SUNXI)/sunxi_dma.c
SRCS	+=  $(SUNXI)/sunxi_gpio.c
SRCS	+=  $(SUNXI)/sunxi_spi.c
SRCS	+=  $(SUNXI)/sunxi_sdhci.c
SRCS	+=  $(SUNXI)/sdmmc.c

SRCS	+=  $(CPU)/dram.c
SRCS	+=  $(CPU)/sunxi_clk.c
