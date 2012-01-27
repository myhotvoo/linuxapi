#define DEBUG
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>

//define DO_DEBUG
#include "kernel_helper.h" // our own helper

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Veltzer");
MODULE_DESCRIPTION("Show the completion API for completing requests from the kernel");

/*
 * The completion API is a fairly recent API within the kernel that is lighter than
 * the wait_queue API in that on each completion event you can have just one process running.
 * Meaning that the completion structure is NOT a linked list but rather just one process(thread)
 * waiting. This is good for writing various designs where we lower the request into the kernel and
 * are not sure who exactly is going to handle it at lower levels and therefore we would not like to
 * commit to specific wait queue at this time (think packet handling etc...).
 *
 * TODO:
 * - do dynamic allocation of chrdev and remove the stupid paramters for this module.
 * - remove unneeded includes.
 * - find the right h file to include for the completion API (I am including way too much).
 * - do better ioctl names instead of numbers.
 */

// parameters for this module

static int chrdev_alloc_dynamic = 1;
module_param(chrdev_alloc_dynamic, bool, 0444);
MODULE_PARM_DESC(chrdev_alloc_dynamic, "Allocate the device number dynamically?");

static int first_minor = 0;
module_param(first_minor, int, 0444);
MODULE_PARM_DESC(first_minor, "first minor to allocate in dynamic mode (usually best to keep at 0)");

static int kern_major = 253;
module_param(kern_major, int, 0444);
MODULE_PARM_DESC(kern_major, "major to allocate in static mode");

static int kern_minor = 0;
module_param(kern_minor, int, 0444);
MODULE_PARM_DESC(kern_minor, "minor to allocate in static mode");

// constants for this module

// number of files we expose via the chr dev
static const int MINORS_COUNT = 1;

int register_dev(void);
void unregister_dev(void);

// our own functions
static int __init mod_init(void) {
	return(register_dev());
}


static void __exit mod_exit(void) {
	unregister_dev();
}


// declaration of init/cleanup functions of this module

module_init(mod_init);
module_exit(mod_exit);

// first the structures

struct kern_dev {
	// pointer to the first device number allocated to us
	dev_t first_dev;
	// cdev structures for the char devices we expose to user space
	struct cdev cdev;
};

// static data
static struct kern_dev *pdev;
static struct class    *my_class;
static struct device   *my_device;

// now the functions

/*
 * This is the ioctl implementation.
 */
// a completion structure
struct completion comp;
static long kern_unlocked_ioctll(struct file *filp, unsigned int cmd, unsigned long arg) {
	int i;

	PR_DEBUG("start");
	switch (cmd) {
	case 0:
		init_completion(&comp);
		break;

	case 1:
		wait_for_completion(&comp);
		break;

	case 2:
		wait_for_completion_interruptible(&comp);
		break;

	case 3:
		i = wait_for_completion_interruptible_timeout(&comp, msecs_to_jiffies(arg));
		PR_DEBUG("i is %d", i);
		break;

	case 4:
		complete(&comp);
		break;

	case 5:
		complete_all(&comp);
		break;

	case 6:
		INIT_COMPLETION(comp);
		break;
	}
	return(0);
}


/*
 * The open implementation. Currently this does nothing
 */
static int kern_open(struct inode *inode, struct file *filp) {
	PR_DEBUG("start");
	return(0);
}


/*
 * The release implementation. Currently this does nothing
 */
static int kern_release(struct inode *inode, struct file *filp) {
	PR_DEBUG("start");
	return(0);
}


/*
 * The read implementation. Currently this does nothing.
 */
static ssize_t kern_read(struct file *filp, char __user *buf, size_t count, loff_t *pos) {
	PR_DEBUG("start");
	return(0);
}


/*
 * The write implementation. Currently this does nothing.
 */
static ssize_t kern_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos) {
	PR_DEBUG("start");
	return(0);
}


/*
 * The file operations structure.
 */
static struct file_operations my_fops = {
	.owner   = THIS_MODULE,
	.open    = kern_open,
	.release = kern_release,
	.read    = kern_read,
	.write   = kern_write,
	.unlocked_ioctl   = kern_unlocked_ioctll,
};

int register_dev(void) {
	// create a class
	my_class = class_create(THIS_MODULE, THIS_MODULE->name);
	if (IS_ERR(my_class)) {
		goto goto_nothing;
	}
	PR_DEBUG("created the class");
	// alloc and zero
	pdev = kmalloc(sizeof(struct kern_dev), GFP_KERNEL);
	if (pdev == NULL) {
		goto goto_destroy;
	}
	memset(pdev, 0, sizeof(struct kern_dev));
	if (chrdev_alloc_dynamic) {
		if (alloc_chrdev_region(&pdev->first_dev, first_minor, MINORS_COUNT, THIS_MODULE->name)) {
			PR_DEBUG("cannot alloc_chrdev_region");
			goto goto_dealloc;
		}
	} else {
		pdev->first_dev = MKDEV(kern_major, kern_minor);
		if (register_chrdev_region(pdev->first_dev, MINORS_COUNT, THIS_MODULE->name)) {
			PR_DEBUG("cannot register_chrdev_region");
			goto goto_dealloc;
		}
	}
	PR_DEBUG("allocated the device");
	// create the add the sync device
	cdev_init(&pdev->cdev, &my_fops);
	pdev->cdev.owner = THIS_MODULE;
	pdev->cdev.ops = &my_fops;
	kobject_set_name(&pdev->cdev.kobj, THIS_MODULE->name);
	if (cdev_add(&pdev->cdev, pdev->first_dev, 1)) {
		PR_DEBUG("cannot cdev_add");
		goto goto_deregister;
	}
	PR_DEBUG("added the device");
	// now register it in /dev
	my_device = device_create(
	        my_class,                                                                                                                   /* our class */
	        NULL,                                                                                                                       /* device we are subdevices of */
	        pdev->first_dev,
	        NULL,
		"%s",
	        THIS_MODULE->name
	        );
	if (my_device == NULL) {
		PR_DEBUG("cannot create device");
		goto goto_create_device;
	}
	PR_DEBUG("did device_create");
	return(0);

//goto_all:
//	device_destroy(my_class,pdev->first_dev);
goto_create_device:
	cdev_del(&pdev->cdev);
goto_deregister:
	unregister_chrdev_region(pdev->first_dev, MINORS_COUNT);
goto_dealloc:
	kfree(pdev);
goto_destroy:
	class_destroy(my_class);
goto_nothing:
	return(-1);
}

void unregister_dev(void) {
	device_destroy(my_class, pdev->first_dev);
	cdev_del(&pdev->cdev);
	unregister_chrdev_region(pdev->first_dev, MINORS_COUNT);
	kfree(pdev);
	class_destroy(my_class);
}