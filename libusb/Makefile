##############################################
#Makefile for PTP Driver Lib
##############################################
.SUFFIXES = .c .o 

SHARED_LIB = y
COMPILE_OPT = y

#OPENSRC_DEF_CHIP=$(PTP_DEF_CHIP)
OPENSRC_DEF_CHIP=VALENCIA
ifeq ($(origin OPENSRC_DEF_CHIP), undefined)
OPENSRC_DEF_CHIP = INTEL
endif
##########################################################
##########################################################
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
INC =  ./Inc
LIBPATH = ../../Lib/$(OPENSRC_DEF_CHIP)

ifeq ($(OPENSRC_DEF_CHIP), VALENCIA)
COMPILE_OPT = n
endif
ifeq ($(OPENSRC_DEF_CHIP), Trident)
COMPILE_OPT = n
endif
ifeq ($(OPENSRC_DEF_CHIP), X4)
COMPILE_OPT = n
 	ADD_COMPILE_OPT += -EL -msoft-float -march=mips32r2
 	ADD_LINK_OPT += -EL -msoft-float -march=mips32r2
endif
ifeq ($(OPENSRC_DEF_CHIP), INTEL)
COMPILE_OPT = y
endif
##########################################################
##########################################################
ifeq ($(COMPILE_OPT), n)
CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wshadow \
		  -Wcast-qual \
		  -fno-strict-aliasing -fsigned-char -O2\
		  -ffunction-sections -fdata-sections\
		  -Werror $(ADD_COMPILE_OPT)\
		  -I $(INC)
endif
ifeq ($(COMPILE_OPT), y)
CFLAGS = -c -O2 -Wall -fPIC -I $(INC)
endif

ifeq ($(OPENSRC_DEF_CHIP), Trident)
	CFLAGS += -march=mips1 -mtune=mips1
endif
##########################################################
##########################################################
OBJS = descriptors.o error.o linux.o usb.o
SRCS = descriptors.c error.c linux.c usb.c

ifeq ($(SHARED_LIB), y)
TARGET = libusb.so
$(TARGET) : $(OBJS)
ifeq ($(OPENSRC_DEF_CHIP), X4)
	$(CC) -shared -Wl $(ADD_LINK_OPT),-soname,$@ -o $@ $(OBJS)
else
	$(CC) -shared -Wl,-soname,$@ -o $@ $(OBJS)
endif
else
TARGET = libusb.a
$(TARGET) : $(OBJS)
	$(AR) rcv $@ $(OBJS)
	$(RANLIB) $@
endif

##########################################################
##########################################################
all : 	$(TARGET)

dep :
	gccmakedep $(INC) $(SRCS)
clean :
	rm -rf $(OBJS) $(TARGET) core
