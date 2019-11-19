#include "elan_ts.h"
#include "elan_mmi.h"
#include <linux/vmalloc.h>

static short *g_test_adc_databuf = NULL;
static short *g_test_adc_databuf2 = NULL;
static uint8_t *g_rawdatabuf = NULL;
static short *g_totlebuf = NULL;
struct dynamic_anang_para *paramter = NULL;//kzalloc(sizeof(struct dynamic_anang_para), GFP_KERNEL);



void print_rawdata(const int rx, const int tx, int mode, void *data)
{
	int i = 0;
	int j = 0;
	short (*databuf)[rx] = NULL;
	
	if (!g_totlebuf) {
		printk("%s, g_totlebuf is NULL\n", __func__);
		return;
	}
	
	switch (mode) {
		case NOISE_ADC_PRINT:    // noise adc data print
			for (i = 0; i < tx; i++) {
				printk("[elan_mmi]%02d:", i + 1);
				for (j = 0; j < rx; j++) {
					printk("%04d,", g_totlebuf[j + i * rx]);
					if (j == rx - 1) {
						printk("\n");
					}
				}
			}
			break;
		
		case SHORT_DATA_PRINT:    // short data print
			printk("[elan_mmi]:Rx:");
			for (i = 0; i < (rx + tx); i++) {
				printk("%04d,", g_totlebuf[i]);
				if (i == rx - 1) {
					printk("\n");
					printk("[elan_mmi]:Tx:");
				}
			}
			printk("\n");
			break;
		
		case RXTX_DIFF_PRINT:    // tx rx open diff data print
			databuf = (short (*)[rx])data;
			if (databuf == NULL) {
				printk("[elan]:%s,databuf is null\n", __func__);
				return;
			}
			
			for (i = 0; i < tx; i++) {
				printk("[elan_mmi]%02d:", i + 1);
				for (j = 0; j < rx; j++) {
					for (j = 0; j < rx; j++) {
						printk("%04ld,", abs(databuf[i][j]));
						if (j == rx - 1) {
							printk("\n");
						}
					}
				}
			}
			break;

		default:
			printk("[elan]:unknown print rwadata mode!\n");
			break;
	}
	return;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from = NULL;
	char *copy_to = NULL;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while ((*copy_from != '\n') && (*copy_from != '\r') && (*copy_from != '\0'));
	*copy_to = '\0';
}

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n' && **ptr != '\0');
	
	if (**ptr == '\0') {
		return;
	}
	
	*ptr = *ptr + 1;
}

static void parse_valid_data(char *buf_start, loff_t buf_size,
		char *ptr, int32_t *data, int rows, int columns)
{
	int i = 0;
	int j = 0;
	char *token = NULL;
	char *tok_ptr = NULL;
	char row_data[512] = {0}; // ensure have enough space to save one line data
	int col_count = 0;

	if (!ptr) {
		printk("%s, ptr is NULL\n", __func__);
		return;
	}
	
	if (!data) {
		printk("%s, data is NULL\n", __func__);
		return;
	}

	for (i = 0; i < rows; i++) {
		// copy this line to row_data buffer
		memset(row_data, 0, sizeof(row_data));
		copy_this_line(row_data, ptr);
		tok_ptr = row_data;
		col_count = 0;
		while ((token = strsep(&tok_ptr, ", \t\n\r\0")) && (col_count < columns)) {
			if (strlen(token) == 0) {
				continue;
			}
			
			data[j] = (int32_t)simple_strtol(token, NULL, 10);
			j++;
			col_count++;
		}
		goto_next_line(&ptr);	// next row
		if (!ptr || (strlen(ptr) == 0) || (ptr >= (buf_start + buf_size))) {
			printk("invalid ptr, return row = %d\n", i);
			break;
		}
	}
	
	return;
}

static void print_data(char *target_name, int32_t *data, int rows, int columns)
{
	int i = 0;
	int j = 0;

	if (data == NULL) {
		printk("rawdata is NULL\n");
		return;
	}
	
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			printk("\t%d", data[i * columns + j]);
		}
		printk("\n");
	}
	
	return;
}

static int elan_parse_csvfile(char *file_path, char *target_name, int32_t  *data, int rows, int columns)
{
	struct file *fp = NULL;
	struct kstat stat;
	int ret = 0;
	int32_t read_ret = 0;
	char *buf = NULL;
	char *ptr = NULL;
	mm_segment_t org_fs;
	loff_t pos = 0;
	org_fs = get_fs();
	set_fs(KERNEL_DS);


	if (file_path == NULL) {
		printk("[elan]file path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	if (target_name == NULL) {
		printk("target path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		printk("%s, filp_open error, file name is %s.\n", __func__, file_path);
		ret = -EPERM;
		goto exit_free;
	}

	ret = vfs_stat(file_path, &stat);
	if (ret) {
		printk("%s, failed to get file stat.\n", __func__);
		ret = -ENOENT;
		goto exit_free;
	}

	buf = (char *)vmalloc(stat.size + 1);
	if (buf == NULL) {
		printk("%s: vmalloc %lld bytes failed.\n", __func__, stat.size);
		ret = -ESRCH;
		goto exit_free;
	}
	memset(buf, 0, stat.size + 1);
	read_ret = vfs_read(fp, buf, stat.size, &pos);
	if (read_ret > 0) {
		buf[stat.size] = '\0';
		ptr = buf;
		ptr = strstr(ptr, target_name); //find target start addr in buf
		if (ptr == NULL) {
			printk("%s: load %s failed 1!\n", __func__, target_name);
			ret = -EINTR;
			goto exit_free;
		}

		// walk thru this line
		goto_next_line(&ptr);//ptr is target name and ++ptr if data start addr
		if ((ptr == NULL) || (strlen(ptr) == 0)) {
			printk("%s: load %s failed 2!\n", __func__, target_name);
			ret = -EIO;
			goto exit_free;
		}

		// analyze the data
		if (data) {
			parse_valid_data(buf, stat.size, ptr, data, rows,columns);
			//print_data(target_name, data, rows, columns);
		} else {
			printk("%s: load %s failed 3!\n", __func__, target_name);
			ret = -EINTR;
			goto exit_free;
		}
	} else {
		printk("%s: ret=%d,read_ret=%d, stat.size=%lld\n", __func__,ret, read_ret, stat.size);
		ret = -ENXIO;
		goto exit_free;
	} 
	
	ret = 0;
exit_free:
	//printk("%s exit free\n", __func__);
	set_fs(org_fs);
	if (buf) {
	//	printk("vfree buf\n");
		vfree(buf);
		buf = NULL;
	}
	
	if (!IS_ERR_OR_NULL(fp)) {        // fp open fail not means fp is NULL, so free fp may cause Uncertainty
	//	printk("filp close\n");
		filp_close(fp, NULL);
		fp = NULL;
	}
	return ret;
}

static int elan_get_threshold_from_csvfile(int fw_id, int columns, int rows, char *target_name, int32_t *data)
{
	char file_path[100] = {0};
	char file_name[64] = {0};
	int result = 0;


	if ((!data) || (!target_name)) {
			printk("parse csvfile failed, data or target_name or g_elan_ts or elan_chip_data is NULL\n");
			return -1;
	}

	snprintf(file_name, sizeof(file_name), "elanchip_%4X_raw.csv", fw_id);
	snprintf(file_path, sizeof(file_path), "/data/local/tmp/%s", file_name);

	result =  elan_parse_csvfile(file_path, target_name, data, rows, columns);
	if (!result)
		printk("[elan] Get threshold successed form csvfile\n");
	else
		printk("[elan] Get threshold failed form csvfile\n");
	
	return result;
}


static int elan_mt_send_get_data(struct elan_ts_data *ts, const uint8_t *wbuf,
		size_t wsize, uint8_t *rbuf, size_t rsize)
{
	int ret = ACK_OK;

	if (!wbuf || !rbuf)
		return ERR_BUFEMPTY; //invalid buf

	ret = ts->ops->send(wbuf, wsize);
	if (ret != wsize) 
		return ERR_I2CWRITE; //iic transfer error

	ret = ts->ops->poll();
	if (ret)
		return ERR_I2CPOLL;//timeout;
	
	ret = ts->ops->recv(rbuf, rsize);
	if (ret != rsize)
		return ERR_I2CREAD; //iic transfer error
	
	return ACK_OK;
}

static int disable_finger_report(struct elan_ts_data *ts, int mode)
{
	const uint8_t check_report_mode[HID_CMD_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xCA,0x00,0x01};
	uint8_t set_finger_cmd[HID_CMD_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xCA,0x00,0x01};
	uint8_t rbuf[67] = {0x00};
	int retry = 3;
	int ret = ACK_OK;
	
	/*read current report mode : disable->buf[]*/
	ret = elan_mt_send_get_data(ts, check_report_mode, sizeof(check_report_mode), rbuf, sizeof(rbuf));
	if (ret) 
		return ERR_DEVACK;
	else
		printk("[elan]\t\t\t%s report mode:%s\n",__func__,rbuf[7] == 0x00 ? "Enable Report":"Disable Report");

	if (mode == rbuf[7]) {
		printk("[elan]\t\t%s needn't seting report mode, current mode = %s\n",__func__, mode == 0? "Enable Report": "Disable Report");
		return ACK_OK;
	} else {

retry_set:
		set_finger_cmd[10] = mode; 
		ret = ts->ops->send(set_finger_cmd, sizeof(set_finger_cmd));
		if (ret != sizeof(set_finger_cmd)) {
			return ERR_I2CWRITE;
		} else {
			ret = elan_mt_send_get_data(ts, check_report_mode, sizeof(check_report_mode), rbuf, sizeof(rbuf));
			if (ret) {
				if ((retry--) < 0)
					return ERR_DEVACK;
				else 
					goto retry_set;
			} else {
				if (mode == rbuf[7]) {
					return ACK_OK;
				} else {
					if ((retry--) < 0) {
						printk("[elan]\t\t%s try set report mode failed retry count out of range\n",__func__);
						return -1;
					} else {
						goto retry_set;
					}
				}
			}
		}
	}

}

static int alloc_data_buf(struct elan_ts_data *ts)
{

	if ((!ts) || (ts->fw_info.rx <= 0) || (ts->fw_info.tx <= 0)) {
		return ERR_DEVSTATUS;
	}

	g_rawdatabuf = (uint8_t *)kzalloc(sizeof(uint8_t) * (ts->fw_info.tx * ts->fw_info.rx * 2), GFP_KERNEL);
	if (!g_rawdatabuf)
		return ERR_BUFFALLOC;

	g_totlebuf = (short *)kzalloc(sizeof(short) * (ts->fw_info.tx * ts->fw_info.rx), GFP_KERNEL);
	if (!g_totlebuf)
		return ERR_BUFFALLOC;

	paramter = kzalloc(sizeof(struct dynamic_anang_para), GFP_KERNEL);
	if (!paramter)
		return ERR_BUFFALLOC;

	return ACK_OK;
}


static int elan_mt_disable_algorithm(struct elan_ts_data *ts)
{
	int ret = ACK_OK;
	uint8_t rbuf[67] = {0x00};
	const uint8_t cmd_algorithm1[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xCA,0x00,0x01};
	uint8_t cmd_algorithm2[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xB1,0x00,0x01};
	uint8_t cmd_algorithm3[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xC1,0x00,0x01};

	/*disable algorithm1*/
	printk("[elan]\t\t\t%s stage1: send disable algorithm1 command\n",__func__);
	ret = ts->ops->send(cmd_algorithm1, sizeof(cmd_algorithm1));
	if (ret != sizeof(cmd_algorithm1))
		return ERR_I2CWRITE;//goto err_algorithm1;

	/*disable algorithm2*/
	printk("[elan]\t\t\t%s stage1: send read algorithm2 and set disable algorithm2\n",__func__);
	ret = elan_mt_send_get_data(ts, cmd_algorithm2, sizeof(cmd_algorithm2), rbuf, sizeof(rbuf));
	if (ret)
		return ERR_DEVACK; //goto err_gorithm2;
	else {
		cmd_algorithm2[7] = 0x54;
		cmd_algorithm2[9] = ((((rbuf[6] << 8) | rbuf[7]) & 0xfebf) >> 8);
		cmd_algorithm2[10] =((((rbuf[6] << 8) | rbuf[7]) & 0xfebf) & 0xff);
		
		ret = ts->ops->send(cmd_algorithm2,sizeof(cmd_algorithm2));
		if (ret != sizeof(cmd_algorithm2))
			return ERR_I2CWRITE;//goto err_algorithm2;
	}

	/*disable algorithm3*/
	printk("[elan]\t\t\t%s stage1: send read algorithm3 and set disable algorithm3\n",__func__);
	ret = elan_mt_send_get_data(ts, cmd_algorithm3, sizeof(cmd_algorithm3), rbuf, sizeof(rbuf));
	if (ret)
		return ERR_DEVACK;//goto err_algorithm3;
	else {
		cmd_algorithm3[7] = 0x54;
		cmd_algorithm3[9] = ((((rbuf[6] << 8) | rbuf[7]) & 0xedad) >> 8);
		cmd_algorithm3[10] = ((((rbuf[6] << 8) | rbuf[7]) & 0xedad) & 0xff);

		ret = ts->ops->send(cmd_algorithm3, sizeof(cmd_algorithm3));
		if (ret != sizeof(cmd_algorithm3))
			return ERR_I2CWRITE;//goto err_algorithm3;
		else
			return ACK_OK;//ret = ACK_OK;
	}
	
//	return ret;

//err_algorithm1:
//err_algorithm2:
//err_algorithm3:
//	return ret;
}


static int elan_mt_read_dynamic_anaglcgpara(struct elan_ts_data *ts)
{
	const uint8_t cmd_ph1[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xC5,0x00,0x01};
	const uint8_t cmd_ph2[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xC6,0x00,0x01};
	const uint8_t cmd_ph3[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xC7,0x00,0x01};
	int ret = ACK_OK;
	uint8_t rbuf[67] = {0x00};


	/*read ph1 Parameter*/
	printk("[elan]\t\t\t%s stage1: read original ph1\n",__func__);
	ret = elan_mt_send_get_data(ts, cmd_ph1, sizeof(cmd_ph1), rbuf, sizeof(rbuf));
	if (ret) 
		return ERR_DEVACK;//goto err_ph1;
	else {
		paramter->origph1 = rbuf[7];
		printk("[elan]\t\t\t%s ph1:0x%2x\n",__func__,paramter->origph1);
	}

	/*read ph2 Parameter*/
	printk("[elan]\t\t\t%s stage1: read original ph2\n",__func__);
	ret = elan_mt_send_get_data(ts, cmd_ph2, sizeof(cmd_ph2), rbuf, sizeof(rbuf));
	if (ret)
		return ERR_DEVACK;//goto err_ph2;
	else {
		paramter->origph2 = rbuf[7];
		printk("[elan]\t\t\t%s ph2:0x%2x\n",__func__,paramter->origph2);
	}
		
	/*read ph3 Parameter*/
	printk("[elan] \t\t\t%s stage1: read original ph3\n",__func__);
	ret = elan_mt_send_get_data(ts, cmd_ph3, sizeof(cmd_ph3), rbuf, sizeof(rbuf));
	if (ret)
		return ERR_DEVACK;//goto err_ph3;
	else {
		paramter->origph3 = rbuf[7];
		printk("[elan]\t\t\t%s ph3:0x%2x\n",__func__,paramter->origph3);
		ret = ACK_OK;
	}

	return ret;

//err_ph1:
//err_ph2:
//err_ph3:
//	return ret;

}

static int test_mode(struct elan_ts_data *ts, bool on)
{
	const uint8_t enter_mode[37]	= {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x55,0x55,0x55,0x55};
	const uint8_t exit_mode[37]		= {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0xA5,0xA5,0xA5,0xA5};
	const uint8_t get_mode[37]		= {0x04,0x00,0x23,0x00,0x03,0x00,0x06,0x96,0x00,0x00,0x00,0x00,0x01};
	int ret = 0, status = 0;
	uint8_t rbuf[67] = {0x00};

	if (on) {
		ret = ts->ops->send(enter_mode, sizeof(enter_mode));
		if (ret != sizeof(enter_mode))
			goto err_set_mode;
		printk("[elan]\t\t\t\t%s enter test mode\n",__func__);

	} else {
		ret = ts->ops->send(exit_mode, sizeof(exit_mode));
		if (ret != sizeof(exit_mode))
			goto err_set_mode;
		printk("[elan]\t\t\t\t%s exit test mode\n",__func__);
	}
	
	mdelay(10);
	
	ret = elan_mt_send_get_data(ts, get_mode, sizeof(get_mode), rbuf, sizeof(rbuf));
//	printk("[elan] get  mode ret = %d,buf[7] = %2x\n",ret,rbuf[7]);
	if (ret)
		goto err_get_mode;
	else {
		status = ((rbuf[7] & 0x80) >> 7);
		if(on) {
			if (!status) {
				printk("[elan]\t\t\t\t%s enter test mode success!!\n", __func__);
				ret = 0;
			} else {
				printk("[elan]\t\t\t\t%s enter test mode falied!!\n", __func__);
				ret = -1;
			}
		} else if (!on) {
			if (status) {
				printk("[elan]\t\t\t\t%s exit test mode success!!\n", __func__);
				ret = 0;
			} else {
				printk("[elan]\t\t\t\t%s exit test mode failed!!\n", __func__);
				ret = -1;
			}
		}
	}

	return ret;

err_get_mode:
err_set_mode:

	return ret;

}	


static int elan_get_raw_data(struct elan_ts_data *ts, int len, bool checkdata)
{
	int ret = 0;
	uint8_t data[67] = {0x00};
	int recv_num = len / 60 + ((len % 60) != 0); //60 byte per times
	int i = 0, j = 0;

	if (!g_totlebuf || !g_rawdatabuf) {
		return -EINVAL;
	}

	for (; i < recv_num; i++) {
		ret = ts->ops->recv(data,sizeof(data));
		if(ret != sizeof(data))
			return -EINVAL;

		if(checkdata) {
			if ((i == (recv_num - 1)) && ((len % 60) != 0)) {
				memcpy(g_rawdatabuf + 60 * i, data + 7, len % 60);
			} else {
				memcpy(g_rawdatabuf + 60 * i, data + 7, 60);
			}
		}
	}

	if (checkdata) {
		for (i = 0; i < len; i = i + 2) {
			g_totlebuf[j] = (g_rawdatabuf[i] << 8) | g_rawdatabuf[i + 1];
			j++;
		}	
	}

	return 0;
}

static int enter_get_data_mode(struct elan_ts_data *ts, int framenum)
{
	int ret = ACK_OK;
	const uint8_t data_mode_mode[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04, 0x54, 0xCD, 0x80, framenum};
	uint8_t mode_ok[4] = {0x42, 0x41, 0x53, 0x45};
	uint8_t rbuf[67] = {0x00};

	
	ret = elan_mt_send_get_data(ts,data_mode_mode,sizeof(data_mode_mode), rbuf, sizeof(rbuf));
	if (ret)
		return ERR_DEVACK;//return -EINVAL;

//	printk("[elan]\t\t\t%s recv %2x:%2x:%2x:%2x\n", __func__,rbuf[4],rbuf[5],rbuf[6],rbuf[7]);
	if(memcmp(rbuf + 4 ,mode_ok , sizeof(mode_ok)))
		return ERR_DEVACK;//return -EINVAL;
	else
		return ACK_OK;
}

/*adc data is test whwen check is true, otherwise is skipftame*/
static int get_normal_adc_data(struct elan_ts_data *ts, bool checkdata)
{
	int ret = ACK_OK;
	int datalength = 0;
	uint8_t nor_adc_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x06,0x58,0x02,0x00,0x00,0x00,0x00};

	int rx = ts->fw_info.rx > ts->fw_info.tx ? ts->fw_info.rx : ts->fw_info.tx;
	int tx = ts->fw_info.tx > ts->fw_info.rx ? ts->fw_info.rx : ts->fw_info.tx;
	int j = 0, i =0;
	datalength = ts->fw_info.tx * ts->fw_info.rx * 2;
	nor_adc_cmd[11] = (datalength & 0xff00) >> 8;
	nor_adc_cmd[12] = datalength & 0x00ff;

	/*step1: enter test mode and test*/
	printk("[elan]\t\t\t %s  enter test mode and test\n",__func__);
	ret = test_mode(ts,true);
	if (ret) {
		printk("[elan]\t\t\t%s enter test mode and test failed\n",__func__);
		return ERR_TESTMODE;//return ret;
	}
	/*step2: send get notmal adc command*/
	printk("[elan]\t\t\t%s send get nolmal adc command\n", __func__);
	ret = ts->ops->send(nor_adc_cmd, sizeof(nor_adc_cmd));
	if (ret != sizeof(nor_adc_cmd)) {
		printk("[elan] \t\t\t%s send get nolmal adc command failed\n", __func__);
		return ERR_I2CWRITE;//return -EINVAL;
	}

	ret = ts->ops->poll();
	if (ret) {
		printk("[elan] \t\t\t%s wait interrupt low failed!!!\n",__func__);
		return ERR_I2CPOLL;
	}

	/*step 3: recv normal data*/
	printk("[elan]\t\t\t%s enter recv raw data, frame-mode=%s\n", __func__, checkdata == true?"store":"skip");
	ret = elan_get_raw_data(ts,datalength,checkdata);
	if (ret){
		printk("[elan]\t\t\t%s enter recv raw data, frame-mode=%s  failed\n", __func__, checkdata == true?"store":"skip");
		return ERR_GETRAWDATA;//return ret;
	}

	/*step 4: exit test mode and test*/
	printk("[elan]\t\t\t%s exit test mode and test\n", __func__);
	ret = test_mode(ts, false);
	if (ret) {
		printk("[elan]\t\t\t%s exit test mode and test failed\n", __func__);
		return ERR_TESTMODE;//return ret;
	}

	if (checkdata) {
		printk("raw data:\n");
		for (j = 0; j < tx; j++) {
			for (i = 0; i < rx - 1; i++)
				printk("%d ", g_totlebuf[i+j*rx]);
			printk("\n");
		}
	}

	//return ret;
	return ACK_OK;
}

static int elan_mt_get_normal_adc(struct elan_ts_data *ts)
{
	int ret = ACK_OK;
	static int count = 2;
	int trycount = 3;
	/*step1: enter get data mode, skip frame*/
	printk("[elan]\t\t%s stage%d: enter NormalADCSkipFrame mode\n", __func__,count);
try:
	ret = enter_get_data_mode(ts,2);//skip frame number = 2
	if (ret) {
		printk("[elan]\t\t%s stage%d: enter NormalADCSkipFrame mode failed retry count =%d \n", __func__,count, 2-trycount);
		while(trycount--)
			goto try;
		if(count++ > 4)
			count = 2;
		return ERR_SKIPFRAME;
	}
	/*step2: get notmal adc data false*/
	printk("[elan]\t\t%s stage%d: get Normal ADC Data don't store\n",__func__, count);
	ret = get_normal_adc_data(ts, false);
	if (ret) {
		printk("[elan]\t\t%s stage%d: get Normal ADC Data don't store failed\n",__func__,count);
		if (count++ > 4)
			count = 2;
		return ERR_GETNORMALADC; //ret;
	}

	/*step 3: enter get data mode, test frame*/
	printk("[elan]\t\t%s stage%d: enter NormalADCTestFrame mode\n",__func__,count);
	ret = enter_get_data_mode(ts,3); //test frame number =3
	if (ret) {
		printk("[elan]\t\t%s stage%d: enter NormalADCTestFrame mode failed\n",__func__,count);
		if (count++ > 4)
			count = 2;
		return ERR_TESTFRAME;
	}


	/*step 4 get normal adc data true*/
	printk("[elan]\t\t%s stage%d: get Normal ADC Data and store\n", __func__,count);
	ret = get_normal_adc_data(ts,true);
	if (ret) {
		printk("[elan]\t\t%s stage%d: get Normal ADC Data and store failed\n", __func__,count);
		if (count++ > 4)
			count = 2;
		return ERR_GETNORMALADC;
	}
	
	if (count++ > 4)
		count = 2;

	return ACK_OK;
}

static int rx_open_test(struct elan_ts_data *ts)
{
	int i = 0;
	int j = 0;
	int over_hb = 0;
	int edge_over_hb = 0;
	int csv_rx_over_hbpt_ratio = 0;
	int csv_rx_edge_trace_ratio = 0;
	int ret = 0;//NO_ERR;
	int res = 0;//NO_ERR;
	// remember rx must binger than tx
	int rx = ts->fw_info.rx > ts->fw_info.tx ? ts->fw_info.rx : ts->fw_info.tx;
	int tx = ts->fw_info.tx > ts->fw_info.rx ? ts->fw_info.rx : ts->fw_info.tx;
	u32 data_buf[2] = {0};
	u32 *csv_rx_differ_hb = NULL;
	short (*databuf)[rx] = (short (*)[rx])g_totlebuf;
	short (*rx_diff_data)[rx - 1] = NULL;
	short *rx_diff_buff = (short *)kzalloc(sizeof(short) * tx * (rx - 1), GFP_KERNEL);
	
	if (!rx_diff_buff) {
		printk("[elan]:alloc rx_diff_buff fail\n");
		return -EINVAL;
	}
	
	rx_diff_data = (short (*)[rx - 1])rx_diff_buff;
	csv_rx_differ_hb = kzalloc(sizeof(u32) * (rx - 1) * tx, GFP_KERNEL);
	if (!csv_rx_differ_hb) {
		printk("[elan]:%s csv_rx_differ_hb buf is null!\n", __func__);
		kfree(rx_diff_data);
		rx_diff_data = NULL;
		return -EINVAL;
	}
	
	if (!g_totlebuf) {
		printk("[elan]:%s,data buf is null\n", __func__);
		kfree(rx_diff_data);
		rx_diff_data = NULL;
		kfree(csv_rx_differ_hb);
		csv_rx_differ_hb = NULL;
		return -EINVAL;
	}

	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, rx - 1, tx, "rx_diff_hb"/*RX_DIFFER_HB*/, csv_rx_differ_hb);
	if (res) {
		printk("get RxDifferHB err\n");
		ret = -EINVAL;
		goto rx_open_test_exit;
	}

	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, 1, 1, "rx_over_hbpt_ratio"/*RX_OVER_HBPT_RATIO*/, data_buf);
	if (res) {
		printk("get RxOverHBPTRatio err\n");
		ret = -EINVAL;
		goto rx_open_test_exit;
	}
	csv_rx_over_hbpt_ratio = data_buf[0]; //12
	printk("[elan]:csv_rx_over_hbpt_ratio = %d\n", csv_rx_over_hbpt_ratio);

	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, 1, 1, "rx_edge_trace_count"/*RX_EDGE_TRACE_COUNT*/, data_buf);
	if (res) {
		printk("get RxEdgeTraceCount err\n");
		ret = -EINVAL;
		goto rx_open_test_exit;
	}
	csv_rx_edge_trace_ratio = data_buf[0]; //1
	printk("[elan]:csv_rx_edge_trace_ratio = %d\n", csv_rx_edge_trace_ratio);

	for (i = 0; i < tx; i++) {
		for (j = 0; j < rx - 1; j++) {
			if (databuf[i][j] < 0 || databuf[i][j + 1] < 0) {
				printk("[elan]:%s,databuf[%d][%d]=%d,databuf[%d][%d]=%d\n", \
						__func__, i, j, databuf[i][j], i + 1, j, databuf[i + 1][j]);
				ret = -EINVAL;
				goto rx_open_test_exit;
			}

			rx_diff_data[i][j] = abs(databuf[i][j] - databuf[i][j + 1]);
			if (!((j >= csv_rx_edge_trace_ratio) && (j <= rx - 2 - csv_rx_edge_trace_ratio) &&
						(i >= csv_rx_edge_trace_ratio) && (i <= tx - 1 - csv_rx_edge_trace_ratio))) { // rx-2 is edge
				if (rx_diff_data[i][j] > (databuf[i][j] * csv_rx_differ_hb[j + (rx - 1) * i] / 100)) {
					printk("[elan over_hb]:i=%d,j=%d\n", i, j);
					edge_over_hb++;
				}
			} else {
				if (rx_diff_data[i][j] > (databuf[i][j] * csv_rx_differ_hb[j + (rx - 1) * i] / 100)) {
					printk("[elan over_hb]:i=%d,j=%d\n", i, j);
					over_hb++;
				}
			}
		}
	}

	if (1)
		print_rawdata(rx - 1, tx, RXTX_DIFF_PRINT, rx_diff_data);


	printk("[elan]:RxDiffData edge_over_hb=%d,over_hb=%d\n", edge_over_hb, over_hb);
	if ((edge_over_hb > tx * csv_rx_over_hbpt_ratio / 100) ||
			(over_hb > tx * csv_rx_over_hbpt_ratio / 100)) {
		printk("[elan]:edge_over_hb is over spec!\n");
		ret = -EINVAL;
	}

rx_open_test_exit:
	if (rx_diff_data) {
		kfree(rx_diff_data);
		rx_diff_data = NULL;
	}
	
	if (csv_rx_differ_hb) {
		kfree(csv_rx_differ_hb);
		csv_rx_differ_hb = NULL;
	}
		
	return ret;
}

/*stage 3: start*/
static int elan_set_read_ph(struct elan_ts_data *ts, int ph1, int ph2, int ph3)
{
	int ret = 0;
	uint8_t set_ph1_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xC5,0x00,ph1}; 
	uint8_t set_ph2_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xC6,0x00,ph2}; 
	uint8_t set_ph3_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xC7,0x00,ph3}; 
	const uint8_t settpparameter_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x2D,0x00,0x01};
	uint8_t rbuf[67] = {0x00};
	/*set ph1*/
	ret = ts->ops->send(set_ph1_cmd,sizeof(set_ph1_cmd));
	if (ret != sizeof(set_ph1_cmd)) {
		ret = -EINVAL;
		printk("[elan] \t\t%s set ph1 failed\n", __func__);
		goto err_set_ph1;
	}

	/*set ph2*/
	ret = ts->ops->send(set_ph2_cmd,sizeof(set_ph2_cmd));
	if (ret != sizeof(set_ph2_cmd)) {
		ret = -EINVAL;
		printk("[elan] \t\t%s set ph2 failed\n", __func__);
		goto err_set_ph2;
	}
	
	/*set ph3*/
	ret = ts->ops->send(set_ph3_cmd, sizeof(set_ph3_cmd));
	if(ret != sizeof(set_ph3_cmd)) {
		ret = -EINVAL;
		printk("[elan] \t\t%s set ph3 failed\n", __func__);
		goto err_set_ph3;
	}
	
	/*set tpparameter*/
	ret = ts->ops->send(settpparameter_cmd, sizeof(settpparameter_cmd));
	if (ret != sizeof(settpparameter_cmd)) {
		ret = -EINVAL;
		goto err_set_tpparameter;
	}

	mdelay(300);
	set_ph1_cmd[7] = 0x53;
	set_ph2_cmd[7] = 0x53;
	set_ph3_cmd[7] = 0x53;

	
	ret = elan_mt_send_get_data(ts, set_ph1_cmd, sizeof(set_ph1_cmd), rbuf, sizeof(rbuf));
	if (ret || (rbuf[7] != ph1)) {
		return -EINVAL;
	}
	
	printk("[elan] \t\t%s stage3 ph1:rbuf[7] = %2x:%2x\n", __func__, ph1,rbuf[7]);

	ret = elan_mt_send_get_data(ts, set_ph2_cmd, sizeof(set_ph2_cmd), rbuf, sizeof(rbuf));
	if (ret || (rbuf[7] != ph2))
		return -EINVAL;
		
	printk("[elan] \t\t%s stage3 ph2:rbuf[7] = %2x:%2x\n", __func__, ph2,rbuf[7]);
	
	ret = elan_mt_send_get_data(ts, set_ph3_cmd, sizeof(set_ph3_cmd), rbuf, sizeof(rbuf));
	if (ret || (rbuf[7] != ph3))
		return -EINVAL;

	printk("[elan] \t\t%s stage3 ph3:rbuf[7] = %2x:%2x\n", __func__, ph3,rbuf[7]);


	ret = 0;
	return ret;
	
//err_read_dynamic_phx:
err_set_tpparameter:
err_set_ph3:
err_set_ph2:	
err_set_ph1:
	return ret;
}

static int adc_mean_lowboundary_test(struct elan_ts_data *ts)
{
	int i=0,meauAdc=0,totleAdc=0;
	int adc_lb=0;
	int ret=0;
	int len = (ts->fw_info.rx) * (ts->fw_info.tx);
	u32 data_buf[2]={0};


	ret = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, 1, 1, "adc_lb", data_buf);
	if ( ret ) {
		printk("[elan] \t\t%s get ADCLB err\n", __func__);
		return -EINVAL;
	}

	adc_lb = data_buf[0];
	printk("[elan]:adc_lb = %d\n",adc_lb);
	for(i = 0;i < len; i++)
	{
		totleAdc += g_totlebuf[i];
	}
	
	if (len > 0) {
		meauAdc = totleAdc / len;
		printk("[elan]:meau adc=%d\n",meauAdc);
		if(meauAdc < adc_lb){
			printk("[elan]:meauAdc<%d\n",adc_lb);
			return -EINVAL;
		}
	} else {
		printk("[elan]:data len is no right!\n");
		return -EINVAL;
	}
		
	return ret;
}

/*stage3: end*/

/*stage4: start*/
static int elan_tx_open_test(struct elan_ts_data *ts)
{
	int i = 0;
	int j = 0;
	int over_hb = 0;
	int edge_over_hb = 0;
	int csv_tx_over_hbpt_ratio = 0;
	int csv_tx_edge_trace_count = 0;
	u32 data_buf[2] = {0};
	u32 *csv_tx_diff_hb = NULL;
	int ret = 0;
	int res = 0;
	const int rx = ts->fw_info.rx > ts->fw_info.tx ? ts->fw_info.rx : ts->fw_info.tx;
	const int tx = ts->fw_info.tx > ts->fw_info.rx ? ts->fw_info.rx : ts->fw_info.tx;
	short (*databuf)[rx] = (short (*)[rx])g_totlebuf;
	short (*tx_diff_data)[rx] = NULL;
	short *tx_diff_buff = (short *)kzalloc(sizeof(short) * (tx - 1) * rx, GFP_KERNEL);

	if (!tx_diff_buff) {
		printk("[elan]:%s tx_diff_buff is null!\n", __func__);
		return -EINVAL;
	}
	tx_diff_data = (short (*)[rx])tx_diff_buff;

	csv_tx_diff_hb = kzalloc(sizeof(u32) * (tx - 1) * rx, GFP_KERNEL);
	if (!csv_tx_diff_hb) {
		printk("[elan]:%s csv_tx_diff_hb buf is null!\n", __func__);
		kfree(tx_diff_data);
		tx_diff_data = NULL;
		return -EINVAL;
	}

	if (!g_totlebuf) {
		printk("[elan]:%s data buf is null!\n", __func__);
		kfree(tx_diff_data);
		tx_diff_data = NULL;
		kfree(csv_tx_diff_hb);
		csv_tx_diff_hb = NULL;
		return -EINVAL;
	}

	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, rx, tx - 1,  "tx_diff_hb", csv_tx_diff_hb);
	if (res) {
		printk("get TX_DIFFER_HB err\n");
		ret = -EINVAL;
		goto tx_open_test_exit;
	}
	
	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, 1, 1, "tx_over_hbpt_ratio", data_buf);
	if (res) {
		printk("get TxOverHBPTRatio err\n");
		ret = -EINVAL;
		goto tx_open_test_exit;
	}
	csv_tx_over_hbpt_ratio = data_buf[0];
	printk("[elan]:csv_tx_over_hbpt_ratio = %d\n", csv_tx_over_hbpt_ratio);


	res = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, 1, 1,  "tx_edge_trace_Count", data_buf);
	if (res) {
		printk("get TX_EDGE_TRACE_COUNT err\n");
		ret = -EINVAL;
		goto tx_open_test_exit;
	}
	csv_tx_edge_trace_count = data_buf[0];
	printk("[elan]:csv_tx_edge_trace_count = %d\n", csv_tx_edge_trace_count);

	for (i = 0; i < tx - 1; i++) {
		for (j = 0; j < rx; j++) {
			if (databuf[i][j] < 0 || databuf[i + 1][j] < 0) {
				printk("[elan]:%s,databuf[%d][%d]=%d,databuf[%d][%d]=%d\n", \
						__func__, i, j, databuf[i][j], i + 1, j, databuf[i + 1][j]);
				ret = -EINVAL;
				goto tx_open_test_exit;
			}
			tx_diff_data[i][j] = databuf[i][j] - databuf[i + 1][j];
			if (!((j >= csv_tx_edge_trace_count) && (j <= rx - 1 - csv_tx_edge_trace_count) &&
						(i >= csv_tx_edge_trace_count) && (i <= tx - 2 - csv_tx_edge_trace_count))) { // tx-2 is edge
				if (abs(tx_diff_data[i][j]) > (databuf[i][j]*csv_tx_diff_hb[j + rx * i] / 100)) {
					printk("[over_hbelan]:Edge====i=%d,j=%d,tx_diff_data=%d\n", i, j, tx_diff_data[i][j]);
					edge_over_hb++;
				}
			} else {
				if (abs(tx_diff_data[i][j]) > (databuf[i][j]*csv_tx_diff_hb[j + rx * i] / 100)) {
					printk("[over_hbelan]:====i=%d,j=%d,tx_diff_data=%d\n", i, j, tx_diff_data[i][j]);
					over_hb++;
				}
			}
		}
	}

	if(1)
		print_rawdata(rx, tx-1, RXTX_DIFF_PRINT, tx_diff_data);



	printk("[elan]:TxDiffData edge_over_hb=%d,over_hb=%d\n", edge_over_hb, over_hb);
	if ((edge_over_hb > (rx * csv_tx_over_hbpt_ratio / 100)) ||
			(over_hb > rx * csv_tx_over_hbpt_ratio / 100)) {
		printk("[elan]:edge_over_hb is over spec!\n");
		ret = -EINVAL;
		goto tx_open_test_exit;
	}

tx_open_test_exit:
	if (tx_diff_data) {
		kfree(tx_diff_data);
		tx_diff_data = NULL;
	}
	
	if (csv_tx_diff_hb) {
		kfree(csv_tx_diff_hb);
		csv_tx_diff_hb = NULL;
	}
	
	return ret;
}
/*stage4: end*/

/*stage5: start*/
static int txrx_short_data(struct elan_ts_data *ts, int shortdata_mode, int framenum, bool checkdata)
{
	int ret = ACK_OK;
	int datalen = ts->fw_info.rx * ts->fw_info.tx * 2;
	int rx = ts->fw_info.rx > ts->fw_info.tx ? ts->fw_info.rx : ts->fw_info.tx;
	int tx = ts->fw_info.tx > ts->fw_info.rx ? ts->fw_info.rx : ts->fw_info.tx;
	uint8_t get_data_mode[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x34,0x80,(uint8_t)shortdata_mode|(uint8_t)framenum};
	uint8_t get_txrx_short_data[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x06,0x58,0x01,0x00,0x00, (datalen & 0xff00) >> 8, datalen & 0xff};
	uint8_t dummy_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x06,0x58,0xC2,0x00,0x00,0x00,0x00};


	/*step 1: send enter get data mode*/
	ret = ts->ops->send(get_data_mode, sizeof(get_data_mode));
	if (ret != sizeof(get_data_mode)) {
		printk("[elan]\t\t\t%s send enter get data %s frame mode failed!!\n", __func__, framenum == 0x02 ? "skip":"test" );
		return ERR_I2CWRITE;//return -EINVAL;
	} else 
		printk("[elan]\t\t\t%s send enter get data %s frame mode success!!\n", __func__, framenum == 0x02 ? "skip":"test" );


	/*step 2: enter test mode and test*/
	ret = test_mode(ts,true);
	if (ret) {
		printk("[elan]\t\t\t%s enter test mode and test result: failed!!\n", __func__);
		return ERR_TESTMODE;//return -EINVAL;
	} else 
		printk("[elan]\t\t\t%s enter test mode and test result: successs!!\n", __func__);
	
	/*step 3: checkdata */
	ret = ts->ops->send(get_txrx_short_data, sizeof(get_txrx_short_data));
	if (ret != sizeof(get_txrx_short_data)) {
		printk("[elan]\t\t\t%s send get rxtx %s data failed!!\n", __func__, shortdata_mode == 0xC0 ? "short":"short2");
		return ERR_I2CWRITE;//return -EINVAL;
	} else 
		printk("[elan]\t\t\t%s send get rxtx %s data success!!\n", __func__, shortdata_mode == 0xC0 ? "short":"short2");

	
	ret = ts->ops->poll();
	if (ret) {
		printk("[elan]\t\t\t%s wait interrupt low failed\n", __func__);
		return ERR_I2CPOLL;
	}
	/*step 4: get short rxtx data*/
	ret = elan_get_raw_data(ts,datalen,checkdata);
	if (ret) {
		printk("[elan]\t\t\t%s get short rxtx data %s-%sfailed!!\n",\
				__func__, framenum == 0x02 ? "skip":"test",  shortdata_mode == 0xC0 ? "short":"short2");
		return ERR_GETRAWDATA;//return -EINVAL;
	}else
		printk("[elan]\t\t\t%s get short rxtx data %s-%s success\n",\
				__func__, framenum == 0x02 ? "skip":"test",  shortdata_mode == 0xC0 ? "short":"short2");

	/*step 5: send dummy command*/
	ret = ts->ops->send(dummy_cmd, sizeof(dummy_cmd));
	if (ret != sizeof(dummy_cmd)) {
		printk("[elan]\t\t\t%s send dummy command failed!!\n", __func__);
		return ERR_I2CWRITE;//return -EINVAL;
	} else
		printk("[elan]\t\t\t%s send dummy command success!!\n", __func__);
	

	/*step 6: exit mode and test*/
	ret = test_mode(ts,false);
	if (ret) {
		printk("[elan]\t\t\t%s exit test mode and test result: failed!!\n",__func__);
		return ERR_TESTMODE;//return -EINVAL;
	}else
		printk("[elan]\t\t\t%s exit test mode and test result: succcess!!\n",__func__);

	if (checkdata) {	
		printk("%s data:\n",shortdata_mode == 0xC0 ? "short":"short2");
		if (shortdata_mode == 0xC0) {
			g_test_adc_databuf = (short *)kzalloc(sizeof(short) * (ts->fw_info.tx + ts->fw_info.rx), GFP_KERNEL);
			memcpy(g_test_adc_databuf, g_totlebuf, sizeof(short) * (tx + rx));
		} else {
			g_test_adc_databuf2 = (short *)kzalloc(sizeof(short) * (ts->fw_info.tx + ts->fw_info.rx), GFP_KERNEL);
			memcpy(g_test_adc_databuf2, g_totlebuf, sizeof(short) * (tx + rx));
		}
		print_rawdata(rx, tx, SHORT_DATA_PRINT, NULL);
	}
	
	return ACK_OK;
}

static int enter_short_mode(struct elan_ts_data *ts)
{
	int ret = ACK_OK;
	const uint8_t short_mode_cmd[37] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x33,0x10,0x00};


	ret = ts->ops->send(short_mode_cmd, sizeof(short_mode_cmd));
	if (ret != sizeof(short_mode_cmd)) {
		printk("[elan] \t\t%s enter mode failed\n", __func__);
		return ERR_I2CWRITE;//return -EINVAL;
	} else {
		printk("[elan] \t\t%s enter mode sucesss\n", __func__);
		ret = ACK_OK;
	}

	return ret;
}


static int txrx_short_get_data(struct elan_ts_data *ts)
{
	int ret = ACK_OK;

	/*step 1: get short data: 0xC0:shor data, 0x02 skip frame, false free data*/
	ret = txrx_short_data(ts, 0xC0, 0x02, false);
	if (ret) {
		printk("[elan]\t\t\t\t%s skip frame and get short data failed\n",__func__);
		return ERR_SHORTDATA;
	}
	/*step 2: get short data: 0xC0:shor data, 0x02 test frame, false free data*/
	ret = txrx_short_data(ts, 0xC0, 0x03, true);
	if (ret) {
		printk("[elan]\t\t\t\t%s test frame and get short data failed\n",__func__);
		return ERR_SHORTDATA;
	}
	/*step 3: get short data: 0xC0:shor data, 0x02 skip frame, false free data*/
	ret = txrx_short_data(ts, 0x40, 0x02, false);
	if (ret) {
		printk("[elan]\t\t\t\t%s skip frame and get short data 2 failed\n",__func__);
		return ERR_SHORTDATA;
	}

	/*step 4: get short data: 0xC0:shor data, 0x02 test frame, false free data*/
	ret = txrx_short_data(ts ,0x40, 0x03, true);
	if (ret) {
		printk("[elan]\t\t\t\t%s test frame and get short data 2 failed\n",__func__);
		return ERR_SHORTDATA;
	}

	return ACK_OK;
}

static int rxtx_short_test(struct elan_ts_data *ts)
{
	int i = 0;
	int ret = 0;
	int over_hb = 0;
	bool txrx_short_test_result = true;
	int rx = ts->fw_info.rx > ts->fw_info.tx ? ts->fw_info.rx : ts->fw_info.tx;
	int tx = ts->fw_info.tx > ts->fw_info.rx ? ts->fw_info.rx : ts->fw_info.tx;
	u32 csv_tx_trace_adjust_differ_hb_cond1[55] = {0};
	u32 csv_rx_trace_adjust_differ_hb_cond1[55] = {0};
	short *differ_data = g_test_adc_databuf;

	if (!g_test_adc_databuf || !g_test_adc_databuf2 || ts->fw_info.rx <= 1 || ts->fw_info.tx <= 1) { // avoid tx -1 < 0
		printk("[elan]:%s,data buf is null\n", __func__);
		return -EINVAL;
	}

	ret = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, tx - 1, 1, "tx_trace_adjdiff_hb_cond1", csv_tx_trace_adjust_differ_hb_cond1);
	if (ret) {
		printk("get TX_TRACE_ADJ_DIFFER_HB_COND1 err\n");
		return -EINVAL;
	}
	
	ret = elan_get_threshold_from_csvfile(ts->fw_info.fw_id, rx - 1, 1, "rx_trace_adjdiff_hb_cond1", csv_rx_trace_adjust_differ_hb_cond1);
	if (ret) {
		printk("get RX_TRACE_ADJ_DIFFER_HB_COND1 err\n");
		return -EINVAL;
	}

	printk("[elan_mmi]:Rx diff:");
	for (i = 0; i < (rx + tx); i++) {
		if (g_test_adc_databuf[i] == 0 || g_test_adc_databuf2[i] == 0) {
			txrx_short_test_result = false;
			printk("[elan]:g_test_adc_databuf[%d]=%d,g_test_adc_databuf2[%d]=%d",
					i, g_test_adc_databuf[i], i, g_test_adc_databuf2[i]);
		}
		
		differ_data[i] = abs(g_test_adc_databuf[i] - g_test_adc_databuf2[i]);
		printk("%03d,", differ_data[i]);
		if (i == rx - 1) {
			printk("\n");
			printk("[elan_mmi]:Tx diff:");
		}
	}
	printk("\n");


	printk("[elan_mmi]:RX Short Test Adjacent Differ Data:");
	for (i = 0; i < (rx - 1); i++) {
		differ_data[i] = abs(differ_data[i] - differ_data[i + 1]);
		printk("%d,", differ_data[i]);
		if ((i > 0) && (i < rx - 2)) {
			if (differ_data[i] > (8192 * csv_rx_trace_adjust_differ_hb_cond1[i] / 100)) { // 100 is percent
				over_hb++;
				printk("[elan]:%s,over_hb=%d\n", __func__, over_hb);
			}
		} else {
			if (differ_data[i] > (8192 * csv_rx_trace_adjust_differ_hb_cond1[i] / 100)) { // 100 is percent
				over_hb++;
				printk("[elan]:%s,over_hb=%d\n", __func__, over_hb);
			}
		}
	}

	if (over_hb > 0) {
		printk("\n[elan]:RX Short Test Fail!\n");
		txrx_short_test_result = false;
	} else {
		printk("\n[elan]:RX Short Test Pass!\n");
	}

	over_hb = 0;
	printk("[elan_mmi]:TX Short Test Adjacent Differ Data:");
	for (i = rx; i < (rx + tx - 1); i++) {
		differ_data[i] = abs(differ_data[i] - differ_data[i + 1]);
		printk("%d,", differ_data[i]);
		if ((i > rx) && (i < rx + tx - 2)) {
			if ((differ_data[i] > 8192 * csv_tx_trace_adjust_differ_hb_cond1[i - rx] / 100)) {
				over_hb++;
			}
		} else {
			if (differ_data[i] > 8192 * csv_tx_trace_adjust_differ_hb_cond1[i - rx] / 100) {
				over_hb++;
			}
		}
	}

	if (over_hb > 0) {
		txrx_short_test_result = false;
	}

	if (!txrx_short_test_result) {
		printk("\n[elan]:TX Short Test Fail!\n");
		ret = -EINVAL;
	} else {
		printk("\n[elan]:TX Short Test Pass!\n");
		ret = 0;
	}

	return ret;

}
/*stage6: end*/


/*stage 1*/
static int tp_module_test_init(struct elan_ts_data *ts)
{
	int ret = ACK_OK;

	printk("[elan]\t\t===========Start Tp Module Initial============\n");
	elan_switch_irq(0);
	/*step 1: check fw information*/
	printk("[elan]\t\t%s stage1: check fw infomation\n\n", __func__);
	ret =  __fw_packet_handler(ts->client);
	if (ret)
		return ERR_DEVSTATUS;

	/*disable report*/
	ret = disable_finger_report(ts,1);
	if (ret)
		return ERR_DISABLEREPOR;

	/*step 2: alloc buff*/
	printk("[elan]\t\t%s stage1: alloc data buf\n\n",__func__);
	ret = alloc_data_buf(ts);
	if (ret)
		return ERR_BUFFALLOC;

	/*step 3: Diable Algorithm*/
	printk("[elan]\t\t%s stage1: Diable Algorithm\n\n",__func__);
	ret = elan_mt_disable_algorithm(ts);
	if (ret)
		return ERR_DISABLALG;

	/*step4:  Read Dynamic Analog Parameter*/
	printk("[elan]\t\t%s stage1: Read Dynamic Analog Parameter\n\n",__func__);
	ret = elan_mt_read_dynamic_anaglcgpara(ts);
	if (ret)
		return ERR_DYNAMICANALOG;///return ret;

	/*step 5: re-calibrete and delay 300ms*/
	printk("[elan]\t\t%s stage1: recalibrate\n\n",__func__);
	ret = elan_ts_calibrate(ts->client);
	if (ret)
		return ERR_RECALIBRATE;//return ret;
	mdelay(300);
	printk("[elan]\t\t===========End Tp Module Initial============\n");

	//return ret;
	return ACK_OK;
}
/*stage 2*/
static int elan_mt_rx_open_test(struct elan_ts_data *ts)
{
	int ret = ACK_OK;
	
	printk("[elan]===========Start Tp Rx Open Test============\n");
	/*step: get normal adc*/
	printk("[elan]\t\t%s stage2: Get Normal ADC\n\n", __func__);
	ret = elan_mt_get_normal_adc(ts);
	if (ret) {
		printk("[elan]\t\t%s stage2: Get Normal ADC failed\n", __func__);
		return ERR_NORMALADC;//return ret;
	}

	/*step 2: do rx  open test*/
	printk("[elan]\t\t%s stage2: do rx open test\n\n", __func__);
	ret = rx_open_test(ts);
	if (ret) {
		printk("[elan]\t\t%s stage2: do rx open test failed\n", __func__);
		return ERR_RXOPEN;//return ret;
	}

	printk("[elan]===========End Tp Rx Open Test============\n");

//	return ret;
	return ACK_OK;
}


/*stage 3: meaning value check*/
static int elan_mt_mean_lowboundary_test(struct elan_ts_data *ts)
{
	int ret = 0;
	
	printk("[elan]========Start Mean Low Boundary Test==================\n");
	
	/*step 1: set and read ph check , arg1:ph1, arg2:ph2, arg3:ph3*/
	printk("[elan]\t%s set and read back phx\n\n", __func__);
	ret = elan_set_read_ph(ts, 0x19, 0x38, 0x15);
	if (ret) {
		printk("[elan]\t%s set and read back phx failed\n",__func__);
		return ERR_SETPHX; // return ret;
	}

	mdelay(100);
	/*step 2: get normal adc*/
	printk("[elan] \t%s get normal adc 2 \n\n",__func__);
	ret = elan_mt_get_normal_adc(ts);
	if (ret) {
		printk("[elan] \t%s get normal adc failed\n",__func__);
		return ERR_NORMALADC; // return ret;
	}

	/*step 3: low boundary test*/
	printk("[elan] \t%s lowboundary_test\n\n",__func__);
	ret = adc_mean_lowboundary_test(ts);
	if (ret) {
		printk("[elan] \t%s lowboundary_test failed\n",__func__);
		return ERR_LBTEST;
	}

	printk("[elan]========End Mean Low Boundary Test==================\n");
	return ACK_OK;
}

/*stage4 tx open test*/
static int elan_mt_tx_open_test(struct elan_ts_data *ts)
{
	int ret = 0;

	printk("\n[elan]==========Start Tx Open Test====================\n");
	ret = elan_tx_open_test(ts);
	if (ret)
		ret = ERR_TXOPENTEST;
	printk("\n[elan]==========End Tx Open Test====================\n");
	return ret;
}

/*stage5 txrx short test*/
static int elan_txrx_short_check(struct elan_ts_data *ts)
{
	int ret = 0;

	printk("\n[elan]==========Start TxRx Short Test====================\n");
	/*step 1: set and read ph check , arg1:ph1, arg2:ph2, arg3:ph3*/
	printk("[elan]\t%s set and read back phx\n\n", __func__);
	ret = elan_set_read_ph(ts, 0x19, 0x60, 0x60);
	if (ret) {
		printk("[elan]\t%s set and read back phx failed\n",__func__);
		return ERR_SETPHX;//return ret;
	}

	/*step 2: enter short mode*/
	printk("[elan] \t%s enter short mode\n\n", __func__);
	ret = enter_short_mode(ts); 
	if(ret) {
		printk("[elan] \t%s enter short mode failed\n", __func__);
		return ERR_SHORTMODE;//return ret;
	}
	
	/*step 3:get rxtx short data*/
	printk("[elan] \t%s get txrx short data\n\n", __func__);
	ret = txrx_short_get_data(ts);
	if(ret) {
		printk("[elam] \t%s get  txrx short data failed\n",__func__);
		return ERR_GETSHORTDATA;//return ret;
	}

	/*step 4: rxtx short test*/
	printk("[elan] \t %s rxtx short test\n\n", __func__);
	ret = rxtx_short_test(ts);
	if (ret) {
		return ERR_SHORTTEST;
		printk("[elan] \t%s rxtx short test failed!!\n",__func__);
	}
	printk("\n[elan]==========End TxRx Short Test====================\n");

	return ACK_OK;//return ret;
}


int tp_module_test(struct elan_ts_data *ts)
{
	int ret = 0;
	/*stag1 */
	
	printk("[elan]\tstage1: Tp Module Init\n");
	ret = tp_module_test_init(ts);
	if (ret) { 
		ret = ERR_TPMODUINIT;
		goto err_test_init;
	}
	/*stage 2: get noise data*/
	
	/*stage 2: rx open test*/
	printk("[elan]\n\tstage2: Tp Rx Open Test\n");
	ret = elan_mt_rx_open_test(ts);
	if(ret) {
		ret = ERR_RXOPENTEST;
		goto err_normal_adc;
	}
	/*stage 3: meaning value check*/
	printk("[elan]\n\tstage3: Mean Low Boundary Test\n");
	ret = elan_mt_mean_lowboundary_test(ts);
	if (ret) {
		ret = ERR_MEANLBTEST;
		goto err_mean_lowboundary;
	}
	/*stage 4: tx open check();*/
	printk("[elan]\n\tstage4: Tx Open Test\n");
	ret = elan_mt_tx_open_test(ts);
	if (ret) {
		ret = ERR_TXOPEN;
		goto err_tx_open;
	}
	/*stage 5: txrx short test*/
	printk("[elan]\n\tstage5: RXTX Short Test\n");
	ret = elan_txrx_short_check(ts);
	if (ret) {
		ret = ERR_TXRXSHORT;
		goto err_txrx_short;
	}

	ret = ACK_OK;
	printk("[elan] Tp Module Test Success!!!\n");

err_txrx_short:
err_tx_open:
err_mean_lowboundary:
err_normal_adc:
err_test_init:

	elan_switch_irq(1);
	elan_ts_hw_reset(&ts->hw_info);
	return ret;
}




