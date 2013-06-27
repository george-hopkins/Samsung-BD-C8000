ifeq ($(CONFIG_DISCONTIG_MEM),y)
zreladdr-y      := 0x60008000
params_phys-y   := 0x60000100
initrd_phys-y   := 0x60800000
endif

ifeq ($(CONFIG_SDP_SINGLE_DDR_A),y)
zreladdr-y      := 0x60008000
params_phys-y   := 0x60000100
initrd_phys-y   := 0x60800000
endif

ifeq ($(CONFIG_SDP_SINGLE_DDR_B),y)
zreladdr-y      := 0x80008000
params_phys-y   := 0x80000100
initrd_phys-y   := 0x80800000
endif
