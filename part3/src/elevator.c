#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module proc file for elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* elevator_entry;

// H attempt added stuff

#define  MAX_PASSENGERS 5
#define MAX_WEIGHT 750

#define PASS_FRESHMAN 0
#define PASS_SOPHMORE 1
#define PASS_JUNIOR 2
#define PASS_SENIOR 3


struct {
    int total_cnt;
    int total_weight;
    struct list_head list;
} passengers;

typedef struct passenger {
    int id;
    int weight;
    const char *name;
    struct list_head list;
} Passenger;

int add_passenger(int type)
{
    int weight;
    char *name;
    Passenger *p;

    if (passengers.total_cnt >= MAX_PASSENGERS)
        return 0;

// switch on what is inputted into add passenger and its gonna get called in elevator_proc_open i think
    switch (type)
{
    case PASS_FRESHMAN:
        weight = 100;
        // dont know if i need this name part
        name = "Freshman";
        break;
    case PASS_SOPHMORE:
        weight = 150;
        name = "Sophmore";
        break;
    case PASS_JUNIOR:
        weight = 200;
        name = "Junior";
        break;
    case PASS_SENIOR:
        weight = 250;
        name = "Senior";
        break;
    default:
        return -1;
}

    p->id = type;
    p->weight = weight;
    p->name = name;

    passengers.total_cnt += 1;
    passengers.total_weight += weight;

    return 0;
}

int del_passenger(int type)
{


    return 0;
}
// H attempt added stuff end

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: \n");
    len += sprintf(buf + len, "Current load: \n");
    len += sprintf(buf + len, "Elevator status: \n");

    len += sprintf(buf + len, "\n[ ] Floor 6: \n");
    len += sprintf(buf + len, "[ ] Floor 5: \n");
    len += sprintf(buf + len, "[ ] Floor 4: \n");
    len += sprintf(buf + len, "[ ] Floor 3: \n");
    len += sprintf(buf + len, "[ ] Floor 2: \n");
    len += sprintf(buf + len, "[ ] Floor 1: \n");

    len += sprintf(buf + len, "\nNumber of passengers: \n");
    len += sprintf(buf + len, "Number of passengers waiting: \n");
    len += sprintf(buf + len, "Number of passengers serviced: \n");




    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void)
{
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit elevator_exit(void)
{
    proc_remove(elevator_entry);
}

module_init(elevator_init);
module_exit(elevator_exit);
