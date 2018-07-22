MSYS_BASE := c:/MinGW/msys/1.0
MSYS_INCDIR := include include/arpa include/asm include/ include/cygwin include/machine include/net include/netinet include/sys include/w32api
MSYS_INCDIR	:= $(addprefix -I$(MSYS_BASE)/,$(MSYS_INCDIR))

XTENSA_TOOLS_ROOT ?= c:/Espressif/xtensa-lx106-elf/bin
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc


GZIP_COMPRESSION ?= no
USE_HEATSHRINK ?= yes

CFLAGS=-I../../lib/heatshrink -I../../include -I..  -std=gnu99 -nostdlib
ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS		+= -DESPFS_GZIP
endif

ifeq ("$(USE_HEATSHRINK)","yes")
CFLAGS		+= -DESPFS_HEATSHRINK
endif

OBJS=main.o heatshrink_encoder.o
TARGET=mkespfsimage

$(TARGET): $(OBJS)
ifeq ("$(GZIP_COMPRESSION)","yes")
	$(CC) -o $@ $^ -lz
else
	$(CC) -o $@ $^
endif

clean:
	rm -f $(TARGET) $(OBJS)