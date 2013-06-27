#SELP.arm.3.x support A1 2007-10-22
# CONFIG_MACH_A1FPGA
ifeq ($(CONFIG_MACH_A1FPGA),y)
zreladdr-y      := 0x70008000
params_phys-y   := 0x70000100
initrd_phys-y   := 0x70800000
endif
# CONFIG_MACH_A1FPGA END

# CONFIG_MACH_POLARIS
ifeq ($(CONFIG_MACH_POLARIS),y)
# CONFIG_SYS_MEM_WEST
ifeq ($(CONFIG_SYS_MEM_WEST),y)
zreladdr-y      := 0x60008000
params_phys-y   := 0x60000100
initrd_phys-y   := 0x60800000
endif
# CONFIG_SYS_MEM_WEST END

# CONFIG_SYS_MEM_EAST
ifeq ($(CONFIG_SYS_MEM_EAST),y)
zreladdr-y      := 0xD0008000
params_phys-y   := 0xD0000100
initrd_phys-y   := 0xD0800000
endif
# CONFIG_SYS_MEM_EAST END
endif
# CONFIG_MACH_POLARIS END

# CONFIG_MACH_SDP78FPGA
ifeq ($(CONFIG_MACH_SDP78FPGA),y)
zreladdr-y	:= 0x60008000
params_phys-y	:= 0x60000100
initrd_phys-y	:= 0x60800000
endif
# CONFIG_MACH_SDP78FPGA END

# CONFIG_MACH_LEONID_XXX
ifeq ($(CONFIG_MACH_LEONID_XXX),y)
zreladdr-y      := 0x60008000
params_phys-y   := 0x60000100
initrd_phys-y   := 0x60800000
endif
# CONFIG_MACH_LEONID END

