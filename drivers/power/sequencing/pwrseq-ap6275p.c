// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Collabora Ltd.
 *
 * Datasheet: https://www.lcsc.com/datasheet/lcsc_datasheet_2203281730_AMPAK-Tech-AP6275P_C2984107.pdf
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regulator/consumer.h>
#include <linux/pwrseq/provider.h>

#define AP6275P_NUM_SUPPLIES 2
static const char * const ap6275p_supplies[AP6275P_NUM_SUPPLIES] = {
	"vbat",
	"vddio",
};

#define AP6275P_NUM_CLOCKS 2
static const char * const ap6275p_clocks[AP6275P_NUM_CLOCKS] = {
	"ref",
	"rtc",
};

struct ap6275p_ctx {
	struct pwrseq_device *pwrseq;
	struct device *dev;
	struct regulator_bulk_data supplies[AP6275P_NUM_SUPPLIES];
	struct clk_bulk_data clocks[AP6275P_NUM_CLOCKS];
	struct gpio_desc *bt_gpio;
	struct gpio_desc *wlan_gpio;
};


static int pwrseq_ap6275p_pwup_delay(struct pwrseq_device *pwrseq)
{
	msleep(50);
	return 0;
}

static int pwrseq_ap6275p_vregs_enable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);
	int ret;

	ret = regulator_bulk_enable(AP6275P_NUM_SUPPLIES, ctx->supplies);
	fsleep(100); /* Two RTC (32.678kHz) clock cycles */
	return ret;
}

static int pwrseq_ap6275p_vregs_disable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	return regulator_bulk_disable(AP6275P_NUM_SUPPLIES, ctx->supplies);
}

static const struct pwrseq_unit_data pwrseq_ap6275p_vregs_unit_data = {
	.name = "regulators-enable",
	.enable = pwrseq_ap6275p_vregs_enable,
	.disable = pwrseq_ap6275p_vregs_disable,
};

static int pwrseq_ap6275p_clk_enable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	return clk_bulk_prepare_enable(AP6275P_NUM_CLOCKS, ctx->clocks);
}

static int pwrseq_ap6275p_clk_disable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	clk_bulk_disable_unprepare(AP6275P_NUM_CLOCKS, ctx->clocks);

	return 0;
}

static const struct pwrseq_unit_data pwrseq_ap6275p_clk_unit_data = {
	.name = "clock-enable",
	.enable = pwrseq_ap6275p_clk_enable,
	.disable = pwrseq_ap6275p_clk_disable,
};

static const struct pwrseq_unit_data *pwrseq_ap6275p_unit_deps[] = {
	&pwrseq_ap6275p_clk_unit_data,
	&pwrseq_ap6275p_vregs_unit_data,
	NULL
};

static int pwrseq_ap6275p_bt_enable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	gpiod_set_value_cansleep(ctx->bt_gpio, 1);

	return 0;
}

static int pwrseq_ap6275p_bt_disable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	gpiod_set_value_cansleep(ctx->bt_gpio, 0);

	return 0;
}

static const struct pwrseq_unit_data pwrseq_ap6275p_bt_unit_data = {
	.name = "bluetooth-enable",
	.deps = pwrseq_ap6275p_unit_deps,
	.enable = pwrseq_ap6275p_bt_enable,
	.disable = pwrseq_ap6275p_bt_disable,
};

static int pwrseq_ap6275p_wlan_enable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	gpiod_set_value_cansleep(ctx->wlan_gpio, 1);

	return 0;
}

static int pwrseq_ap6275p_wlan_disable(struct pwrseq_device *pwrseq)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);

	gpiod_set_value_cansleep(ctx->wlan_gpio, 0);

	return 0;
}

static const struct pwrseq_unit_data pwrseq_ap6275p_wlan_unit_data = {
	.name = "wlan-enable",
	.deps = pwrseq_ap6275p_unit_deps,
	.enable = pwrseq_ap6275p_wlan_enable,
	.disable = pwrseq_ap6275p_wlan_disable,
};

static const struct pwrseq_target_data pwrseq_ap6275p_bt_target_data = {
	.name = "bluetooth",
	.unit = &pwrseq_ap6275p_bt_unit_data,
	.post_enable = pwrseq_ap6275p_pwup_delay,
};

static const struct pwrseq_target_data pwrseq_ap6275p_wlan_target_data = {
	.name = "wlan",
	.unit = &pwrseq_ap6275p_wlan_unit_data,
	.post_enable = pwrseq_ap6275p_pwup_delay,
};

static const struct pwrseq_target_data *pwrseq_ap6275p_targets[] = {
	&pwrseq_ap6275p_bt_target_data,
	&pwrseq_ap6275p_wlan_target_data,
	NULL
};

static int pwrseq_ap6275p_match(struct pwrseq_device *pwrseq,
				struct device *dev)
{
	struct ap6275p_ctx *ctx = pwrseq_device_get_drvdata(pwrseq);
	struct fwnode_reference_args ref;
	struct fwnode_handle *fwnode;
	int ret;

	/* The PMU supplies power to the Bluetooth and WLAN modules. */
	ret = fwnode_property_get_reference_args(dev->fwnode, "vdd-supply",
						 NULL, 0, 0, &ref);
	if (ret)
		return 0;

	/* parent is regulators node */
	fwnode = fwnode_get_next_parent(ref.fwnode);

	/* parent is PMU node */
	fwnode = fwnode_get_next_parent(fwnode);

	ret = (fwnode == ctx->dev->fwnode) ? 1 : 0;

	fwnode_handle_put(fwnode);

	return ret;
}

static int pwrseq_ap6275p_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ap6275p_ctx *ctx;
	struct pwrseq_config config = {0};
	int ret, i;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dev = dev;

	for (i=0; i<AP6275P_NUM_SUPPLIES; i++)
		ctx->supplies[i].supply = ap6275p_supplies[i];

	for (i=0; i<AP6275P_NUM_CLOCKS; i++)
		ctx->clocks[i].id = ap6275p_clocks[i];

	ret = devm_regulator_bulk_get(dev, AP6275P_NUM_SUPPLIES, ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ret = devm_clk_bulk_get(dev, AP6275P_NUM_CLOCKS, ctx->clocks);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get clocks\n");

	ctx->bt_gpio = devm_gpiod_get_optional(dev, "bt-enable", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->bt_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->bt_gpio),
				     "Failed to get the Bluetooth enable GPIO\n");

	ctx->wlan_gpio = devm_gpiod_get_optional(dev, "wlan-enable", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->wlan_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->wlan_gpio),
				     "Failed to get the WLAN enable GPIO\n");

	config.parent = dev;
	config.owner = THIS_MODULE;
	config.drvdata = ctx;
	config.match = pwrseq_ap6275p_match;
	config.targets = pwrseq_ap6275p_targets;

	ctx->pwrseq = devm_pwrseq_device_register(dev, &config);
	if (IS_ERR(ctx->pwrseq))
		return dev_err_probe(dev, PTR_ERR(ctx->pwrseq),
				     "Failed to register the power sequencer\n");

	return 0;
}

static const struct of_device_id pwrseq_ap6275p_of_match[] = {
	{
		.compatible = "ampak,ap6275p-pmu",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, pwrseq_ap6275p_of_match);

static struct platform_driver pwrseq_ap6275p_driver = {
	.driver = {
		.name = "pwrseq-ap6275p",
		.of_match_table = pwrseq_ap6275p_of_match,
	},
	.probe = pwrseq_ap6275p_probe,
};
module_platform_driver(pwrseq_ap6275p_driver);

MODULE_AUTHOR("Sebastian Reichel <sebastian.reichel@collabora.com>");
MODULE_DESCRIPTION("AP6275P Power Sequencing driver");
MODULE_LICENSE("GPL");
