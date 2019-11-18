#ifndef _LINUX_ELANSPI_TS_H
#define _LINUX_ELANSPI_TS_H


#define ELANSPI_TS_NAME "elanspi_ts"
#define SPI_MAX_SPEED	(3*1000*1000)


struct hw_spi_info {
	int vendor_id;
	int intr_gpio;
	int rst_gpio;
	int lcm_xres;
	int lcm_yres;
	int irq;
};

struct elan_spi_tranfer {
	uint8_t *tx;
	uint8_t *rx;
	ssize_t size;
};

struct elanspi_ts_data {
	struct hw_spi_info hw_info;
	struct spi_device *spi;
	struct elan_spi_tranfer spi_tranfer;
	struct input_dev *input_dev;
	struct miscdevice elan_fp_mdev;
};


#endif
