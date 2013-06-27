#
# Makefile for the linux kernel.
#

obj-y				:= ssdtv_irq.o 

obj-$(CONFIG_ARCH_SSA1FPGA)	+= ssa1_fpga.o ssa1fpga_clock.o
obj-$(CONFIG_MACH_A1FPGA)	+= mach-ssa1fpga.o

obj-$(CONFIG_ARCH_SDP75)	+= sdp75.o sdp75_clock.o
obj-$(CONFIG_MACH_POLARIS)	+= mach-polaris.o

obj-$(CONFIG_ARCH_SDP78FPGA)	+= sdp78fpga.o sdp78fpga_clock.o
obj-$(CONFIG_MACH_SDP78FPGA)	+= mach-sdp78fgpa.o

obj-$(CONFIG_ARCH_SDP83)	+= sdp83.o sdp83_clock.o
obj-$(CONFIG_MACH_LEONID)	+= mach-leonid.o
obj-$(CONFIG_MACH_LEONID64512)	+= mach-leonid64512.o
obj-$(CONFIG_MACH_LEONID64256)	+= mach-leonid64256.o
obj-$(CONFIG_MACH_LEONID48320)	+= mach-leonid48320.o
obj-$(CONFIG_MACH_LEONID48256)	+= mach-leonid48256.o
obj-$(CONFIG_MACH_LEONID48EU)	+= mach-leonid48eu.o
obj-$(CONFIG_MACH_LEONID48US)	+= mach-leonid48us.o
obj-$(CONFIG_MACH_LEONID32US)	+= mach-leonid32us.o
obj-$(CONFIG_SDP83_GPIO)	+= sdp83_gpio.o

obj-$(CONFIG_PCI)		+= pci.o ssdtv_pci.o 
obj-$(CONFIG_SSDTV_TIMER)	+= time.o
obj-$(CONFIG_HIGH_RES_TIMERS)	+= ssdtv_hrtime.o
