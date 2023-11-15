#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/linkage.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module proc file for elevator");

//define constants related to elevator operation
#define MAX_WEIGHT 750
#define MAX_PASSENGERS 5        //We do not have a check for this yet, we need to add it
#define FRESH_WEIGHT 100
#define SOPH_WEIGHT 150
#define JUN_WEIGHT 200
#define SEN_WEIGHT 250
#define MIN_FLOOR 1
#define MAX_FLOOR 6


// Structs Definition of passenger struct with linked list 
typedef struct passenger {
   int start_floor;
   int dest_floor;
   int weight;
   int type;
   struct list_head passengers; // to be used for linked list: link passengers together
} Passenger;

//elevator struct 
struct elevator {
   int current_floor;
   int target_floor;
   int status;
   int ppl_on_board;
   int weight;
   struct list_head passengers; 
   struct task_struct *elevator_thread;
   struct mutex lock;
};

//globals for building and elevator control 
struct list_head building[7];
struct mutex building_lock;

//profile related variables 
static struct proc_dir_entry* proc_entry;
static int read_proc;   // procfile reading
int stop;               // --stop check  

//elevator instance 
struct elevator my_elevator;
int passengers_waiting[7];               
int serviced_passengers;
int none_waiting; // bool 

static char *output;    // str to store output to proc       


//function declarations for elevator operations 
int elevator(void *data);
void add_passenger(void);
void del_passenger(void);
int output_print(void);
void set_to_offline(void);
void elevator_status_setter(void);


// Procfile Handler Functions

static int procfile_open(struct inode *inode, struct file *file) {
   read_proc = 1;
   output = kmalloc(sizeof(char)*800,__GFP_RECLAIM | __GFP_IO | __GFP_FS);
   if(output == NULL) {
       printk(KERN_WARNING "read_proc_warning");
       return -ENOMEM;
   }
   return output_print();
  
}

// read proc file and copy its contents to user buffer 
static ssize_t procfile_read(struct file* file, char __user *buf, size_t size, loff_t *offset) {
    int len = strlen(output);
    int ret;
    read_proc = !read_proc;
    if(read_proc) {
        return 0;
    }
    ret = copy_to_user(buf, output, len);
    if (ret != 0) {
        return -EFAULT;
    }
    return len;
}

//release the procfile and free allocated memory 
int procfile_release(struct inode *sp_inode, struct file *sp_file) {
   kfree(output);
   return 0;
}

// file ops for proc file 
static struct proc_ops procfile_fops = {
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_release = procfile_release,
};




// Elevator Handler Functions

//main elevator thread function
int elevator(void *data) {
    struct elevator *elevator_data = data;
    while(!kthread_should_stop()) {
        if(mutex_lock_interruptible(&elevator_data->lock) == 0) {
            if(my_elevator.status != 1 && my_elevator.status != 2) {
                if(my_elevator.status == 3) {
                    ssleep(1); // waits one sec before loading and unloading
                    if(my_elevator.ppl_on_board > 0) {
                        del_passenger();
                    }
                    if(stop != 1 && passengers_waiting[my_elevator.current_floor] > 0) {
                        add_passenger();
                        }
                    if(my_elevator.ppl_on_board == 0 && stop != 1 &&
                        my_elevator.target_floor == my_elevator.current_floor && none_waiting == 0) {

                        if(my_elevator.current_floor < 6) {
                            my_elevator.target_floor = my_elevator.current_floor++;
                            my_elevator.status = 4;
                        }
                        else {
                            my_elevator.target_floor = my_elevator.current_floor--;
                            my_elevator.status = 5;
                        }
                    }
                    
                    else if (stop == 1 && my_elevator.ppl_on_board != 0) {
                        Passenger *temp;
                        temp = list_first_entry(&my_elevator.passengers, Passenger, passengers);
                        my_elevator.target_floor = temp->dest_floor;

                    elevator_status_setter();
                    }
                }
                // If status is UP or DOWN: sleep for 2 seconds and then move.
                if(my_elevator.status == 4 || my_elevator.status == 5) {
                    ssleep(2);
                    my_elevator.current_floor = my_elevator.target_floor;
                    my_elevator.status = 3;
                }  
            }
            // Check if stop has been called & elevator is empty
            if(stop == 1 && my_elevator.ppl_on_board == 0) {
                set_to_offline();
            }        
            mutex_unlock(&elevator_data->lock); 
        }
    }
    return 0;
}

// Function to set elevator to offline status
void set_to_offline(){
    my_elevator.status = 1;
    my_elevator.target_floor = 1;
    my_elevator.current_floor = 1; 
}

// Function to set UP or DOWN elevator status
void elevator_status_setter(){
    // Elevator status setter
    if(my_elevator.target_floor > my_elevator.current_floor) { //UP
    my_elevator.status = 4;
    }
    else if(my_elevator.target_floor < my_elevator.current_floor){
        my_elevator.status = 5; // DOWN
    }
    // else {
    //     my_elevator.status = 3; // IDLE
    // }    
}

// Function to add / load passangers onto elevator
void add_passenger(void){

    Passenger *pass_temp;
    struct list_head *temp;
    struct list_head *next_node;

    printk(KERN_NOTICE "Loading passengers at floor %d\n",my_elevator.current_floor);
  
        if(mutex_lock_interruptible(&building_lock) == 0) {
            if(passengers_waiting[my_elevator.current_floor] == 0) {
                mutex_unlock(&building_lock);
                return;
            }
            // Loop through list
            list_for_each_safe(temp, next_node, &building[my_elevator.current_floor]) {
                pass_temp = list_entry(temp, Passenger, passengers);

                // Weight and Num Passengers check: move from buildng to elevator if max not surpassed
                if(((pass_temp->weight + my_elevator.weight) < MAX_WEIGHT) && (my_elevator.ppl_on_board < MAX_PASSENGERS)) {
                    my_elevator.ppl_on_board++;
                    my_elevator.weight = my_elevator.weight + pass_temp->weight;
                    my_elevator.target_floor = pass_temp->dest_floor;    // set target floor
                    list_move_tail(temp, &my_elevator.passengers);
                    passengers_waiting[my_elevator.current_floor]--;
                    elevator_status_setter();
                }
            }
            mutex_unlock(&building_lock);
        }
}

// Function to delete / unload passangers from elevator & free memory
void del_passenger(void){
    int i = 1;
    struct list_head *temp, *next_node;
    struct list_head move_list;
    Passenger *pass_temp;

    // Empty elevator check
    if(my_elevator.ppl_on_board == 0) {
        return;
    }

    printk(KERN_NOTICE "Deleting/ Unloading passengers at floor %d\n", my_elevator.current_floor);

    INIT_LIST_HEAD(&move_list);

    // Loop through list of passengers
    list_for_each_safe(temp, next_node, &my_elevator.passengers) {
        printk(KERN_NOTICE "Inside unload loop, %d on board", my_elevator.ppl_on_board);
        pass_temp = list_entry(temp, Passenger, passengers);

        // Check if curr passanger has reached destination floor & delete them/ free memory
        if(pass_temp->dest_floor == my_elevator.current_floor) {
            list_move_tail(temp,&move_list);
            serviced_passengers++;
            my_elevator.ppl_on_board--;
            my_elevator.weight = my_elevator.weight - pass_temp->weight;
            list_del(temp);
            kfree(pass_temp);
        }
      
    }
    printk(KERN_NOTICE "Inside delete/unload loop, %d on board", my_elevator.ppl_on_board);

    // If elevator is empty check for next location to go to
    if(my_elevator.ppl_on_board == 0) {
        while(i < 7) {
              
            if(passengers_waiting[i] > 0) {
                my_elevator.target_floor = i;
                elevator_status_setter();
            }
            i++;
        }
    // Check if building is completely empting -> set elevator to IDLE
        if(passengers_waiting[my_elevator.target_floor] == 0) {
            my_elevator.status = 2;
            none_waiting = 1;
        }
    }

    // If building is not empty, we take first enttry and set their target floor as destination floor
    if(my_elevator.ppl_on_board > 0) {
        Passenger *person;
        person = list_first_entry(&my_elevator.passengers, Passenger, passengers);
        my_elevator.target_floor = person->dest_floor;
        elevator_status_setter();
       }
}



// Output Function; print elevator current stat to proc 
int output_print(void) {

    char *buf = kmalloc(sizeof(char)*800,__GFP_RECLAIM);
    int i;
    Passenger *person;
    struct list_head *temp;
    int pass_waiting = 0;
    if(mutex_lock_interruptible(&my_elevator.lock) == 0) {

        for (i = 1; i < 7; i++) {
            pass_waiting+=passengers_waiting[i];
        }
      
        strcpy(output,"");

        switch(my_elevator.status) {
            case 1:
                strcat(output, "Elevator state: OFFLINE");
                break;
            case 2:
                strcat(output, "Elevator state: IDLE");
                break;
            case 3:
                strcat(output, "Elevator state: LOADING");
                break;
            case 4:
                strcat(output, "Elevator state: UP");
                break;
            case 5:
                strcat(output, "Elevator state: DOWN");
                break;
            default:
                strcat(output, "Elevator state: ERROR??");
                break;
        }

        strcat(output,"\n");

        sprintf(buf,"Current floor: %d\n", my_elevator.current_floor);
        strcat(output, buf);

        sprintf(buf,"Current load: %d lbs\n", my_elevator.weight);
        strcat(output, buf);

        sprintf(buf,"Elevator status: ");
        strcat(output, buf);
        list_for_each(temp, &my_elevator.passengers) {
                person = list_entry(temp, Passenger, passengers);
                switch(person->type) {
                    case 0:
                        strcat(output, "F");
                        break;
                    case 1:
                        strcat(output, "O");
                        break;
                    case 2:
                        strcat(output, "J");
                        break;
                    case 3:
                        strcat(output, "S");
                        break;
                }
                sprintf(buf,"%d ", person->dest_floor);
                strcat(output, buf);
            }
        sprintf(buf,"\n");
        strcat(output, buf);

        for (i = 6; i > 0; i--) {
            strcat(output,"[");

            if(i == my_elevator.current_floor) {
                strcat(output,"*");
            }
            else {
                strcat(output, " ");
            }

            sprintf(buf,"] Floor %d: %d ", i, passengers_waiting[i]);
            strcat(output, buf);
            
            list_for_each(temp, &building[i]) {
                person = list_entry(temp, Passenger, passengers);
                switch(person->type) {
                    case 0:
                        strcat(output, "F");
                        break;
                    case 1:
                        strcat(output, "O");
                        break;
                    case 2:
                        strcat(output, "J");
                        break;
                    case 3:
                        strcat(output, "S");
                        break;
                }
                sprintf(buf,"%d ", person->dest_floor);
                strcat(output, buf);
            }
            strcat(output,"\n");


       }
        sprintf(buf,"\nNumber of passengers: %d\n", my_elevator.ppl_on_board);
        strcat(output, buf);


        sprintf(buf,"Number of passengers waiting: %d\n", pass_waiting);
        strcat(output, buf);


        sprintf(buf,"Number passengers serviced: %d\n", serviced_passengers);
        strcat(output, buf);
     
      
        strcat(output,"\n");
        kfree(buf);

        mutex_unlock(&my_elevator.lock);
    }
    return 0;
}


// Syscalls; functions implement the sytem calls for elevator control
extern long(*STUB_issue_request)(int,int,int);
extern long(*STUB_start_elevator)(void);
extern long(*STUB_stop_elevator)(void);

// ISSUE_REQUEST Syscall Function to issue a request to elevator 
long issue_request(int start_floor, int destination_floor, int type) {
    printk(KERN_NOTICE "%s: the three values are %d %d %d\n",
        __FUNCTION__,start_floor,destination_floor,type);

    // Check if request is valid
    if(start_floor < 1 || start_floor > 6 || destination_floor > 6 ||
        destination_floor < 0 || type > 3 || type < 0) {
        return 1;
    }

    // Check if elevator has been stopped
    //if(stop == 0) {
        Passenger *temp; //Create passenger
        temp = kmalloc(sizeof(Passenger), __GFP_RECLAIM);
        if(mutex_lock_interruptible(&my_elevator.lock) == 0) {
            // Add passenger values
            temp->type = type;
            temp->start_floor = start_floor;
            temp->dest_floor = destination_floor;
            switch(type) {
                case 0:
                    temp->weight = FRESH_WEIGHT;
                    break;
                case 1:
                    temp->weight = SOPH_WEIGHT;
                    break;
                case 2:
                    temp->weight = JUN_WEIGHT;
                    break;
                case 3:
                    temp->weight = SEN_WEIGHT;
                    break;
            }
      
            // my added stuff !!!
            if(my_elevator.status == 1)
            {
                my_elevator.target_floor = temp->start_floor; 
                list_add_tail(&temp->passengers, &building[start_floor]);
                passengers_waiting[start_floor]++;
            }
            // If elevator is IDLE at time of passenger addition
           if(my_elevator.status == 2)
           {
                // Set target floor to passenger's current location to "pick them up"
                my_elevator.target_floor = temp->start_floor; 
                list_add_tail(&temp->passengers, &building[start_floor]);
                passengers_waiting[start_floor]++;
                none_waiting = 0;

                // Update elevator status
                if(my_elevator.current_floor > temp->start_floor) {
                    my_elevator.status = 5;
                }
                else if (my_elevator.current_floor < temp -> start_floor) {
                    my_elevator.status = 4;
                }
                else {
                    my_elevator.status = 3;
                }
            }
           
            // If elevator is moving (aka. not IDLE), we simply add passengers
            else if (my_elevator.status == 3 || my_elevator.status == 4 || my_elevator.status == 5) {

                    none_waiting = 0;
                    list_add_tail(&temp->passengers, &building[start_floor]);
                    passengers_waiting[start_floor]++;  
            }
            mutex_unlock(&my_elevator.lock);
        }
    //} 
    return 0;
}


// START_ELEVATOR Syscall Function
long start_elevator(void) {
    int i;
    int temp_target = 0;
    if(mutex_lock_interruptible(&my_elevator.lock) == 0) {
        //Check if elevator is already running
        if(my_elevator.status != 1) {
            mutex_unlock(&my_elevator.lock);
            return 1;    
        }
        // Check if elevator is offline & continue
        if(my_elevator.status == 1) {
            // Check if stop was called and finish prior processes
            if(stop == 1) {
                for(i = 1; i < 7; i++) {
                    if(passengers_waiting[i] > 0) {
                        temp_target = i;
                    }
                }
            }
            // Since offline, reset base values and set to IDLE
            printk(KERN_NOTICE "%s",__FUNCTION__);
            my_elevator.status = 2;
            my_elevator.ppl_on_board = 0;
            my_elevator.weight = 0;
            my_elevator.current_floor = 1;
            my_elevator.target_floor = 1;
            stop = 0;

            // Manage passenger waiting if stop was called
            if(temp_target > 0) {
                my_elevator.target_floor = temp_target;

                // Update elevator status
                if (my_elevator.target_floor > my_elevator.current_floor) {
                    my_elevator.status = 4;
                }
                else if (my_elevator.target_floor == my_elevator.current_floor) {
                    my_elevator.status = 3;
                }
            }
        }

        mutex_unlock(&my_elevator.lock);
    }

    return 0;
}


// STOP_ELEVATOR syscal function
long stop_elevator(void) {
    printk(KERN_NOTICE "%s",__FUNCTION__);
    // Check mutex lock
    if(mutex_lock_interruptible(&my_elevator.lock) == 0) {
        if(stop == 1) {
            // Elevator is already stopped so stop was unsuccessful
            mutex_unlock(&my_elevator.lock);
            return 1;
        }
        // Set stop to show elevator is stopped
        stop = 1;
        mutex_unlock(&my_elevator.lock);
    }
    // Stop happened succesfully
    return 0;
}

// Function to set kthread for elevator
void elevator_thread_init(struct elevator *elevator_data) {
    mutex_init(&elevator_data->lock);
    elevator_data->elevator_thread=kthread_run(elevator,elevator_data,"ELEVATOR");
}

// Elevator module loading
static int elevator_init(void) {
    int i;
    stop = 0;
    my_elevator.status = 1;

    // Initialize elevator stub functions
    STUB_start_elevator = start_elevator;
    STUB_stop_elevator = stop_elevator;
    STUB_issue_request = issue_request;

    // Initialize elevator passenger list
    INIT_LIST_HEAD(&my_elevator.passengers);
    proc_entry = proc_create("elevator", 0666, NULL, &procfile_fops);

    // Check proc
    if(proc_entry == NULL)
    {
        STUB_start_elevator = NULL;
        STUB_stop_elevator = NULL;
        STUB_issue_request = NULL;
        return -ENOMEM;
    }

    // Initialize the building
    for(i = 0; i < 7; i++)
    {
        INIT_LIST_HEAD(&building[i]);
        passengers_waiting[i] = 0;
    }
    
    // Initalize elevator thread
    elevator_thread_init(&my_elevator);
    if(IS_ERR(my_elevator.elevator_thread)) {
        // Elevator thread initalization failed
        printk(KERN_WARNING "ERROR: elevator thread\n");
        STUB_start_elevator = NULL;
        STUB_stop_elevator = NULL;
        STUB_issue_request = NULL;
        remove_proc_entry("elevator",NULL);
        return PTR_ERR(my_elevator.elevator_thread);
    }

    // Elevator module successfully initalized
    return 0;
}

module_init(elevator_init);

// Elevator module unloading
static void elevator_exit(void) {
    struct list_head *temp;
    struct list_head *next_node;
    struct list_head move_list;
    int i;
    Passenger *pass_temp;

    // Initliaze list used for freeing up memory
    INIT_LIST_HEAD(&move_list);
    
    // Check memory while unloading module
    if(mutex_lock_interruptible(&building_lock) == 0) {
        my_elevator.status = 1;
        for (i = 1; i < 7; i++) {
            // Free up building memory
            if(passengers_waiting[i] > 0) {
                list_for_each_safe(temp, next_node, &building[i]) {
                    pass_temp = list_entry(temp, Passenger, passengers);
                    list_move_tail(temp,&move_list);
                    list_del(temp);
                    kfree(pass_temp);
                    passengers_waiting[i]--;
                }
            }
        }
        if(my_elevator.ppl_on_board > 0) {
            // Free up elevator memory
            list_for_each_safe(temp, next_node, &my_elevator.passengers) {
                pass_temp = list_entry(temp, Passenger, passengers);
                list_move_tail(temp,&move_list);
                list_del(temp);
                kfree(pass_temp);
                my_elevator.ppl_on_board--;
            }
        }
        mutex_unlock(&my_elevator.lock);
    }

    // Stop elevator thread
    kthread_stop(my_elevator.elevator_thread);
    // Remove proc
    remove_proc_entry("elevator", NULL);
    // Destroy locks
    mutex_destroy(&my_elevator.lock);
    mutex_destroy(&building_lock);
    // Set function stubs to null
    STUB_start_elevator = NULL;
    STUB_stop_elevator = NULL;
    STUB_issue_request = NULL;
}

module_exit(elevator_exit);


 