#pragma once

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);

static int __init timer_init(void);
static void __exit timer_exit(void);
