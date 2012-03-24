/*
 * kcalc.c - A character device that can be used for 
 * basic arithmetic calculation.
 *
 * Copyright (C) 2012 Linjie Ding <i [at] dingstyle.me>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/device.h>

#include <asm/uaccess.h>

/* parser header */
#include "parser.h"

/* buffer declaration */
#include "buffer.h"

/* function prototypes */

static int kcalc_open(struct inode *inode, struct file *file);
static int kcalc_release(struct inode *inode, struct file *file);
static ssize_t kcalc_read(struct file *file, char __user *buf,
                          size_t count, loff_t *ppos);
static ssize_t kcalc_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos);

/* global variables */

static struct class *kcalc_class;
static dev_t kcalc_dev;         /* dynamically assigned at registration. */
static struct cdev *kcalc_cdev; /* dynamically allocated at runtime. */

/* file operations */

static struct file_operations kcalc_fops = {
    .read    = kcalc_read,
    .write   = kcalc_write,
    .open    = kcalc_open,
    .release = kcalc_release,
    .owner   = THIS_MODULE,
};

/*
 * kcalc_open: open the pseudo kcalc device
 * @inode: the inode of the /dev/kcalc device
 * @file:  the in-kernel representation of this opened device
 */

static int kcalc_open(struct inode *inode, struct file *file) {
    /* A debug message when the device got opened */
    printk(KERN_INFO "kcalc: device file opened.\n");
    return 0;
}

/*
 * kcalc_release: close (release) the pseudo kcalc device
 * @inode: the inode of the /dev/kcalc device
 * @file:  the in-kernel representation of this opened device
 */

static int kcalc_release(struct inode *inode, struct file *file) {
    /* A debug message when the device is closedv */
    printk(KERN_INFO "kcalc: device file released.\n");
    return 0;
}

/*
 * kcalc_read: read the calculation result from device
 * @file:  the in-kernel representation of this opened device
 * @buf:   the userspace buffer to write into
 * @count: how many bytes to write into
 * @ppos:  the current file position
 */

static ssize_t kcalc_read(struct file *file, char __user *buf,
                          size_t count, loff_t *ppos) {
    ssize_t bytes = count;

    if (count > result_ring.count)
        bytes = result_ring.count;
    
    if (result_ring.head + bytes >= RING_SIZE) {
        ssize_t fst = RING_SIZE - result_ring.head, rst = bytes - fst;
        /* copy the first part */
        if (copy_to_user((void __user *)buf,
                    (void *)(result_ring.buf + result_ring.head), fst))
            return -EFAULT;
        /* copy the rest part */
        if (copy_to_user((void __user *)buf, result_ring.buf, rst))
            return -EFAULT;
        result_ring.head = (result_ring.head + bytes) % RING_SIZE;
        result_ring.count -= bytes;
    } else {
        if (copy_to_user((void __user *)buf, result_ring.buf, bytes))
            return -EFAULT;
        result_ring.head = result_ring.head + bytes;
        result_ring.count -= bytes;
    }

    *ppos += bytes;
    return bytes;
}

/*
 * kcalc_write: write the expressions into device
 * @file:  the in-kernel representation of this opened device
 * @buf:   the userspace buffer to read from
 * @count: how many bytes to read from
 * @ppos:  the current file position
 */
static ssize_t kcalc_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos) {
    size_t bytes = count;

    /* truncate at data longer than INBUF_SIZE */
    if (count > INBUF_SIZE)
        bytes = INBUF_SIZE;

    if (copy_from_user((void *)(expr_buf.buf),
                (const void __user *)buf, bytes))
        return -EFAULT;

    *ppos += bytes;

    /* evaluate expressions and put the resuolt in the output ring */
    kcalc_parse();

    /* reset read posistion and count of input buffer */
    expr_buf.count = expr_buf.rpos = 0;

    return bytes;
}

/*
 * kcalc_init: initialize the pseudo kcalc device
 * Description: This function allocates a few resources (a cdev,
 * a device , a sysfs class...) in order to register a new device
 * and populate an entry in sysfs that udev can use to setup a 
 * new /dev/kcalc entry for reading from the device.
 */

static int __init kcalc_init(void) {
    if (alloc_chrdev_region(&kcalc_dev, 0, 1, "kcalc"))
        goto error;

    if ((kcalc_cdev = cdev_alloc()) == 0)
        goto error;

    kobject_set_name(&kcalc_cdev->kobj, "kcalc_cdev");
    kcalc_cdev->ops = &kcalc_fops;  /* wire up file ops */
    if (cdev_add(kcalc_cdev, kcalc_dev, 1)) {
        kobject_put(&kcalc_cdev->kobj);
        unregister_chrdev_region(kcalc_dev, 1);
        goto error;
    }

    kcalc_class = class_create(THIS_MODULE, "kcalc");
    if (IS_ERR(kcalc_class)) {
        printk(KERN_ERR "Error creating kcalc class.\n");
        cdev_del(kcalc_cdev);
        unregister_chrdev_region(kcalc_dev, 1);
        goto error;
    }
    device_create(kcalc_class, NULL, kcalc_dev, NULL, "kcalc");
    printk(KERN_INFO "KKK! my dev id %d, %d\n", MAJOR(kcalc_dev), MINOR(kcalc_dev));

    return 0;

error:
    printk(KERN_ERR "kcalc: could not register device.\n");
    return 1;
}

/*
 * kcalc_exit: uninitialize the pseudo kcalc device
 * Description: This function frees up any resource that got allocated 
 * at init time and prepares for the driver to be unloaded.
 */

static void __exit kcalc_exit(void) {
    device_destroy(kcalc_class, kcalc_dev);
    class_destroy(kcalc_class);
    cdev_del(kcalc_cdev);
    unregister_chrdev_region(kcalc_dev, 1);
}

/* declare init/exit functions */

module_init(kcalc_init);
module_exit(kcalc_exit);

/* define module meta data */

MODULE_AUTHOR("Linjie Ding <i [at] dingstyle.me>");
MODULE_DESCRIPTION("A character device providing simple arithmetic calculation function.");
MODULE_LICENSE("GPL v2");
