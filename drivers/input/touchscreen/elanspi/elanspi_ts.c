#include <linux/spi/spi.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/of_gpio.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#ifdef CONFIG_OF
	#include <linux/of.h>
	#include <linux/of_irq.h>
#endif

#include <linux/miscdevice.h>
#include "elanspi_ts.h"


struct elanspi_ts_data *gspits;


static int elanspi_parse_dt(struct device *dev, struct hw_spi_info hw_info)
{
	return 0;
}

static int elanspi_gpio_init(struct hw_spi_info *hw_info)
{
	return 0;
}

static int elanspi_hw_initial(struct elanspi_ts_data *spits)
{
	int ret = 0;
	struct spi_device *spi = spits->spi;
	struct hw_spi_info *hw_info;

	pr_err("[elanspi] run malloc\n");
	hw_info = devm_kzalloc(&spi->dev, sizeof(struct hw_spi_info), GFP_KERNEL);
//	hw_info = kzalloc(sizeof(struct hw_spi_info), GFP_KERNEL);
	if (!hw_info) {
		dev_err(&spi->dev,
				"ETP Failed to allocate memory for hw_info\n");
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	 if (spi->dev.of_node) {
		ret = elanspi_parse_dt(&spi->dev, hw_info);
		if (ret)
			return ret;
	 } else
#endif
	{
		dev_err(&spi->dev," [elanspi 4.2: start get platform data\n");
		hw_info = spi->dev.platform_data;
		hw_info->irq = spi->irq;
	}
	
    spits->hw_info = *hw_info;

	dev_info(&spi->dev, "[elan] rst = %d, int = %d, irq=%d\n",hw_info->rst_gpio, hw_info->intr_gpio,hw_info->irq);
	dev_info(&spi->dev, "[elan] lcm_x = %d, lcm_y = %d\n",hw_info->lcm_xres, hw_info->lcm_yres);

	ret = elanspi_gpio_init(&spits->hw_info);

	return 0;
}

static int elanspi_ts_probe(struct spi_device *spi)
{
	struct elanspi_ts_data *spits = NULL;
	int err = 0;

	printk("[elanspi enter spi touch probe\n");

	/*1 spi device setting*/
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;

	if (SPI_MAX_SPEED < spi->max_speed_hz)
		spi->max_speed_hz = SPI_MAX_SPEED; //devices support max HZ

	spits = kzalloc(sizeof(struct elanspi_ts_data), GFP_KERNEL);
	if (spits == NULL) {
		printk("[elanspi] %s: allocate elanspi_ts_data failed\n", __func__);
		return -ENOMEM;
	}
	
	spits->spi = spi;
	spi_set_drvdata(spi, spits);
	//spits->spi = spi;
	gspits = spits;

	/*board data or dt parse*/
	err = elanspi_hw_initial(spits);

	/*power setting*/
	/*hw reset*/
	/*elanspi ic initial*/
	/*input devices register*/
	/*register irq handler*/
	return 0;
}


static int elanspi_ts_remove(struct spi_device *spi)
{
	printk("[elanspi:%s enter\n", __func__);
	return 0;
}

static const struct spi_device_id elanspi_ts_id[] = {
	{ELANSPI_TS_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(spi, elanspi_ts_id);

#ifdef CONFIG_OF
static struct of_device_id elanspi_ts_of_match[] = {
	{.compatible = "elan,spitouch",},
	{}
};

MODULE_DEVICE_TABLE(of, elanspi_ts_of_match);
#endif

static struct spi_driver elanspi_ts_driver = {
	.driver = {
		.name	= ELANSPI_TS_NAME,
		.owner	= THIS_MODULE,
		//.pm		= elanspi_ts_pm,
#ifdef CONFIG_OF
		.of_match_table = elanspi_ts_of_match,
#endif
	},
	.probe		= elanspi_ts_probe,
	.remove		= elanspi_ts_remove,
	.id_table	= elanspi_ts_id,
};

static int __init elanspi_ts_init(void)
{
	printk("[elanspi]:%s enter\n", __func__);
	return spi_register_driver(&elanspi_ts_driver);
}

static void __exit elanspi_ts_exit(void)
{
	printk("[elanspi]:%s enter\n", __func__);
	spi_unregister_driver(&elanspi_ts_driver);
}

module_init(elanspi_ts_init);
module_exit(elanspi_ts_exit);

MODULE_DESCRIPTION("ELAN SPI TOUCH DRIVER");
MODULE_LICENSE("GPL");


