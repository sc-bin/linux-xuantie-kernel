/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2024 Canaan Kendryte Technology Co., Ltd.
 * Author: K230 IOMUX driver
 *
 * This file defines function selectors for K230 IOMUX controller.
 *
 * For device tree pinctrl configuration, use:
 *   - pins = "ioXX" (string list of pin names)
 *   - function = "alt0", "alt1", "alt2", etc.
 *
 * For pinctrl phandle usage (e.g., in GPIO consumer properties):
 *   - <&iomux pin_number flags>
 */

#ifndef PINCTRL_K230_IOMUX_H
#define PINCTRL_K230_IOMUX_H

/*
 * K230 IOMUX Function Select (IO_SEL[2:0] bits 13:11)
 *
 * Each pin has a 32-bit register at offset pin*4 from base 0x91105000
 *
 * Bit layout (per TRM Section 12.9.2):
 *   Bit 31:   DI   - Input data (RO)
 *   Bit 13:11 IO_SEL - Function select (000=func1, 001=func2, etc.)
 *   Bit 10:   SL   - Slew rate enable
 *   Bit 9:    MSC  - Voltage control (dual-voltage pads)
 *   Bit 8:    IE   - Input enable (1=enable)
 *   Bit 7:    OE   - Output enable (1=enable)
 *   Bit 6:    PU   - Pull up (1=enable)
 *   Bit 5:    PD   - Pull down (1=enable)
 *   Bit 4:1   DS   - Drive strength select
 *   Bit 0:    ST   - Schmitt trigger (1=enable)
 */

/*
 * For device tree pinctrl configuration, use string pin names and function names:
 *
 * Example:
 *   uart0_pins: uart0-pins {
 *       pinmux {
 *           pins = "io38", "io39";
 *           function = "alt1";  // alt1 = FUNC2 = UART0_TXD/UART0_RXD
 *           bias-disable;
 *           drive-strength = <8>;
 *       };
 *   };
 *
 * Function name mapping:
 *   "alt0" = FUNC1 (GPIO/resv mode)
 *   "alt1" = FUNC2 (Primary alternate)
 *   "alt2" = FUNC3 (Secondary alternate)
 *   "alt3" = FUNC4 (Tertiary alternate)
 *   "alt4" = FUNC5 (Quaternary alternate)
 *   "alt5" = FUNC6 (Test function 1)
 *   "alt6" = FUNC7 (Test function 2)
 *   "alt7" = FUNC8 (Test function 3)
 */

/* PIN numbers */
#define K230_NUM_PINS			64

/*
 * IOMUX Function Select Definitions
 * Format: IO<NUM>_<FUNCTION_NAME>
 *
 * Function 0 (alt0/FUNC1): GPIO/rsv mode
 * Function 1 (alt1/FUNC2): Primary alternate function
 * Function 2 (alt2/FUNC3): Secondary alternate function
 * Function 3 (alt3/FUNC4): Tertiary alternate function
 * Function 4 (alt4/FUNC5): Quaternary alternate function
 * Function 5 (alt5/FUNC6): Test function 1
 * Function 6 (alt6/FUNC7): Test function 2
 * Function 7 (alt7/FUNC8): Test function 3
 */

/* IO0 */
#define K230_IO0_GPIO0			"alt0"
#define K230_IO0_BOOT0			"alt1"
#define K230_IO0_TEST_PIN0			"alt3"

/* IO1 */
#define K230_IO1_GPIO1			"alt0"
#define K230_IO1_BOOT1			"alt1"
#define K230_IO1_TEST_PIN1			"alt3"

/* IO2 */
#define K230_IO2_GPIO2			"alt0"
#define K230_IO2_JTAG_TCK			"alt1"
#define K230_IO2_PULSE_CNTR0			"alt2"
#define K230_IO2_TEST_PIN2			"alt3"

/* IO3 */
#define K230_IO3_GPIO3			"alt0"
#define K230_IO3_JTAG_TDI			"alt1"
#define K230_IO3_PULSE_CNTR1			"alt2"
#define K230_IO3_UART1_TXD			"alt3"
#define K230_IO3_TEST_PIN0			"alt4"

/* IO4 */
#define K230_IO4_GPIO4			"alt0"
#define K230_IO4_JTAG_TDO			"alt1"
#define K230_IO4_PULSE_CNTR2			"alt2"
#define K230_IO4_UART1_RXD			"alt3"
#define K230_IO4_TEST_PIN1			"alt4"

/* IO5 */
#define K230_IO5_GPIO5			"alt0"
#define K230_IO5_JTAG_TMS			"alt1"
#define K230_IO5_PULSE_CNTR3			"alt2"
#define K230_IO5_UART2_TXD			"alt3"
#define K230_IO5_TEST_PIN2			"alt4"

/* IO6 */
#define K230_IO6_GPIO6			"alt0"
#define K230_IO6_JTAG_RST			"alt1"
#define K230_IO6_PULSE_CNTR4			"alt2"
#define K230_IO6_UART2_RXD			"alt3"
#define K230_IO6_TEST_PIN3			"alt4"

/* IO7 */
#define K230_IO7_GPIO7			"alt0"
#define K230_IO7_PWM2			"alt1"
#define K230_IO7_I2C4_SCL			"alt2"
#define K230_IO7_TEST_PIN3			"alt3"
#define K230_IO7_DI0				"alt4"

/* IO8 */
#define K230_IO8_GPIO8			"alt0"
#define K230_IO8_PWM3			"alt1"
#define K230_IO8_I2C4_SDA			"alt2"
#define K230_IO8_TEST_PIN4			"alt3"
#define K230_IO8_DI1				"alt4"

/* IO9 */
#define K230_IO9_GPIO9			"alt0"
#define K230_IO9_PWM4			"alt1"
#define K230_IO9_UART1_TXD			"alt2"
#define K230_IO9_I2C1_SCL			"alt3"
#define K230_IO9_DI2				"alt4"

/* IO10 */
#define K230_IO10_GPIO10			"alt0"
#define K230_IO10_3D_CTRL_IN			"alt1"
#define K230_IO10_UART1_RXD			"alt2"
#define K230_IO10_I2C1_SDA			"alt3"
#define K230_IO10_DI3			"alt4"

/* IO11 */
#define K230_IO11_GPIO11			"alt0"
#define K230_IO11_3D_CTRL_OUT1		"alt1"
#define K230_IO11_UART2_TXD			"alt2"
#define K230_IO11_I2C2_SCL			"alt3"
#define K230_IO11_DO0			"alt4"

/* IO12 */
#define K230_IO12_GPIO12			"alt0"
#define K230_IO12_3D_CTRL_OUT2		"alt1"
#define K230_IO12_UART2_RXD			"alt2"
#define K230_IO12_I2C2_SDA			"alt3"
#define K230_IO12_DO1			"alt4"

/* IO13 */
#define K230_IO13_GPIO13			"alt0"
#define K230_IO13_M_CLK1			"alt1"
#define K230_IO13_DO2			"alt4"

/* IO14 */
#define K230_IO14_GPIO14			"alt0"
#define K230_IO14_OSPI_CS			"alt1"
#define K230_IO14_TEST_PIN5			"alt2"
#define K230_IO14_QSPI0_CS0			"alt3"
#define K230_IO14_DO3			"alt4"

/* IO15 */
#define K230_IO15_GPIO15			"alt0"
#define K230_IO15_OSPI_CLK			"alt1"
#define K230_IO15_TEST_PIN6			"alt2"
#define K230_IO15_QSPI0_CLK			"alt3"
#define K230_IO15_CO3			"alt4"

/* IO16 */
#define K230_IO16_GPIO16			"alt0"
#define K230_IO16_OSPI_D0			"alt1"
#define K230_IO16_QSPI1_CS4			"alt2"
#define K230_IO16_QSPI0_D0			"alt3"
#define K230_IO16_CO2			"alt4"

/* IO17 */
#define K230_IO17_GPIO17			"alt0"
#define K230_IO17_OSPI_D1			"alt1"
#define K230_IO17_QSPI1_CS3			"alt2"
#define K230_IO17_QSPI0_D1			"alt3"
#define K230_IO17_CO1			"alt4"

/* IO18 */
#define K230_IO18_GPIO18			"alt0"
#define K230_IO18_OSPI_D2			"alt1"
#define K230_IO18_QSPI1_CS2			"alt2"
#define K230_IO18_QSPI0_D2			"alt3"
#define K230_IO18_CO0			"alt4"

/* IO19 */
#define K230_IO19_GPIO19			"alt0"
#define K230_IO19_OSPI_D3			"alt1"
#define K230_IO19_QSPI1_CS1			"alt2"
#define K230_IO19_QSPI0_D3			"alt3"
#define K230_IO19_TEST_PIN4			"alt4"

/* IO20 */
#define K230_IO20_GPIO20			"alt0"
#define K230_IO20_OSPI_D4			"alt1"
#define K230_IO20_QSPI1_CS0			"alt2"
#define K230_IO20_PULSE_CNTR0		"alt3"
#define K230_IO20_TEST_PIN5			"alt4"

/* IO21 */
#define K230_IO21_GPIO21			"alt0"
#define K230_IO21_OSPI_D5			"alt1"
#define K230_IO21_QSPI1_CLK			"alt2"
#define K230_IO21_PULSE_CNTR1		"alt3"
#define K230_IO21_TEST_PIN6			"alt4"

/* IO22 */
#define K230_IO22_GPIO22			"alt0"
#define K230_IO22_OSPI_D6			"alt1"
#define K230_IO22_QSPI1_D0			"alt2"
#define K230_IO22_PULSE_CNTR2		"alt3"
#define K230_IO22_TEST_PIN7			"alt4"

/* IO23 */
#define K230_IO23_GPIO23			"alt0"
#define K230_IO23_OSPI_D7			"alt1"
#define K230_IO23_QSPI1_D1			"alt2"
#define K230_IO23_PULSE_CNTR3		"alt3"
#define K230_IO23_TEST_PIN8			"alt4"

/* IO24 */
#define K230_IO24_GPIO24			"alt0"
#define K230_IO24_OSPI_DQS			"alt1"
#define K230_IO24_QSPI1_D2			"alt2"
#define K230_IO24_PULSE_CNTR4		"alt3"
#define K230_IO24_TEST_PIN9			"alt4"

/* IO25 */
#define K230_IO25_GPIO25			"alt0"
#define K230_IO25_PWM5			"alt1"
#define K230_IO25_QSPI1_D3			"alt2"
#define K230_IO25_PULSE_CNTR5		"alt3"
#define K230_IO25_TEST_PIN10			"alt4"

/* IO26 */
#define K230_IO26_GPIO26			"alt0"
#define K230_IO26_MMC1_CLK			"alt1"
#define K230_IO26_TEST_PIN7			"alt2"
#define K230_IO26_PDM_CLK			"alt3"

/* IO27 */
#define K230_IO27_GPIO27			"alt0"
#define K230_IO27_MMC1_CMD			"alt1"
#define K230_IO27_PULSE_CNTR5		"alt2"
#define K230_IO27_PDM_IN0			"alt3"
#define K230_IO27_CI0			"alt4"

/* IO28 */
#define K230_IO28_GPIO28			"alt0"
#define K230_IO28_MMC1_D0			"alt1"
#define K230_IO28_UART3_TXD			"alt2"
#define K230_IO28_PDM_IN1			"alt3"
#define K230_IO28_CI1			"alt4"

/* IO29 */
#define K230_IO29_GPIO29			"alt0"
#define K230_IO29_MMC1_D1			"alt1"
#define K230_IO29_UART3_RXD			"alt2"
#define K230_IO29_3D_CTRL_IN			"alt3"
#define K230_IO29_CI2			"alt4"

/* IO30 */
#define K230_IO30_GPIO30			"alt0"
#define K230_IO30_MMC1_D2			"alt1"
#define K230_IO30_UART3_RTS			"alt2"
#define K230_IO30_3D_CTRL_OUT1		"alt3"
#define K230_IO30_CI3			"alt4"

/* IO31 */
#define K230_IO31_GPIO31			"alt0"
#define K230_IO31_MMC1_D3			"alt1"
#define K230_IO31_UART3_CTS			"alt2"
#define K230_IO31_3D_CTRL_OUT2		"alt3"
#define K230_IO31_TEST_PIN11			"alt4"

/* IO32 */
#define K230_IO32_GPIO32			"alt0"
#define K230_IO32_I2C0_SCL			"alt1"
#define K230_IO32_IIS_CLK			"alt2"
#define K230_IO32_UART3_TXD			"alt3"
#define K230_IO32_TEST_PIN12			"alt4"

/* IO33 */
#define K230_IO33_GPIO33			"alt0"
#define K230_IO33_I2C0_SDA			"alt1"
#define K230_IO33_IIS_WS			"alt2"
#define K230_IO33_UART3_RXD			"alt3"
#define K230_IO33_TEST_PIN13			"alt4"

/* IO34 */
#define K230_IO34_GPIO34			"alt0"
#define K230_IO34_I2C1_SCL			"alt1"
#define K230_IO34_IIS_D_IN0_PDM_IN3		"alt2"
#define K230_IO34_UART3_RTS			"alt3"
#define K230_IO34_TEST_PIN14			"alt4"

/* IO35 */
#define K230_IO35_GPIO35			"alt0"
#define K230_IO35_I2C1_SDA			"alt1"
#define K230_IO35_IIS_D_OUT0_PDM_IN1		"alt2"
#define K230_IO35_UART3_CTS			"alt3"
#define K230_IO35_TEST_PIN15			"alt4"

/* IO36 */
#define K230_IO36_GPIO36			"alt0"
#define K230_IO36_I2C3_SCL			"alt1"
#define K230_IO36_IIS_D_IN1_PDM_IN2		"alt2"
#define K230_IO36_UART4_TXD			"alt3"
#define K230_IO36_TEST_PIN16			"alt4"

/* IO37 */
#define K230_IO37_GPIO37			"alt0"
#define K230_IO37_I2C3_SDA			"alt1"
#define K230_IO37_IIS_D_OUT1_PDM_IN0		"alt2"
#define K230_IO37_UART4_RXD			"alt3"
#define K230_IO37_TEST_PIN17			"alt4"

/* IO38 */
#define K230_IO38_GPIO38			"alt0"
#define K230_IO38_UART0_TXD			"alt1"
#define K230_IO38_TEST_PIN8			"alt2"
#define K230_IO38_QSPI1_CS0			"alt3"
#define K230_IO38_HSYNC0			"alt4"

/* IO39 */
#define K230_IO39_GPIO39			"alt0"
#define K230_IO39_UART0_RXD			"alt1"
#define K230_IO39_TEST_PIN9			"alt2"
#define K230_IO39_QSPI1_CLK			"alt3"
#define K230_IO39_VSYNC0			"alt4"

/* IO40 */
#define K230_IO40_GPIO40			"alt0"
#define K230_IO40_UART1_TXD			"alt1"
#define K230_IO40_I2C1_SCL			"alt2"
#define K230_IO40_QSPI1_D0			"alt3"
#define K230_IO40_TEST_PIN18			"alt4"

/* IO41 */
#define K230_IO41_GPIO41			"alt0"
#define K230_IO41_UART1_RXD			"alt1"
#define K230_IO41_I2C1_SDA			"alt2"
#define K230_IO41_QSPI1_D1			"alt3"
#define K230_IO41_TEST_PIN19			"alt4"

/* IO42 */
#define K230_IO42_GPIO42			"alt0"
#define K230_IO42_UART1_RTS			"alt1"
#define K230_IO42_PWM0			"alt2"
#define K230_IO42_QSPI1_D2			"alt3"
#define K230_IO42_TEST_PIN20			"alt4"

/* IO43 */
#define K230_IO43_GPIO43			"alt0"
#define K230_IO43_UART1_CTS			"alt1"
#define K230_IO43_PWM1			"alt2"
#define K230_IO43_QSPI1_D3			"alt3"
#define K230_IO43_TEST_PIN21			"alt4"

/* IO44 */
#define K230_IO44_GPIO44			"alt0"
#define K230_IO44_UART2_TXD			"alt1"
#define K230_IO44_I2C3_SCL			"alt2"
#define K230_IO44_TEST_PIN10			"alt3"
#define K230_IO44_SPI2AXI_CLK		"alt4"

/* IO45 */
#define K230_IO45_GPIO45			"alt0"
#define K230_IO45_UART2_RXD			"alt1"
#define K230_IO45_I2C3_SDA			"alt2"
#define K230_IO45_TEST_PIN11			"alt3"
#define K230_IO45_SPI2AXI_CS			"alt4"

/* IO46 */
#define K230_IO46_GPIO46			"alt0"
#define K230_IO46_UART2_RTS			"alt1"
#define K230_IO46_PWM2			"alt2"
#define K230_IO46_I2C4_SCL			"alt3"
#define K230_IO46_TEST_PIN22			"alt4"

/* IO47 */
#define K230_IO47_GPIO47			"alt0"
#define K230_IO47_UART2_CTS			"alt1"
#define K230_IO47_PWM3			"alt2"
#define K230_IO47_I2C4_SDA			"alt3"
#define K230_IO47_TEST_PIN23			"alt4"

/* IO48 */
#define K230_IO48_GPIO48			"alt0"
#define K230_IO48_UART4_TXD			"alt1"
#define K230_IO48_TEST_PIN12			"alt2"
#define K230_IO48_I2C0_SCL			"alt3"
#define K230_IO48_SPI2AXI_DIN		"alt4"

/* IO49 */
#define K230_IO49_GPIO49			"alt0"
#define K230_IO49_UART4_RXD			"alt1"
#define K230_IO49_TEST_PIN13			"alt2"
#define K230_IO49_I2C0_SDA			"alt3"
#define K230_IO49_SPI2AXI_DOUT		"alt4"

/* IO50 */
#define K230_IO50_GPIO50			"alt0"
#define K230_IO50_UART3_TXD			"alt1"
#define K230_IO50_I2C2_SCL			"alt2"
#define K230_IO50_QSPI0_CS4			"alt3"
#define K230_IO50_TEST_PIN24			"alt4"

/* IO51 */
#define K230_IO51_GPIO51			"alt0"
#define K230_IO51_UART3_RXD			"alt1"
#define K230_IO51_I2C2_SDA			"alt2"
#define K230_IO51_QSPI0_CS3			"alt3"
#define K230_IO51_TEST_PIN25			"alt4"

/* IO52 */
#define K230_IO52_GPIO52			"alt0"
#define K230_IO52_UART3_RTS			"alt1"
#define K230_IO52_PWM4			"alt2"
#define K230_IO52_I2C3_SCL			"alt3"
#define K230_IO52_TEST_PIN26			"alt4"

/* IO53 */
#define K230_IO53_GPIO53			"alt0"
#define K230_IO53_UART3_CTS			"alt1"
#define K230_IO53_PWM5			"alt2"
#define K230_IO53_I2C3_SDA			"alt3"

/* IO54 */
#define K230_IO54_GPIO54			"alt0"
#define K230_IO54_QSPI0_CS0			"alt1"
#define K230_IO54_MMC1_CMD			"alt2"
#define K230_IO54_PWM0			"alt3"
#define K230_IO54_TEST_PIN27			"alt4"

/* IO55 */
#define K230_IO55_GPIO55			"alt0"
#define K230_IO55_QSPI0_CLK			"alt1"
#define K230_IO55_MMC1_CLK			"alt2"
#define K230_IO55_PWM1			"alt3"
#define K230_IO55_TEST_PIN28			"alt4"

/* IO56 */
#define K230_IO56_GPIO56			"alt0"
#define K230_IO56_QSPI0_D0			"alt1"
#define K230_IO56_MMC1_D0			"alt2"
#define K230_IO56_PWM2			"alt3"
#define K230_IO56_TEST_PIN29			"alt4"

/* IO57 */
#define K230_IO57_GPIO57			"alt0"
#define K230_IO57_QSPI0_D1			"alt1"
#define K230_IO57_MMC1_D1			"alt2"
#define K230_IO57_PWM3			"alt3"
#define K230_IO57_TEST_PIN30			"alt4"

/* IO58 */
#define K230_IO58_GPIO58			"alt0"
#define K230_IO58_QSPI0_D2			"alt1"
#define K230_IO58_MMC1_D2			"alt2"
#define K230_IO58_PWM4			"alt3"
#define K230_IO58_TEST_PIN31			"alt4"

/* IO59 */
#define K230_IO59_GPIO59			"alt0"
#define K230_IO59_QSPI0_D3			"alt1"
#define K230_IO59_MMC1_D3			"alt2"
#define K230_IO59_PWM5			"alt3"

/* IO60 */
#define K230_IO60_GPIO60			"alt0"
#define K230_IO60_PWM0			"alt1"
#define K230_IO60_I2C0_SCL			"alt2"
#define K230_IO60_QSPI0_CS2			"alt3"
#define K230_IO60_HSYNC1			"alt4"

/* IO61 */
#define K230_IO61_GPIO61			"alt0"
#define K230_IO61_PWM1			"alt1"
#define K230_IO61_I2C0_SDA			"alt2"
#define K230_IO61_QSPI0_CS1			"alt3"
#define K230_IO61_VSYNC1			"alt4"

/* IO62 */
#define K230_IO62_GPIO62			"alt0"
#define K230_IO62_M_CLK2			"alt1"
#define K230_IO62_UART3_DE			"alt2"
#define K230_IO62_TEST_PIN14			"alt3"

/* IO63 */
#define K230_IO63_GPIO63			"alt0"
#define K230_IO63_M_CLK3			"alt1"
#define K230_IO63_UART3_RE			"alt2"
#define K230_IO63_TEST_PIN15			"alt3"

/* IO64 ( Second IOMUX Controller ) */
#define K230_IO64_GPIO64			"alt1"
#define K230_IO64_INT0				"alt2"

/* IO65 */
#define K230_IO65_GPIO65			"alt1"
#define K230_IO65_INT1				"alt2"

/* IO66 */
#define K230_IO66_GPIO66			"alt1"
#define K230_IO66_INT2				"alt2"

/* IO67 */
#define K230_IO67_GPIO67			"alt1"
#define K230_IO67_INT3				"alt2"

/* IO68 */
#define K230_IO68_GPIO68			"alt1"
#define K230_IO68_INT4				"alt2"

/* IO69 */
#define K230_IO69_GPIO69			"alt1"
#define K230_IO69_INT5				"alt2"

/* IO70 */
#define K230_IO70_GPIO70			"alt1"
#define K230_IO70_OUT1				"alt2"

/* IO71 (Second IOMUX Controller - maps to io7) */
#define K230_IO71_GPIO71			"alt1"
#define K230_IO71_OUT2				"alt2"


/* K230_IO macros for pin name strings */
#define K230_IO0	"io0"
#define K230_IO1	"io1"
#define K230_IO2	"io2"
#define K230_IO3	"io3"
#define K230_IO4	"io4"
#define K230_IO5	"io5"
#define K230_IO6	"io6"
#define K230_IO7	"io7"
#define K230_IO8	"io8"
#define K230_IO9	"io9"
#define K230_IO10	"io10"
#define K230_IO11	"io11"
#define K230_IO12	"io12"
#define K230_IO13	"io13"
#define K230_IO14	"io14"
#define K230_IO15	"io15"
#define K230_IO16	"io16"
#define K230_IO17	"io17"
#define K230_IO18	"io18"
#define K230_IO19	"io19"
#define K230_IO20	"io20"
#define K230_IO21	"io21"
#define K230_IO22	"io22"
#define K230_IO23	"io23"
#define K230_IO24	"io24"
#define K230_IO25	"io25"
#define K230_IO26	"io26"
#define K230_IO27	"io27"
#define K230_IO28	"io28"
#define K230_IO29	"io29"
#define K230_IO30	"io30"
#define K230_IO31	"io31"
#define K230_IO32	"io32"
#define K230_IO33	"io33"
#define K230_IO34	"io34"
#define K230_IO35	"io35"
#define K230_IO36	"io36"
#define K230_IO37	"io37"
#define K230_IO38	"io38"
#define K230_IO39	"io39"
#define K230_IO40	"io40"
#define K230_IO41	"io41"
#define K230_IO42	"io42"
#define K230_IO43	"io43"
#define K230_IO44	"io44"
#define K230_IO45	"io45"
#define K230_IO46	"io46"
#define K230_IO47	"io47"
#define K230_IO48	"io48"
#define K230_IO49	"io49"
#define K230_IO50	"io50"
#define K230_IO51	"io51"
#define K230_IO52	"io52"
#define K230_IO53	"io53"
#define K230_IO54	"io54"
#define K230_IO55	"io55"
#define K230_IO56	"io56"
#define K230_IO57	"io57"
#define K230_IO58	"io58"
#define K230_IO59	"io59"
#define K230_IO60	"io60"
#define K230_IO61	"io61"
#define K230_IO62	"io62"
#define K230_IO63	"io63"
#define K230_IO64	"io0"
#define K230_IO65	"io1"
#define K230_IO66	"io2"
#define K230_IO67	"io3"
#define K230_IO68	"io4"
#define K230_IO69	"io5"
#define K230_IO70	"io6"
#define K230_IO71	"io7"

#endif /* PINCTRL_K230_IOMUX_H */
