// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Kendryte K230 temperature sensor support driver
 *
 * Copyright (C) 2024, Canaan Bright Sight Co., Ltd
 */

#include <linux/clk.h>
#include <linux/cpu_cooling.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/types.h>

#define TS_CONFIG 0x00
#define TS_DATA 0x04
#define TS_POWERDOWN 0x22
#define TS_POWERON 0x23

struct canaan_thermal_data {
	struct thermal_zone_device *tz;
	void __iomem *base;
};

/*
// TS_DATA 寄存器内低12位数值转为实际温度
def calculate_temperature(data):
    base = 3565.87
    d1 = pow(data, 1) * 7.10128
    d2 = pow(data, 2) * 4.36150e-3
    d3 = pow(data, 3) * 1.10063e-6
    d4 = pow(data, 4) * 1.01472e-10
    return d4-d3+d2-d1+base
*/

// 因为公式计算太耗时，预先算好了表格
#define TS_DATA_TABLE_LOWER 3030
#define TS_DATA_TABLE_UPPER 3220
#define TS_DATA_TABLE_SIZE (TS_DATA_TABLE_UPPER - TS_DATA_TABLE_LOWER)
const int temperature_table[191] = {
    26988,     27294,     27600,     27906,     28211,     28517,     28822,     29128,
    29433,     29739,     30044,     30349,     30654,     30959,     31263,     31568,
    31873,     32177,     32481,     32786,     33090,     33394,     33698,     34002,
    34306,     34609,     34913,     35217,     35520,     35824,     36127,     36430,
    36733,     37036,     37339,     37642,     37945,     38248,     38551,     38853,
    39156,     39458,     39760,     40063,     40365,     40667,     40969,     41271,
    41573,     41875,     42177,     42479,     42780,     43082,     43384,     43685,
    43987,     44288,     44589,     44891,     45192,     45493,     45794,     46095,
    46396,     46697,     46998,     47299,     47600,     47900,     48201,     48502,
    48802,     49103,     49403,     49704,     50004,     50305,     50605,     50905,
    51205,     51506,     51806,     52106,     52406,     52706,     53006,     53306,
    53606,     53906,     54206,     54506,     54806,     55106,     55406,     55705,
    56005,     56305,     56605,     56904,     57204,     57504,     57804,     58103,
    58403,     58702,     59002,     59302,     59601,     59901,     60200,     60500,
    60800,     61099,     61399,     61698,     61998,     62297,     62597,     62896,
    63196,     63495,     63795,     64095,     64394,     64694,     64993,     65293,
    65593,     65892,     66192,     66491,     66791,     67091,     67390,     67690,
    67990,     68290,     68589,     68889,     69189,     69489,     69789,     70089,
    70389,     70689,     70989,     71289,     71589,     71889,     72189,     72489,
    72789,     73089,     73390,     73690,     73990,     74291,     74591,     74892,
    75192,     75493,     75794,     76094,     76395,     76696,     76997,     77297,
    77598,     77899,     78201,     78502,     78803,     79104,     79405,     79707,
    80008,     80310,     80611,     80913,     81215,     81517,     81818,     82120,
    82422,     82725,     83027,     83329,     83631,     83934,     84236
};
static int calculate_temperature(int data)
{
	int index = data - TS_DATA_TABLE_LOWER;
	if(index > TS_DATA_TABLE_SIZE)
		index = TS_DATA_TABLE_SIZE;
	if (index < 0)
		index = 0;
	return temperature_table[index];
}
static int canaan_get_temp(struct thermal_zone_device *tz, int *temp)
{
	struct canaan_thermal_data *data = tz->devdata;
	u32 val = 0;

	iowrite32(TS_POWERDOWN, data->base + TS_CONFIG);
	iowrite32(TS_POWERON, data->base + TS_CONFIG);
	msleep(20);

	while (1) {
		val = ioread32(data->base + TS_DATA);
		// msleep(2600);

		if (val >> 12) {
			*temp = calculate_temperature(val & 0x0FFF);
			break;
		}
	}

	return 0;
}

static struct thermal_zone_device_ops canaan_tz_ops = {
	.get_temp = canaan_get_temp,
};

static const struct of_device_id of_canaan_thermal_match[] = {
	{ .compatible = "canaan,k230-tsensor" },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_canaan_thermal_match);

static int canaan_thermal_probe(struct platform_device *pdev)
{
	struct canaan_thermal_data *data;
	struct resource *res;
	int ret;

	dev_vdbg(&pdev->dev, "[TS]: %s %d\n", __func__, __LINE__);

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	platform_set_drvdata(pdev, data);

	data->tz = thermal_tripless_zone_device_register(
		"canaan_thermal_zone", data, &canaan_tz_ops, NULL);

	if (IS_ERR(data->tz)) {
		ret = PTR_ERR(data->tz);
		dev_err(&pdev->dev,
			"failed to register thermal zone device %d\n", ret);
		return ret;
	}

	iowrite32(TS_POWERDOWN, data->base + TS_CONFIG);
	iowrite32(TS_POWERON, data->base + TS_CONFIG);
	msleep(20);

	dev_vdbg(&pdev->dev, "[TS]: %s %d\n", __func__, __LINE__);

	return 0;
}

static int canaan_thermal_remove(struct platform_device *pdev)
{
	struct canaan_thermal_data *data = platform_get_drvdata(pdev);

	thermal_zone_device_unregister(data->tz);

	return 0;
}

static struct platform_driver canaan_thermal = {
	.driver = {
		.name = "canaan_thermal",
		.of_match_table = of_canaan_thermal_match,
	},
	.probe = canaan_thermal_probe,
	.remove = canaan_thermal_remove,
};
module_platform_driver(canaan_thermal);

MODULE_DESCRIPTION("Thermal driver for canaan k230 Soc");
MODULE_LICENSE("GPL");
