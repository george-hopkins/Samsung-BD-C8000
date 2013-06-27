#include <linux/module.h>
#include <asm/arch/sdp_gpio_board.h>

static struct sdp_gpio_port sdp92_gpio_ports[] = {
	{
		.portno = 0,
		.npins = 8,
		.reg_offset = 0x50,
		.pins = {
			[0] = {.reg_offset = 0x04, .reg_shift = 10},
			[1] = {.reg_offset = 0x04, .reg_shift = 12},
			[2] = {.reg_offset = 0x04, .reg_shift = 14},
			[3] = {.reg_offset = 0x04, .reg_shift = 16},
			[4] = {.reg_offset = 0x04, .reg_shift = 18},
			[5] = {.reg_offset = 0x04, .reg_shift = 20},
			[6] = {.reg_offset = 0x04, .reg_shift = 22},
			[7] = {.reg_offset = 0x04, .reg_shift = 24},
		},
	},
	{
		.portno = 1,
		.npins = 8,
		.reg_offset = 0x5c,
		.pins = {
			[0] = {.reg_offset = 0x04, .reg_shift = 26},
			[1] = {.reg_offset = 0x04, .reg_shift = 28},
			[2] = {.reg_offset = 0x04, .reg_shift = 30},
			[3] = {.reg_offset = 0x08, .reg_shift = 0},
			[4] = {.reg_offset = 0x08, .reg_shift = 2},
			[5] = {.reg_offset = 0x1c, .reg_shift = 4},
			[6] = {.reg_offset = 0x1c, .reg_shift = 6},
			[7] = {.reg_offset = 0x1c, .reg_shift = 8},
		},
	},
	{
		.portno = 2,
		.npins = 1,
		.reg_offset = 0x64,
		.pins = {
			[0] = {.reg_offset = 0x1c, .reg_shift = 10},
		},
	},
	{
		.portno = 4,
		.npins = 8,
		.reg_offset = 0x80,
		.pins = {
			[0] = {.reg_offset = 0x00, .reg_shift = 24},
			[1] = {.reg_offset = 0x00, .reg_shift = 26},
			[2] = {.reg_offset = 0x00, .reg_shift = 28},
			[3] = {.reg_offset = 0x00, .reg_shift = 30},
			[4] = {.reg_offset = 0x04, .reg_shift = 0},
			[5] = {.reg_offset = 0x04, .reg_shift = 2},
			[6] = {.reg_offset = 0x30, .reg_shift = 20},
			[7] = {.reg_offset = 0x30, .reg_shift = 22},
		},
	},
	{
		.portno = 5,
		.npins = 8,
		.reg_offset = 0x8c,
		.pins = {
			[0] = {.reg_offset = 0x00, .reg_shift = 20},
			[1] = {.reg_offset = 0x00, .reg_shift = 22},
			[2] = {.reg_offset = 0x30, .reg_shift = 24},
			[3] = {.reg_offset = 0x30, .reg_shift = 26},
			[4] = {.reg_offset = 0x30, .reg_shift = 28},
			[5] = {.reg_offset = 0x30, .reg_shift = 30},
			[6] = {.reg_offset = 0x04, .reg_shift = 6},
			[7] = {.reg_offset = 0x04, .reg_shift = 8},
		},
	},
};

void __init sdp92_gpio_init(void)
{
	sdp_gpio_add_device (PA_PADCTRL_BASE, 4096, sdp92_gpio_ports, ARRAY_SIZE(sdp92_gpio_ports));
}
