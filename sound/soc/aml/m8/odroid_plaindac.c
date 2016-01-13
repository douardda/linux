/*
 * ASoC driver for PolyVection's PlainDAC+ / PlainDSP
 *
 * Author: Howard Mitchell <hm@hmdedded.co.uk>
 * Modified by: Philip Voigt <pv@polyvection.com> to work with PlainDAC.
 *              David Douard <david.douard@sdfa3.org> to work with PlainDAC on ODroid.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_platform.h>

#ifdef CONFIG_USE_OF
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <plat/io.h>
#include <mach/pinmux.h>
#endif

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "odroid_audio.h"

static const struct of_device_id odroid_plaindac_dt_ids[] = {
	{ .compatible = "polyvection,odroidc-plaindac", },
	{ }
};
MODULE_DEVICE_TABLE(of, odroid_plaindac_dt_ids);

static struct snd_soc_dai_link plaindac_dai = {
	.name		= "PlainDAC",
	.stream_name	= "PlainDAC HiFi",
        .cpu_dai_name   = "aml-i2s-dai.0",
        .platform_name  = "aml-i2s.0",
	.codec_dai_name	= "pcm512x-hifi",
	.dai_fmt        = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS,
};

static struct snd_soc_card plaindac_soc_card = {
	.owner = THIS_MODULE,
	.dai_link = &plaindac_dai,
	.num_links = 1,
};

static void i2s_pinmux_init(struct snd_soc_card *card)
{
	struct odroid_audio_private_data *p_odroid_audio;
	p_odroid_audio = snd_soc_card_get_drvdata(card);
	p_odroid_audio->pin_ctl = devm_pinctrl_get_select(card->dev, "odroid_i2s");
	printk(KERN_DEBUG "=%s==,i2s_pinmux_init done,---%d\n",__func__,p_odroid_audio->det_pol_inv);
}

static void i2s_pinmux_deinit(struct snd_soc_card *card)
{
	struct odroid_audio_private_data *p_odroid_audio;
	
	p_odroid_audio = snd_soc_card_get_drvdata(card);
	if(p_odroid_audio->pin_ctl)
		devm_pinctrl_put(p_odroid_audio->pin_ctl);
}

static int odroid_plaindac_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct odroid_audio_private_data *p_odroid_audio;

	int ret = 0;
	if (!np) {
	  dev_err(&pdev->dev, "Must be instantiated using device tree\n");
	  return -EINVAL;
	}

	printk("odroid_plaindac_probe\n");
	plaindac_dai.codec_of_node = of_parse_phandle(np, "polyvection,audio-codec", 0);
	if (!plaindac_dai.codec_of_node)
		return -EINVAL;

	plaindac_dai.platform_of_node = plaindac_dai.cpu_of_node;
	printk("odroid_plaindac_probe set dai\n");

	p_odroid_audio = devm_kzalloc(&pdev->dev,
				      sizeof(struct odroid_audio_private_data), GFP_KERNEL);
	if (!p_odroid_audio) {
	  dev_err(&pdev->dev, "Can't allocate odroid_audio_private_data\n");
	  return -ENOMEM;
	}
	plaindac_soc_card.dev = &pdev->dev;
	platform_set_drvdata(pdev, &plaindac_soc_card);
	snd_soc_card_set_drvdata(&plaindac_soc_card, p_odroid_audio);
	
	ret = snd_soc_of_parse_card_name(&plaindac_soc_card, "polyvection,model");
	if (ret)
	  goto err;

	printk("odroid_plaindac_probe allocated private data\n");
	ret = snd_soc_register_card(&plaindac_soc_card);
	if (ret) {
	  dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
	  goto err;
	}
	printk("odroid_plaindac_probe registered card\n");

	i2s_pinmux_init(&plaindac_soc_card);
	printk("odroid_plaindac_probe i2s pinmux initializedd\n");

	return ret;

err:
	kfree(p_odroid_audio);
	return ret;
}

static int odroid_plaindac_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct odroid_audio_private_data *p_odroid_audio;

	p_odroid_audio = snd_soc_card_get_drvdata(card);
	snd_soc_unregister_card(card);

	i2s_pinmux_deinit(card);
	kfree(p_odroid_audio);
	return 0;
}

static struct platform_driver odroid_plaindac_driver = {
	.probe		= odroid_plaindac_probe,
	.remove		= odroid_plaindac_remove,
	.driver		= {
		.name	= "odroidc_plaindac",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(odroid_plaindac_dt_ids),
	},
};

static int __init plaindac_init(void)
{
	/*
	 * When using device tree the devices will be created dynamically.
	 * Only register platfrom driver structure.
	 */
	return platform_driver_register(&odroid_plaindac_driver);
}

static void __exit plaindac_exit(void)
{
	platform_driver_unregister(&odroid_plaindac_driver);
	return;
}

module_init(plaindac_init);
module_exit(plaindac_exit);

MODULE_AUTHOR("Howard Mitchell, David Douard <david.douard@sdfa3.org>");
MODULE_DESCRIPTION("plaindac ASoC driver for ODroidC1");
MODULE_LICENSE("GPL");
