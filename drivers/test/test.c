#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/poll.h>

struct test_cdev{
	int val;
	struct cdev cv;
};

dev_t devNo;
struct test_cdev *test_c;
static DECLARE_WAIT_QUEUE_HEAD(test_waitq);
static int test_int_flag = 0;



int test_open(struct inode *node, struct file *filp)
{
	struct test_cdev *n_cdev = container_of(node->i_cdev, struct test_cdev, cv);
	filp->private_data = n_cdev;
	return 0;
}

int test_release(struct inode *node, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

ssize_t test_read(struct file *filp, char __user *buf, size_t len, loff_t *pos)
{
	struct test_cdev *n_cdev = (struct test_cdev *)filp->private_data;
	int err = 0;

	if(copy_to_user(buf, &n_cdev->val, sizeof(n_cdev->val)))
	{
		err = -EFAULT;
		return err;
	}

	return sizeof(n_cdev->val);
}

ssize_t test_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	struct test_cdev *n_cdev = (struct test_cdev *)filp->private_data;
	int err = 0;

	if(copy_from_user(&n_cdev->val, buf, len))
	{
		err = -EFAULT;
		return err;
	}
	
	return sizeof(n_cdev->val);
}
/*
static unsigned int test_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	poll_wait(filp, test_waitq, wait);

	if(test_int_flag > 0)
		mask = test_int_flag;

	test_int_flag 0;
	return mask;
}
*/
struct file_operations test_op = {
	.owner  =       THIS_MODULE,
	.open   =       test_open,
	.release=       test_release,
	.read   =       test_read,
	.write  =       test_write,
//	.poll	=		test_poll,
};

static int test_init(void)
{
	struct class *cls;
	int err = 0;
	
	
	err = alloc_chrdev_region(&devNo,0,1,"test");
	if (err){
		printk("[Minger] alloc_chrdev_region err\n");
		return err;
	}

	test_c = kmalloc(sizeof(struct test_cdev), GFP_KERNEL);
	
	cdev_init(&test_c->cv, &test_op);
	
	err = cdev_add(&test_c->cv, devNo,1);
	if (err) {
		printk("[Minger] cdev_add err\n");
		return err;
	}
	
	test_c->val = 8;
	cls = class_create(THIS_MODULE, "test");
	device_create(cls, NULL, devNo, NULL, "test");
	return 0;
}

static void test_exit(void)
{
	unregister_chrdev_region(devNo, 1);
	kfree(test_c);
	cdev_del(&test_c->cv);
}

module_init(test_init);
module_exit(test_exit);
















