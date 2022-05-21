#include <linux/module.h>  /* Include macros such as, module_init & module_exit */
#include <linux/fs.h>      /* Define init/free functions for device number*/
#include <linux/device.h>  /* Include functions for creating device file*/
#include <linux/slab.h>    /* Include: kmalloc & kfree*/
#include <linux/cdev.h>    /* Include functions for operate with cdev*/
#include <linux/uaccess.h> /* Include functions for data exchange between user and kernel*/
#include <linux/ioctl.h>   /* Include functions for ioctl operation */


#include "char_driver.h"   /* Include registers description for char_driver*/

#define DRIVER_AUTHOR "La Nhat Hy <lahyus7399@gmail.com>"
#define DRIVER_DESC   "A sample virtual character device driver"
#define DRIVER_VERSION "0.7"

/* Define ioctl commands code */
#define MAGICAL_NUMBER 243
#define CHAR_CLR_DATA_REGS _IO(MAGICAL_NUMBER, 0) // Clear data registers
#define CHAR_GET_STS_REGS _IOR(MAGICAL_NUMBER, 1, sts_regs_t *) // Get status from status register
#define CHAR_SET_RD_DATA_REGS _IOW(MAGICAL_NUMBER, 2, unsigned char *) // Set reading permission for data registers
#define CHAR_SET_WR_DATA_REGS _IOW(MAGICAL_NUMBER, 3, unsigned char *) // Set writing permission for data registers

typedef struct 
{
	unsigned char read_count_h_reg;
	unsigned char read_count_l_reg;
	unsigned char write_count_h_reg;
	unsigned char write_count_l_reg;
	unsigned char device_status_reg;
} sts_regs_t;

// Character Device data structure
typedef struct char_dev
{
	unsigned char *control_regs; // control register
	unsigned char *status_regs;  // status register
	unsigned char *data_regs;    // data register
} char_dev_t;

// Character Driver data structure
struct _char_drv
{
    dev_t dev_num; 			     // device number
	struct class *dev_class;     // class contains devices
	struct device *dev; 	     // device in class
	char_dev_t *char_hw;	     // hardware device

	struct cdev *vcdev;          // cdev structure is used to describe character device
	unsigned int open_cnt;		 // number of file open time
} char_drv;

/****************************** DEVICE SPECIFIC - START *****************************/
/* Function: Initialize device */
int char_hw_init(char_dev_t *hw)
{
	// Initialize buffer for registers
	char* buf;
	buf = kzalloc(NUM_DEV_REGS * REG_SIZE, GFP_KERNEL);
	if(!buf)
		return -ENOMEM;

	hw->control_regs = buf;
	hw->status_regs = hw->control_regs + NUM_CTRL_REGS;
	hw->data_regs = hw->status_regs + NUM_STS_REGS;

	// Initialize data for registers
	hw->control_regs[CONTROL_ACCESS_REG] = 0x03;
	hw->status_regs[DEVICE_STATUS_REG] = 0x03;

	return 0;
}

/* Function: Release device */
void char_hw_exit(char_dev_t *hw)
{
	kfree(hw->control_regs);
}

/* Function: Read data from registers of device 
   Parameters:
		* hw: pointer to char device
   		* start_reg: start reading data register
		* num_regs: number of register to read
		* kbuf: address of kernel buffer
*/
int char_hw_read_data(char_dev_t *hw, int start_reg, int num_regs, char* kbuf)
{
	
	int read_bytes = num_regs;
	
	// Check for reading data permission
	if((hw->control_regs[CONTROL_ACCESS_REG] & CTRL_READ_DATA_BIT) == DISABLE)
		return -1;

	// Check for the validity of kernel buffer address
	if(kbuf == NULL)
		return -1;

	// Check for the validity of registers position
	if(start_reg > NUM_DATA_REGS)
		return -1;
		
	// Adjust the number of register(if necessary)
	if(num_regs > (NUM_DATA_REGS - start_reg))
		read_bytes = NUM_DATA_REGS - start_reg;
		
	// Read data from registers to kernel buffer
		// Because this is the virtual device on RAM, we just use 
		// memcpy function to read data of character device
	memcpy(kbuf, hw->data_regs + start_reg, read_bytes);

	// Update reading data time
	hw->status_regs[READ_COUNT_L_REG] += 1;
	if(hw->status_regs[READ_COUNT_L_REG] == 0)
		hw->status_regs[READ_COUNT_H_REG] += 1;
	
	// Return read byte number
	return read_bytes;
}

/* Function: Write data to registers of device 
   Parameters:
		* hw: pointer to char device
   		* start_reg: start writing data register
		* num_regs: number of register to write
		* kbuf: address of kernel buffer
*/
int char_hw_write_data(char_dev_t *hw, int start_reg, int num_regs, char* kbuf)
{
	int write_bytes = num_regs;

	// Check for writing data permission
	if((hw->control_regs[CONTROL_ACCESS_REG] & CTRL_WRITE_DATA_BIT) == DISABLE)
		return -1;

	// Check for the validity of kernel buffer address
	if(kbuf == NULL)
		return -1;

	// Check for the validity of registers position
	if(start_reg > NUM_DATA_REGS)
		return -1;

	// Adjust the number of register(if necessary)
	if(num_regs > (NUM_DATA_REGS - start_reg))
	{
		write_bytes = NUM_DATA_REGS - start_reg;
		hw->status_regs[DEVICE_STATUS_REG] |= STS_DATAREGS_OVERFLOW_BIT;
	}

	// Write data from kernel buffer to register
		// Because this is the virtual device on RAM, we just use 
		// memcpy function to write data to character device
	memcpy(hw->data_regs + start_reg, kbuf, write_bytes);

	// Update writing data time
	hw->status_regs[WRITE_COUNT_L_REG] += 1;
	if(hw->status_regs[WRITE_COUNT_L_REG] == 0)
		hw->status_regs[WRITE_COUNT_H_REG] += 1;

	// Return read byte number
	return write_bytes;
}

/* Function: Clear data on registers */
int char_hw_clear_data(char_dev_t *hw)
{
	if((hw->control_regs[CONTROL_ACCESS_REG] & CTRL_WRITE_DATA_BIT) == DISABLE)
		return -1;
	
	memset(hw->data_regs, 0, NUM_DATA_REGS * REG_SIZE); // Remove data on registers
	hw->status_regs[DEVICE_STATUS_REG] &= ~STS_DATAREGS_OVERFLOW_BIT; // Delete overflow bit status
	
	return 0;
}

/* Function: Read status data from status register */
void char_hw_get_status(char_dev_t *hw, sts_regs_t *status)
{
	// Copy content of 5 status registers to sts_regs_t structure
	memcpy(status, hw->status_regs, NUM_STS_REGS * REG_SIZE);
}

/* Functions: Set up control status for control registers */
    /* ENABLE or DISABLE READ */
void char_hw_enable_read(char_dev_t *hw, unsigned char isEnable)
{
	if(isEnable == ENABLE)
	{
		// control allow read from data registers (adjust on bit 0 of CONTROL_ACCESS_REG register)
		hw->control_regs[CONTROL_ACCESS_REG] |= CTRL_READ_DATA_BIT;
		// update status "enable read" (adjust on bit 0 of DEVICE_STATUS_REG register)
		hw->status_regs[DEVICE_STATUS_REG] |= STS_READ_ACCESS_BIT;
	}
	else
	{
		// control not allow read from data registers (adjust on bit 0 of CONTROL_ACCESS_REG register)
		hw->control_regs[CONTROL_ACCESS_REG] &= ~CTRL_READ_DATA_BIT;
		// update status "disable read" (adjust on bit 0 of DEVICE_STATUS_REG register)
		hw->status_regs[DEVICE_STATUS_REG] &= ~STS_READ_ACCESS_BIT;
	}
}

    /* ENABLE or DISABLE WRITE */
void char_hw_enable_write(char_dev_t *hw, unsigned char isEnable)
{
	if(isEnable == ENABLE)
	{
		// control allow write on data register (adjust on bit 0 of CONTROL_ACCESS_REG register)
		hw->control_regs[CONTROL_ACCESS_REG] |= CTRL_WRITE_DATA_BIT;
		// update status "enable write" (adjust on bit 0 of DEVICE_STATUS_REG register)
		hw->status_regs[DEVICE_STATUS_REG] |= STS_WRITE_ACCESS_BIT;
	}
	else
	{
		// control not allow write on data registers (adjust on bit 0 of CONTROL_ACCESS_REG register)
		hw->control_regs[CONTROL_ACCESS_REG] &= ~CTRL_WRITE_DATA_BIT;
		// update status "disable write" (adjust on bit 0 of DEVICE_STATUS_REG register)
		hw->status_regs[DEVICE_STATUS_REG] &= ~STS_WRITE_ACCESS_BIT;
	}
}

/******************************* DEVICE SPECIFIC - END *****************************/



/******************************** OS SPECIFIC - START *******************************/

/* Functions: Entry points */
static int char_driver_open(struct inode *inode, struct file *filp)
{
	char_drv.open_cnt++; // increase file open time
	printk("Handle opened event (%d)", char_drv.open_cnt);
	return 0;
}

static int char_driver_release(struct inode *inode, struct file *filp)
{
	printk("Handle closed event\n");
	return 0;
}

static ssize_t char_driver_read(struct file *filp, char __user *user_buf, size_t len, loff_t *off)
{
	char *kernel_buf = NULL;
	int num_bytes = 0;
	printk("Handle read event start from %lld, %zu byte\n", *off, len);

	kernel_buf = kzalloc(len, GFP_KERNEL); // allocate kernel buffer with size of len
	if(kernel_buf == NULL)
		return 0;

	// read data from char device buffer to kernel buffer
	num_bytes = char_hw_read_data(char_drv.char_hw, *off, len, kernel_buf); 
	printk("read %d bytes from HW\n", num_bytes);

	if(num_bytes < 0)
	{
		printk("num_bytes < 0 num_bytes = %d\n",num_bytes);
		return -EFAULT;
	}

	if(copy_to_user(user_buf, kernel_buf, num_bytes)) // copy data from kernel buffer to user buffer
	{
		printk("!copy_to_user & num_bytes = %d\n",num_bytes);
		return -EFAULT;
	}

	*off += num_bytes; // update offset value
	return num_bytes;  // return read byte number
}

static ssize_t char_driver_write(struct file *filp, const char __user *user_buf, size_t len, loff_t *off)
{
	char *kernel_buf = NULL;
	int num_bytes = 0;
	printk("Handle write event start from %lld, %zu bytes\n", *off, len);

	kernel_buf = kzalloc(len, GFP_KERNEL);        // allocate kernel buffer with size of len
	if(copy_from_user(kernel_buf, user_buf, len)) // copy data from user buffer to kernel buffer
		return -EFAULT;
	
	// write data from kernel buffer to char device buffer 
	num_bytes = char_hw_write_data(char_drv.char_hw, *off, len, kernel_buf);
	printk("write %d bytes to HW\n", num_bytes);

	if(num_bytes < 0)
		return -EFAULT;
	
	*off += num_bytes; // update offset value
	return num_bytes;  // return write byte number
}

static long char_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	printk("Handle ioctl event (cmd: %u)\n", cmd);

	switch (cmd)
	{
		case CHAR_CLR_DATA_REGS:
		{
			ret = char_hw_clear_data(char_drv.char_hw);
			if(ret < 0)
				printk("Can not clear data registers\n");
			else
				printk("Data registers have been cleared\n");
		}
			break;
		case CHAR_SET_RD_DATA_REGS:
		{
			unsigned char isReadEnable;
			copy_from_user(&isReadEnable, (unsigned char*)arg, sizeof(isReadEnable)); // get current permission from user
			char_hw_enable_read(char_drv.char_hw, isReadEnable); // set permission
			printk("Data registers have been %s to read\n", (isReadEnable == ENABLE)?"enable":"disable");
		}
			break;
		case CHAR_SET_WR_DATA_REGS:
		{
			unsigned char isWriteEnable;
			copy_from_user(&isWriteEnable, (unsigned char*)arg, sizeof(isWriteEnable)); // get current permission from user
			char_hw_enable_write(char_drv.char_hw, isWriteEnable); // set permission
			printk("Data registers have been %s to write\n", (isWriteEnable == ENABLE)?"enable":"disable");
		}
			break;
		case CHAR_GET_STS_REGS:
		{
			sts_regs_t status;
			char_hw_get_status(char_drv.char_hw, &status); // get current status
			copy_to_user((sts_regs_t*)arg, &status, sizeof(status)); // set status to user
			printk("Got information from status registers\n");
		}
			break;
	}
	return ret;
}

/* 
	File Operations structure includes function pointers. 
    It create a 1-1 link between system calls and entry points of the driver
*/
static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = char_driver_open,
	.release = char_driver_release,
	.read = char_driver_read,
	.write = char_driver_write,
	.unlocked_ioctl = char_driver_ioctl,
};

/* Function: Initialize driver */
static int __init char_driver_init(void)
{
    int ret = 0;

	/* Allocate Device Number */
    char_drv.dev_num = 0;
    ret = alloc_chrdev_region(&char_drv.dev_num, 0, 1, "char_device"); // Find a value for device number
    
	if (ret < 0)
    {
        printk("Failed to register device number dynamically\n");
        goto failed_register_devnum;
    }

	printk("allocated device number (%d, %d)\n", MAJOR(char_drv.dev_num), MINOR(char_drv.dev_num));

	/* Create Device File */
		// first: create class name "class_char_device_file"
	char_drv.dev_class = class_create(THIS_MODULE, "class_char_device_file"); 
	if(char_drv.dev_class == NULL)
	{
		printk("failed to create a device class\n");
		goto failed_create_class;
	}

		// second: create device name "char_device_file" with allocated device number
	char_drv.dev = device_create(char_drv.dev_class, NULL, char_drv.dev_num, NULL, "char_device_file");
	if(IS_ERR(char_drv.dev))
	{
		printk("failed to create a device\n");
		goto failed_create_device;
	}

	/* Allocate memory for driver data structure & Initialize */
	char_drv.char_hw = kzalloc(sizeof(char_dev_t), GFP_KERNEL); // allocate memory
	if(!char_drv.char_hw)
	{
		printk("failed to allocate data structure of the driver\n");
		ret = -ENOMEM;
		goto failed_allocate_structure;
	}

	/* Initialize hardware device */
	ret = char_hw_init(char_drv.char_hw);
	if(ret < 0)
	{
		printk("failed to initialize a virtual character device\n");
		goto failed_init_hw;
	}

	/* Register entry point with kernel */
	char_drv.vcdev = cdev_alloc(); // request kernel allocate memory for cdev structure
	if(char_drv.vcdev == NULL)
	{
		printk("failed to allocate cdev structure\n");
		goto failed_allocate_cdev;
	}

		// initialize cdev structure and fill fops information
	cdev_init(char_drv.vcdev, &fops); 
		// register cdev structure with kernel and link to device file
	ret = cdev_add(char_drv.vcdev, char_drv.dev_num, 1); 
	if(ret < 0)
	{
		printk("failed to add a char device to the system\n");
		goto failed_allocate_cdev;
	}

	/* Register handling interrupt function */
	printk("Initialize char driver successfully\n");
	return 0;

failed_allocate_cdev:
	char_hw_exit(char_drv.char_hw);

failed_init_hw:
	kfree(char_drv.char_hw);

failed_allocate_structure:
	device_destroy(char_drv.dev_class, char_drv.dev_num);

failed_create_device:
	class_destroy(char_drv.dev_class);

failed_create_class:
	unregister_chrdev_region(char_drv.dev_num, 1);

failed_register_devnum: 
    return ret;
}

/* Function: Stop driver */
static void __exit char_driver_exit(void)
{
	/* Cancel interrupt handling */

	/* Cancel entry point registration to kernel */
	cdev_del(char_drv.vcdev);

	/* Release hardware device */
	char_hw_exit(char_drv.char_hw);

	/* Release allocated memory for driver data structure */
	kfree(char_drv.char_hw);

	/* Delete device file */
	device_destroy(char_drv.dev_class, char_drv.dev_num);
	class_destroy(char_drv.dev_class);

	/* Release device number */
    unregister_chrdev_region(char_drv.dev_num, 1);

	printk("Exit char driver\n");
}
/********************************* OS SPECIFIC - END ********************************/

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);