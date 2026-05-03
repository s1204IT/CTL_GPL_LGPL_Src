// SPDX-License-Identifier: (GPL-2.0+ or MIT)

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun50iw15_r_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
	/* BANK L */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),		/* s_twi0_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),		/* s_twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_tx */
		SUNXI_FUNCTION(0x4, "s_pwm2"),		/* s_pwm2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_rx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x4, "s_pwm3"),		/* s_pwm3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_ms */
		SUNXI_FUNCTION(0x4, "s_pwm4"),		/* s_pwm5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_ck */
		SUNXI_FUNCTION(0x4, "s_pwm5"),		/* s_pwm5 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_mclk */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_do */
		SUNXI_FUNCTION(0x3, "s_pwm6"),		/* s_pwm6 */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* i2s0_din1 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_dout0 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_di */
		SUNXI_FUNCTION(0x3, "s_pwm7"),		/* s_pwm7 */
		SUNXI_FUNCTION(0x4, "i2s0"),		/* i2s0_dout1 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_din0 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sck */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_ms */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ms */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_bclk */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sda */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_ck */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_lrck */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm0"),		/* s_pwm0 */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_do */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_mclk */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_cs */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_cir"),		/* s_cir_in */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_di */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x5, "s_pwm1"),		/* s_pwm1 */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi2"),		/* s_twi2_sck */
		SUNXI_FUNCTION(0x3, "s_pwm8"),		/* s_pwm8 */
		SUNXI_FUNCTION(0x4, "s_uart0"),		/* r_uart0_tx */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi2"),		/* s_twi2_sda */
		SUNXI_FUNCTION(0x3, "s_pwm9"),		/* s_pwm9 */
		SUNXI_FUNCTION(0x4, "s_uart0"),		/* r_uart0_rx */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_miso */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	/* BANK M */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x4, "s_pwm2"),		/* s_pwm */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x4, "s_pwm3"),		/* s_pwm3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sck */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* s_jtag_ms */
		SUNXI_FUNCTION(0x4, "s_pwm6"),		/* s_pwm6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi_sda */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION(0x4, "s_pwm7"),		/* s_pwm7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm8"),		/* s_pwm8 */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION(0x4, "s_twi2"),		/* s_twi2_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_cir"),		/* s_cir_in */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x4, "s_twi2"),		/* s_twi2_sda */
		SUNXI_FUNCTION(0x5, "s_pwm9"),		/* s_pwm9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
#else
#endif
};

static const unsigned int sun50iw15_r_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static const struct sunxi_pinctrl_desc sun50iw15_r_pinctrl_data = {
	.pins = sun50iw15_r_pins,
	.npins = ARRAY_SIZE(sun50iw15_r_pins),
	.irq_banks = ARRAY_SIZE(sun50iw15_r_irq_bank_map),
	.irq_bank_map = sun50iw15_r_irq_bank_map,
	.pin_base = SUNXI_PIN_BASE('L'),
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

static int sun50iw15_r_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_bsp_pinctrl_init(pdev, &sun50iw15_r_pinctrl_data);
}

static struct of_device_id sun50iw15_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw15-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw15_r_pinctrl_match);

static struct platform_driver sun50iw15_r_pinctrl_driver = {
	.probe	= sun50iw15_r_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw15-r-pinctrl",
		.of_match_table	= sun50iw15_r_pinctrl_match,
	},
};

static int __init sun50iw15_r_pio_init(void)
{
	return platform_driver_register(&sun50iw15_r_pinctrl_driver);
}
postcore_initcall(sun50iw15_r_pio_init);

MODULE_DESCRIPTION("Allwinner sun50iw15 R_PIO pinctrl driver");
MODULE_AUTHOR("<xuefan@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
