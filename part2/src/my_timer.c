#include "my_timer.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");

static struct timespec64 last_time;
static char buf[128]; // to hold timestamp and elapsed time
static struct proc_dir_entry *timer_entry; // pointer to be able to rm entry

// Reading from the /proc/timer file
static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 curr_ts;
    struct timespec64 diff_ts;

    memset(buf, 0, sizeof(buf));

    ktime_get_real_ts64(&curr_ts); //gets curr timestamp


    if (last_time.tv_sec || last_time.tv_nsec) { // Second+ read case
        diff_ts = timespec64_sub(curr_ts, last_time); //Calculates difference from curr to last

        snprintf(buf, sizeof(buf), "current time: %lld.%09ld\nelapsed time: %lld.%09ld\n", 
                curr_ts.tv_sec, curr_ts.tv_nsec, 
                diff_ts.tv_sec, diff_ts.tv_nsec);
    }
    else {  // First read cade
        snprintf(buf, sizeof(buf), "current time: %lld.%09ld\n", curr_ts.tv_sec, curr_ts.tv_nsec);
    }

    // Update last_time for next read
    last_time = curr_ts;

    if (*ppos > 0 || count < strlen(buf)) {
        return 0;
    }

    if (copy_to_user(ubuf, buf, strlen(buf))) {
        return -EFAULT;
    }

    *ppos = strlen(buf);

    return strlen(buf);
}

// File operations for /proc: read
static struct file_operations proc_ops = {
    .owner = THIS_MODULE,
    .read = timer_read,
};

// For load
static int __init timer_init(void)
{
    timer_entry = proc_create("timer", 0444, NULL, &proc_ops); //Creates timer
    if (!timer_entry) {
        return -ENOMEM; //returns memory error if failed
    }

    return 0;
}

// For unload
static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);
