#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/uaccess.h>	/* copy_*_user */

#define MYDEV_NAME "mycdrv"
#define ramdisk_size (size_t) (16*PAGE_SIZE)

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IOW(CDRV_IOC_MAGIC, 1, int)

/* define a device structer and embed cdev */
struct asp_mycdev{
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
};


static int my_major = 300, my_minor = 0;
struct asp_mycdev *my_cdevs;
struct class *asp_class;

int NUM_DEVICES = 3;
module_param(NUM_DEVICES,int,0);

int mycdev_open(struct inode *inode, struct file *filp){
    struct asp_mycdev *dev;

    dev = container_of(inode->i_cdev, struct asp_mycdev, dev);
    filp->private_data = dev;

    return 0;
}

int mycdev_release(struct inode *inode, struct file *filp){
    return 0;
}

ssize_t mycdev_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
	struct asp_mycdev *dev = file->private_data;
	char *ramdisk = dev->ramdisk;
	int nbytes;
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	down_interruptible(&dev->sem);
	nbytes = lbuf - copy_to_user(buf, ramdisk + *ppos, lbuf);
	*ppos += nbytes;
	pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	up(&dev->sem);
	return nbytes;
}

ssize_t mycdev_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos)
{
	struct asp_mycdev *dev = file->private_data;
	char *ramdisk = dev->ramdisk;
	int nbytes;
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	down_interruptible(&dev->sem);
	nbytes = lbuf - copy_from_user(ramdisk + *ppos, buf, lbuf);
	*ppos += nbytes;
	pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	up(&dev->sem);
	return nbytes;
}

static loff_t mycdev_llseek(struct file *file, loff_t offset, int orig)
{
	loff_t testpos;
	switch (orig) {
	case SEEK_SET:
		testpos = offset;
		break;
	case SEEK_CUR:
		testpos = file->f_pos + offset;
		break;
	case SEEK_END:
		testpos = ramdisk_size + offset;
		break;
	default:
		return -EINVAL;
	}
	testpos = testpos < ramdisk_size ? testpos : ramdisk_size;
	testpos = testpos >= 0 ? testpos : 0;
	file->f_pos = testpos;
	pr_info("Seeking to pos=%ld\n", (long)testpos);
	return testpos;
}

long int mycdev_ioctl(struct file *filp,
                 unsigned int cmd, unsigned long arg){
		int err = 0;
		int retval = 0;

		struct asp_mycdev *dev;
		/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	
	switch(cmd) {
		case ASP_CLEAR_BUF:
			memset(dev->ramdisk, 0, ramdisk_size);
			retval = 1;
			break;

		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	return retval;
}

struct file_operations mycdev_fops = {
	.owner =    THIS_MODULE,
	.llseek =   mycdev_llseek,
	.read =     mycdev_read,
	.write =    mycdev_write,
	.unlocked_ioctl = mycdev_ioctl,
	.open =     mycdev_open,
	.release =  mycdev_release,
};

void mycdev_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(my_major, my_minor);

	/* Get rid of our char dev entries */
	if (my_cdevs) {
		for (i = 0; i < NUM_DEVICES; i++) {
			cdev_del(&my_cdevs[i].dev);
			device_destroy(asp_class, MKDEV(my_major,i));
		}
		kfree(my_cdevs);
	}

	if (asp_class)
		class_destroy(asp_class);

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, NUM_DEVICES);
	pr_info("\ndevice unregistered\n");
}


/*
 * Set up the char_dev structure for this device.
 */
static void my_setup_cdev(struct asp_mycdev *dev, int index, struct class *class)
{
	int err, devno = MKDEV(my_major, my_minor + index);
	struct device *device;
    
	cdev_init(&dev->dev, &mycdev_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &mycdev_fops;
	err = cdev_add (&dev->dev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding cdev%d", err, index);

	device = device_create(class, NULL, devno, NULL, MYDEV_NAME "%d", index);

}


int my_init_module(void)
{
	int result, i;
	dev_t dev = 0;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (my_major) {
		dev = MKDEV(my_major, my_minor);
		result = register_chrdev_region(dev, NUM_DEVICES, MYDEV_NAME);
	} else {
		result = alloc_chrdev_region(&dev, my_minor, NUM_DEVICES,
				MYDEV_NAME);
		my_major = MAJOR(dev);
	}

	if (result < 0) {
		printk(KERN_WARNING "mycdev: can't get major %d\n", my_major);
		return result;
	}

	asp_class = class_create(THIS_MODULE, MYDEV_NAME);
    /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	my_cdevs = kmalloc(NUM_DEVICES * sizeof(struct asp_mycdev), GFP_KERNEL);
	if (!my_cdevs) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(my_cdevs, 0, NUM_DEVICES * sizeof(struct asp_mycdev));

        /* Initialize each device. */
	for (i = 0; i < NUM_DEVICES; i++) {
		my_cdevs[i].ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);

		sema_init(&my_cdevs[i].sem, 1);
		my_setup_cdev(&my_cdevs[i], i, asp_class);
	}

        /* At this point call the init function for any friend device */
	dev = MKDEV(my_major, my_minor + NUM_DEVICES);
	pr_info("\nSucceeded in registering character device %s\n", MYDEV_NAME);

	return 0; /* succeed */

  fail:
		mycdev_cleanup_module();
		return result;
}

module_init(my_init_module);
module_exit(mycdev_cleanup_module);

MODULE_AUTHOR("user");
MODULE_LICENSE("GPL v2");
