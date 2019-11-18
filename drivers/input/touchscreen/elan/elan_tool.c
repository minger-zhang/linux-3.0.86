#include "elan_ts.h"
#include <linux/of_gpio.h>

static ssize_t store_disable_irq(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);

	disable_irq(ts->hw_info.irq_num);
	dev_info(&client->dev, "Disable IRQ.\n");
	return count;
}
static DEVICE_ATTR(disable_irq, S_IWUSR, NULL, store_disable_irq);

static ssize_t store_enable_irq(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);

	enable_irq(ts->hw_info.irq_num);
	dev_info(&client->dev, "Enable IRQ.\n");
	return count;
}
static DEVICE_ATTR(enable_irq, S_IWUSR, NULL, store_enable_irq);


static ssize_t store_reset(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	elan_ts_hw_reset(&ts->hw_info);
	dev_info(&client->dev,
			"Reset Touch Screen Controller!\n");
	return count;
}
static DEVICE_ATTR(reset, S_IWUSR, NULL, store_reset);

static ssize_t show_gpio_int(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	return sprintf(buf, "%d\n",
			gpio_get_value(ts->hw_info.intr_gpio));
}
static DEVICE_ATTR(gpio_int, S_IRUGO, show_gpio_int, NULL);

static ssize_t store_calibrate(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);	
	int ret = 0;

	disable_irq(ts->hw_info.irq_num);
	ret = elan_ts_calibrate(client);
	
	if (ret == 0)
		dev_info(&client->dev, "ELAN CALIBRATE Success\n");
	else
		dev_err(&client->dev, "ELAN CALIBRATE Fail\n");
	
	enable_irq(ts->hw_info.irq_num);
	
	return count;
}
static DEVICE_ATTR(calibrate, S_IWUSR, NULL, store_calibrate);


static ssize_t store_check_rek(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;

	disable_irq(ts->hw_info.irq_num);
	ret = elan_ts_check_calibrate(client);
	
	if (ret)
		dev_err(&client->dev, "ELAN CALIBRATE CHECK Fail\n");
	else
		dev_info(&client->dev, "ELAN CHECK CALIBRATE Success\n");
	
	enable_irq(ts->hw_info.irq_num);
	
	return count;
}
static DEVICE_ATTR(check_rek, S_IWUSR, NULL, store_check_rek);

static ssize_t show_fw_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	return sprintf(buf, "FW VER = 0x%04x, FW ID = 0x%04x, BC VER = 0x%04x, tx:rx = %d:%d\n",\
			ts->fw_info.fw_ver, ts->fw_info.fw_id, ts->fw_info.fw_bcv, ts->fw_info.tx, ts->fw_info.rx);
}
static ssize_t store_fw_info(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	disable_irq(ts->hw_info.irq_num);
	ret = __fw_packet_handler(ts->client);
	enable_irq(ts->hw_info.irq_num);

	return count;
}
static DEVICE_ATTR(fw_info, S_IWUSR | S_IRUSR, show_fw_info, store_fw_info);

static ssize_t store_iap_status(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;
	
	disable_irq(ts->hw_info.irq_num);
	elan_ts_hw_reset(&ts->hw_info);
	mdelay(200);
	ret = __hello_packet_handler(client, ts->chip_type);
	disable_irq(ts->hw_info.irq_num);

	return count;
}

static ssize_t show_iap_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	return sprintf(buf, "IAP STATUS = %s\n",(ts->recover < 0 ? "UNKNOW":(ts->recover == 0x01 ? "RECOVERY":"NORMAL")));
}
static DEVICE_ATTR(iap_status, S_IWUSR | S_IRUSR, show_iap_status, store_iap_status);

static ssize_t store_fw_upgrade(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);

	disable_irq(ts->hw_info.irq_num);
	
	get_vendor_fw(ts,ts->fw_store_type);
	FW_Update(ts->client);
	enable_irq(ts->hw_info.irq_num);

	return count;	
}
static DEVICE_ATTR(fw_update, S_IWUSR, NULL,  store_fw_upgrade);


static ssize_t show_fw_store(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	if (ts->fw_store_type > -1 || ts->fw_store_type < 3) {
		return sprintf(buf, "FW STORE = %s\n",\
				(ts->fw_store_type == FROM_SYS_ETC_FIRMWARE ? "/system/etc/firmware/elants_i2c.ekt":\
				(ts->fw_store_type == FROM_SDCARD_FIRMWARE ? "/data/local/tmp/elants_i2c.ekt":"build in driver")));
	} 
}


static ssize_t store_fw_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{

	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int type;

	sscanf(buf,"%d",&type);
	if (type > -1 || type < 3) {	
		ts->fw_store_type = type;
	} else {
		dev_info(&client->dev, "[elan] fw store type out of range!!!\n");
	}
	return count;

}
static DEVICE_ATTR(fw_store, S_IWUSR | S_IRUSR, show_fw_store, store_fw_store);

static ssize_t store_tp_module_test(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{

	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;

	ret = tp_module_test(ts);
	return count;
}
static DEVICE_ATTR(tp_module_test, S_IWUSR, NULL,  store_tp_module_test);

static ssize_t store_tp_print_level(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int level = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	sscanf(buf, "%x", &level);
	ts->level = level;
	return count;
}


static ssize_t show_tp_print_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct elan_ts_data *ts = i2c_get_clientdata(client);
	
	return sprintf(buf, "PRINT LEVEL = %s\n",\
			(ts->level == TP_DEBUG ? "DEBUG":(ts->level == TP_INFO ? "INFO" : (ts->level == TP_WARNING ?"WARNING":"ERROR"))));
}
static DEVICE_ATTR(tp_print_level, S_IWUSR | S_IRUSR, show_tp_print_level, store_tp_print_level);

static struct attribute *elan_default_attributes[] = {
	&dev_attr_enable_irq.attr,
	&dev_attr_disable_irq.attr,
	&dev_attr_gpio_int.attr,
	&dev_attr_reset.attr,
	&dev_attr_calibrate.attr,
	&dev_attr_check_rek.attr,
	&dev_attr_fw_info.attr,
	&dev_attr_iap_status.attr,
	&dev_attr_fw_update.attr,
	&dev_attr_fw_store.attr,
	&dev_attr_tp_module_test.attr,
	&dev_attr_tp_print_level.attr,
	NULL
};

static struct attribute_group elan_default_attribute_group = {
	.name = "elan_ktf",
	.attrs = elan_default_attributes,
};

int elan_sysfs_attri_file(struct elan_ts_data *ts)
{
	int err = 0;
	
	err = sysfs_create_group(&ts->client->dev.kobj, &elan_default_attribute_group);
	if ( err ) {
		dev_err(&ts->client->dev, "[elan] %s sysfs create group error\n",__func__);
	} else {
		dev_err(&ts->client->dev,"[elan] %s sysfs create group success\n",__func__);
	}

	return err;
}

void elan_sysfs_attri_file_remove(struct elan_ts_data *ts)
{
	 sysfs_remove_group(&ts->client->dev.kobj, &elan_default_attribute_group);
}



