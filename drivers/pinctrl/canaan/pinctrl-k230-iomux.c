// SPDX-License-Identifier: GPL-2.0-only
/*
 * K230 SoC IOMUX driver using pinctrl-single
 *
 * Copyright (C) 2024 Canaan Kendryte Technology Co., Ltd.
 *
 * Based on K230 Technical Reference Manual V0.3.1
 * Section 12.9.2: IO MUX and PAD control register
 * Base Address: 0x91105000
 * Each pin has a 32-bit register at offset pin*4
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include <dt-bindings/pinctrl/canaan,k230-iomux.h>

/*
 * K230 IOMUX Register Layout (per pin)
 * Base: 0x91105000, Offset: pin * 4
 *
 * The IOMUX controller supports 64 pins
 *
 * Each pin has a 32-bit register at offset pin*4
 *
 * Bit layout (per TRM Section 12.9.2):
 *   Bit 31   DI   - Input data (RO)
 *   Bit 13:11 IO_SEL - Function select (000=func1, 001=func2, etc.)
 *   Bit 10   SL   - Slew rate enable
 *   Bit 9    MSC  - Voltage control (dual-voltage pads)
 *   Bit 8    IE   - Input enable (1=enable)
 *   Bit 7    OE   - Output enable (1=enable)
 *   Bit 6    PU   - Pull up (1=enable)
 *   Bit 5    PD   - Pull down (1=enable)
 *   Bit 4:1  DS   - Drive strength select
 *   Bit 0    ST   - Schmitt trigger (1=enable)
 */

/* Function names - generic names for function selectors */
static const char * const k230_function_names[] = {
	"alt0",		/* Function 1 - func 1 */
	"alt1",		/* Function 2 - Primary alternate (UART/SPI/I2C/PWM/JTAG/OSPI/etc) */
	"alt2",		/* Function 3 - Secondary alternate */
	"alt3",		/* Function 4 - Tertiary alternate */
	"alt4",		/* Function 5 - Quaternary alternate */
};

/* Group names - one per pin */
static const char * const k230_group_names[] = {
	"io0", "io1", "io2", "io3", "io4", "io5", "io6", "io7",
	"io8", "io9", "io10", "io11", "io12", "io13", "io14", "io15",
	"io16", "io17", "io18", "io19", "io20", "io21", "io22", "io23",
	"io24", "io25", "io26", "io27", "io28", "io29", "io30", "io31",
	"io32", "io33", "io34", "io35", "io36", "io37", "io38", "io39",
	"io40", "io41", "io42", "io43", "io44", "io45", "io46", "io47",
	"io48", "io49", "io50", "io51", "io52", "io53", "io54", "io55",
	"io56", "io57", "io58", "io59", "io60", "io61", "io62", "io63"
};

/* Number of groups (one per pin) */
#define K230_IOMUX_PIN_SIZE		4

/* Register bit fields */
#define K230_IO_SEL_SHIFT		11
#define K230_IO_SEL_MASK		0x7
#define K230_SL_SHIFT			10
#define K230_SL_MASK			0x1
#define K230_MSC_SHIFT			9
#define K230_MSC_MASK			0x1
#define K230_IE_SHIFT			8
#define K230_IE_MASK			0x1
#define K230_OE_SHIFT			7
#define K230_OE_MASK			0x1
#define K230_PU_SHIFT			6
#define K230_PU_MASK			0x1
#define K230_PD_SHIFT			5
#define K230_PD_MASK			0x1
#define K230_DS_SHIFT			1
#define K230_DS_MASK			0xF
#define K230_ST_SHIFT			0
#define K230_ST_MASK			0x1

/* Combined masks for read-modify-write */
#define K230_IO_SEL_BITS		(K230_IO_SEL_MASK << K230_IO_SEL_SHIFT)
#define K230_SL_BITS			(K230_SL_MASK << K230_SL_SHIFT)
#define K230_MSC_BITS			(K230_MSC_MASK << K230_MSC_SHIFT)
#define K230_IE_BITS			(K230_IE_MASK << K230_IE_SHIFT)
#define K230_OE_BITS			(K230_OE_MASK << K230_OE_SHIFT)
#define K230_PU_BITS			(K230_PU_MASK << K230_PU_SHIFT)
#define K230_PD_BITS			(K230_PD_MASK << K230_PD_SHIFT)
#define K230_DS_BITS			(K230_DS_MASK << K230_DS_SHIFT)
#define K230_ST_BITS			(K230_ST_MASK << K230_ST_SHIFT)

#define K230_CONFIG_MASK		(K230_IO_SEL_BITS | K230_SL_BITS | \
					 K230_IE_BITS | K230_OE_BITS | \
					 K230_PU_BITS | K230_PD_BITS | \
					 K230_DS_BITS | K230_ST_BITS)


/* Number of functions (IO_SEL is 3 bits = 8 possible functions) */
#define K230_NUM_FUNCTIONS		ARRAY_SIZE(k230_function_names)

/* Function names - generic names for function selectors */

struct k230_iomux {
	struct device *dev;
	void __iomem *base;
	struct pinctrl_dev *pctl;
	struct clk *clk;
	unsigned num_pins;
};

/* PINCTRL核心回调 */
static int k230_iomux_get_groups_count(struct pinctrl_dev *pctl)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);
	return iomux->num_pins;
}

static const char *k230_iomux_get_group_name(struct pinctrl_dev *pctl,
					      unsigned int group)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);
	static char name[16];

	/* Validate group number against actual number of pins */
	if (group >= iomux->num_pins)
		return NULL;

	snprintf(name, sizeof(name), "io%d", group);
	return name;
}

static int k230_iomux_get_group_pins(struct pinctrl_dev *pctl,
				      unsigned int group,
				      const unsigned int **pins,
				      unsigned int *num_pins)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);

	if (group >= iomux->num_pins)
		return -EINVAL;

	/*
	 * For this simple pinctrl driver, each group contains exactly one pin.
	 * The pin number is the same as the group number.
	 * Using a static buffer is safe here because the pinctrl core copies
	 * the pins array before this function returns.
	 */
	static unsigned int pin_buf;

	pin_buf = group;
	*pins = &pin_buf;
	*num_pins = 1;
	return 0;
}

/* PINMUX回调 */
static int k230_iomux_get_functions_count(struct pinctrl_dev *pctl)
{
	return K230_NUM_FUNCTIONS;
}

static const char *k230_iomux_get_function_name(struct pinctrl_dev *pctl,
						 unsigned int function)
{
	if (function < K230_NUM_FUNCTIONS)
		return k230_function_names[function];
	return NULL;
}

static int k230_iomux_get_function_groups(struct pinctrl_dev *pctl,
					  unsigned int function,
					  const char * const **groups,
					  unsigned int *num_groups)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);

	/* All pins can be used with any function */
	*groups = k230_group_names;
	*num_groups = iomux->num_pins;
	return 0;
}

static int k230_iomux_set_mux(struct pinctrl_dev *pctl,
			      unsigned int function,
			      unsigned int group)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);
	u32 val;

	/* Write function select to pin register */
	val = ioread32(iomux->base + (group * K230_IOMUX_PIN_SIZE));
	val &= ~K230_IO_SEL_BITS;
	val |= (function << K230_IO_SEL_SHIFT);

	//printk("set 0x%lx to 0x%x \n", (unsigned long)iomux->base + (group * K230_IOMUX_PIN_SIZE), val);
	iowrite32(val, iomux->base + (group * K230_IOMUX_PIN_SIZE));

	return 0;
}

/* PINCONFIG回调 */
static int k230_iomux_pinconf_get(struct pinctrl_dev *pctl, unsigned int pin,
				   unsigned long *config)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);
	u32 val;

	val = ioread32(iomux->base + (pin * K230_IOMUX_PIN_SIZE));

	switch (pinconf_to_config_param(*config)) {
	case PIN_CONFIG_BIAS_PULL_UP:
		return (val & K230_PU_BITS) ? 0 : -EINVAL;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		return (val & K230_PD_BITS) ? 0 : -EINVAL;
	case PIN_CONFIG_BIAS_DISABLE:
		return ((val & (K230_PU_BITS | K230_PD_BITS)) == 0) ? 0 : -EINVAL;
	case PIN_CONFIG_DRIVE_STRENGTH:
		*config = (val >> K230_DS_SHIFT) & K230_DS_MASK;
		return 0;
	case PIN_CONFIG_SLEW_RATE:
		*config = (val & K230_SL_BITS) ? 1 : 0;
		return 0;
	case PIN_CONFIG_INPUT_ENABLE:
		*config = (val & K230_IE_BITS) ? 1 : 0;
		return 0;
	case PIN_CONFIG_OUTPUT_ENABLE:
		*config = (val & K230_OE_BITS) ? 1 : 0;
		return 0;
	case PIN_CONFIG_OUTPUT:
		/* Output value is not directly accessible, return current IE/OE state */
		return -ENOTSUPP;
	case PIN_CONFIG_INPUT_SCHMITT:
		*config = (val & K230_ST_BITS) ? 1 : 0;
		return 0;
	default:
		return -ENOTSUPP;
	}
}

static int k230_iomux_pinconf_set(struct pinctrl_dev *pctl, unsigned int pin,
				   unsigned long *configs, unsigned int num_configs)
{
	struct k230_iomux *iomux = pinctrl_dev_get_drvdata(pctl);
	u32 val;
	unsigned int i;

	val = ioread32(iomux->base + (pin * K230_IOMUX_PIN_SIZE));

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);
		u32 arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_BIAS_DISABLE:
			val &= ~(K230_PU_BITS | K230_PD_BITS);
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			val &= ~K230_PU_BITS;
			val |= K230_PU_BITS;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			val &= ~K230_PD_BITS;
			val |= K230_PD_BITS;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			val &= ~K230_DS_BITS;
			val |= (arg & K230_DS_MASK) << K230_DS_SHIFT;
			break;
		case PIN_CONFIG_SLEW_RATE:
			val &= ~K230_SL_BITS;
			if (arg)
				val |= K230_SL_BITS;
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			val &= ~K230_IE_BITS;
			if (arg)
				val |= K230_IE_BITS;
			break;
		case PIN_CONFIG_OUTPUT_ENABLE:
			val &= ~K230_OE_BITS;
			if (arg)
				val |= K230_OE_BITS;
			break;
		case PIN_CONFIG_INPUT_SCHMITT:
			val &= ~K230_ST_BITS;
			if (arg)
				val |= K230_ST_BITS;
			break;
		default:
			return -ENOTSUPP;
		}
	}
	//printk("set 0x%lx to 0x%x \n", (unsigned long)iomux->base + (pin * K230_IOMUX_PIN_SIZE), val);
	iowrite32(val, iomux->base + (pin * K230_IOMUX_PIN_SIZE));
	return 0;
}

static int k230_iomux_pinconf_group_set(struct pinctrl_dev *pctl,
					unsigned int group,
					unsigned long *configs,
					unsigned int num_configs)
{
	return k230_iomux_pinconf_set(pctl, group, configs, num_configs);
}

static const struct pinctrl_ops k230_iomux_pctl_ops = {
	.get_groups_count = k230_iomux_get_groups_count,
	.get_group_name = k230_iomux_get_group_name,
	.get_group_pins = k230_iomux_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_pin,
	.dt_free_map = pinconf_generic_dt_free_map,
};

static const struct pinmux_ops k230_iomux_pmx_ops = {
	.get_functions_count = k230_iomux_get_functions_count,
	.get_function_name = k230_iomux_get_function_name,
	.get_function_groups = k230_iomux_get_function_groups,
	.set_mux = k230_iomux_set_mux,
	.strict = true,
};

static const struct pinconf_ops k230_iomux_conf_ops = {
	.is_generic = true,
	.pin_config_get = k230_iomux_pinconf_get,
	.pin_config_set = k230_iomux_pinconf_set,
	.pin_config_group_set = k230_iomux_pinconf_group_set,
};

/* pinctrl_desc 需要在 probe 中动态初始化 */
static const struct of_device_id k230_iomux_of_match[] = {
	{ .compatible = "canaan,k230-iomux", },
	{ /* sentinel */ },
};

static int k230_iomux_probe(struct platform_device *pdev)
{
	struct k230_iomux *iomux;
	struct resource *res;
	struct pinctrl_desc *desc;
	int ret;
	resource_size_t size;

	iomux = devm_kzalloc(&pdev->dev, sizeof(*iomux), GFP_KERNEL);
	if (!iomux)
		return -ENOMEM;

	iomux->dev = &pdev->dev;
	/* Map IOMUX registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get mem resource\n");
		return -ENODEV;
	}
	size = resource_size(res);
	iomux->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(iomux->base))
		return PTR_ERR(iomux->base);
	/* Calculate number of pins from register space size */
	/* Each pin has a 32-bit (4 byte) register */
	iomux->num_pins = size / 4;
	/* Validate the calculated number of pins */
	if (iomux->num_pins == 0 || iomux->num_pins > 64) {
		dev_err(&pdev->dev, "Invalid register size %pa, expected 4-256 bytes\n", &size);
		return -EINVAL;
	}
	/* Register pinctrl with dynamically initialized desc */
	desc = devm_kzalloc(&pdev->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	*desc = (struct pinctrl_desc){
		.name = "k230-iomux",
		.pctlops = &k230_iomux_pctl_ops,
		.pmxops = &k230_iomux_pmx_ops,
		.confops = &k230_iomux_conf_ops,
		.owner = THIS_MODULE,
	};
	/* Allocate and initialize pin descriptors dynamically */
	desc->pins = devm_kcalloc(&pdev->dev, iomux->num_pins,
				  sizeof(struct pinctrl_pin_desc), GFP_KERNEL);
	if (!desc->pins)
		return -ENOMEM;
	for (unsigned int i = 0; i < iomux->num_pins; i++) {
		char *pin_name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "io%d", i);
		if (!pin_name)
			return -ENOMEM;
		((struct pinctrl_pin_desc *)desc->pins)[i].number = i;
		((struct pinctrl_pin_desc *)desc->pins)[i].name = pin_name;
	}
	desc->npins = iomux->num_pins;
	ret = devm_pinctrl_register_and_init(&pdev->dev, desc, iomux, &iomux->pctl);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register pinctrl: %d\n", ret);
		return ret;
	}
	/* Get clock */
	iomux->clk = devm_clk_get(&pdev->dev, NULL);
	if (PTR_ERR(iomux->clk) == -EPROBE_DEFER)
		return -EPROBE_DEFER;
	/* Enable clock if available */
	if (!IS_ERR(iomux->clk)) {
		ret = clk_prepare_enable(iomux->clk);
		if (ret) {
			dev_err(&pdev->dev, "Failed to enable clock\n");
			return ret;
		}
	}
	/* Enable pinctrl */
	ret = pinctrl_enable(iomux->pctl);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to enable pinctrl: %d\n", ret);
		if (!IS_ERR(iomux->clk))
			clk_disable_unprepare(iomux->clk);
		return ret;
	}
	platform_set_drvdata(pdev, iomux);
	dev_info(&pdev->dev, "K230 IOMUX driver registered\n");

	return 0;
}

static int k230_iomux_remove(struct platform_device *pdev)
{
	struct k230_iomux *iomux = platform_get_drvdata(pdev);

	if (iomux->pctl)
		devm_pinctrl_unregister(&pdev->dev, iomux->pctl);

	if (!IS_ERR(iomux->clk))
		clk_disable_unprepare(iomux->clk);

	return 0;
}

static struct platform_driver k230_iomux_driver = {
	.driver = {
		.name = "k230-iomux",
		.of_match_table = k230_iomux_of_match,
	},
	.probe = k230_iomux_probe,
	.remove = k230_iomux_remove,
};

module_platform_driver(k230_iomux_driver);

MODULE_AUTHOR("Canaan Kendryte Technology Co., Ltd.");
MODULE_DESCRIPTION("K230 SoC IOMUX driver");
MODULE_LICENSE("GPL v2");
