
BUILD_DIR=build

MKSUNXI = mksunxi

CSRC    = mksunxi.c
CXXSRC  =

COBJS   = $(addprefix $(BUILD_DIR)/,$(CSRC:.c=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/,$(CXXSRC:.cpp=.opp))

INCLUDES = -I includes -I ../lib
CFLAGS   = -O2 -std=gnu99 $(INCLUDES)
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

HOSTCC ?= gcc
HOSTCXX ?= g++

all: tools
tools: $(MKSUNXI)

.PHONY: all clean
.SILENT:

clean:
	rm -rf build
	rm -f $(MKSUNXI)

$(BUILD_DIR)/%.o : %.c
	echo "HOSTCC  $@"
	mkdir -p $(@D)
	$(HOSTCC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.opp : %.cpp
	echo "HOSTCXX $@"
	mkdir -p $(@D)
	$(HOSTCXX) $(CXXFLAGS) -c $< -o $@

$(MKSUNXI): $(COBJS)
	echo "HOSTLD  $@"
	$(HOSTCC) $(CFLAGS) $(COBJS) -o $(MKSUNXI)
