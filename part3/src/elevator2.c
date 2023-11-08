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
MODULE_DESCRIPTION("SIMULATES AN ELEVATOR");
/*  These are variables that will be used throughout the module. They can be changed as needed, 
like if the building increased in size or we dealt with different weights. Otherwise, this will 
allow a more clear picture of what is being done throughout the module.*/
#define WEIGHT_CAP 750
#define PASSENGER_CAP 5
#define FRESH_WEIGHT 100
#define SOPH_WEIGHT 150
#define JUN_WEIGHT 200
#define SEN_WEIGHT 250
#define MIN_FLOOR 1
#define MAX_FLOOR 6

/* We need to setup a struct for each passenger. Each passenger will have information stored 
that is important for them. We need to store where they started, where they want to go, the type 
and finally the weight. The last variable is a list head which will allow us to point the passengers 
to each other. This will be important for the elevator, and the building. */
typedef struct passenger {
    int startingFloor;
    int destinationFloor;
    int weight;
    int type;
    struct list_head passengerList;
} Passenger;
/* Next, we will create the struct that will be our Elevator. It will keep track of the current 
location and its goal. It will also need to keep track of the current status of the elevator 
(going up or down for example) and will keep track the total on board and the total weight. 
These variables will be checked against the global varaibles above to make sure they are within 
the allowed range. Finally, we have a list variable so we can store all the passengers that are 
on the elevator and then finally a mutex variable for locking purposes and the variable for the 
kthread.*/
struct thread_parameter {
    int currentLocation;
    int goal;
    int status;
    int totalOnBoard;
    int weight;
    struct list_head passengerList;
    struct task_struct *kthread;
    struct mutex mutex;
};
/* We name our elevator struct here, as well as a list structure for our building. The building 
is an array of lists that goes from 0-10. We ignore index 0 and only deal with 1-10 for the 
floors in the building itself. */
struct thread_parameter Elevator;
struct list_head theBuilding[11];
struct mutex theBuildingMutex;
/* Global variables for various purposes. */
static char *message;                           //char variable to print out our elevator 
                                                //and building if the user calls /proc/elevator

static int read_p;                              //variable for procfile reading

int stop;                                       //variable to determine whether a --stop 
                                                //was issued by the user. If stop == 0 then 
                                                //no, if a --stop has be called then stop will be 
                                                //set to 1

int noPassengersWaiting;                        // an additional "bool" value to check if 
                                                //there are no passengers throughout the building. 
                                                //If so, then this is set to make sure the elevator 
                                                //is set to IDLE.

static struct proc_dir_entry* proc_entry;       //struct to deal with our proc file
int passengerWaitingOnFloor[11];                //a queue of sorts for each floor.
int serviced;                                   //keep track how many passengers have been serviced

/* Functions that will be used further down. */

//This function handles the actual elevator
int my_elevator(void *data); 

//This function moves passengers from the building to the elevator
void loadPassenger(void); 

//This function moves passengers from the elevator and deletes them
void unloadPassenger(void); 

/*Function that sets up our message variable to have the required information 
and formatted correctly.*/
int print_Program(void); 

/* When a user calls our proc file then this function will be called as it is being opened. 
This sets read_p to 1, then sets up the memory for the message variable. */
static int procfile_open(struct inode *inode, struct file *file) {
    read_p = 1;
    message = kmalloc(sizeof(char)*800,__GFP_RECLAIM | __GFP_IO | __GFP_FS);
    if(message == NULL) {
        printk(KERN_WARNING "read_proc_warning");
        return -ENOMEM;
    }
    return print_Program();
    
}
/* Copy the message to the terminal when the proc file is read. */
static ssize_t procfile_read(struct file* file, char __user *buf, size_t size, loff_t *offset) {
    int len = strlen(message);
    read_p = !read_p;
    if(read_p) {
        return 0;
    }    
    copy_to_user(buf, message, len);
    return len;
}
/* When the proc file is released, then we free the memory attached to message. */
int procfile_release(struct inode *sp_inode, struct file *sp_file) {
    kfree(message);
    return 0;
}
/* This struct sets up our file operations. It points it to the specific functions. */
static struct proc_ops elevator_fops = {
    .proc_read = procfile_read,
};

/* For this function we will setup message to be formatted correctly and have the relevant 
information that needs to be printed. */
int print_Program(void) {

    /* First, we will set the memory up for buf so we can store information 
    before copying it to message. */
    char *buf = kmalloc(sizeof(char)*800,__GFP_RECLAIM);
    int i;
    Passenger *p;
    struct list_head *temp;
    int totalWaiting = 0;
    int FreshCounter = 0, SophCounter = 0, JunCounter = 0, SenCounter = 0;
    if(mutex_lock_interruptible(&Elevator.mutex) == 0)
    {
        /* We will cycle through the building and determine how many passengers are 
        waiting throughout the whole building. */
        for (i = 1; i < 11; i++)
        {
            totalWaiting+=passengerWaitingOnFloor[i];
        }
        
        /*  Make sure message is ready for strcat*/
        strcpy(message,"");

        /* Elevator.type is an int variable and each value is tied to one of the 
        states. This if else sequence will make sure the correct state name is 
        copied from the int value. */
        if(Elevator.status == 1)
        {
            strcat(message,"Elevator state: OFFLINE");
        }
        else if(Elevator.status == 2)
        {
            strcat(message,"Elevator state: IDLE");
        }
        else if(Elevator.status == 3)
        {
            strcat(message,"Elevator state: LOADING");
        }
        else if(Elevator.status == 4)
        {
            strcat(message,"Elevator state: UP");
        }
        else if(Elevator.status == 5)
        {
            strcat(message,"Elevator state: DOWN");
        }
        strcat(message,"\n");
        /* Next, we have the current floor of the elevator */
        sprintf(buf,"Current floor: %d\n", Elevator.currentLocation);
        strcat(message, buf);
        /*  The total weight on the elevator*/
        sprintf(buf,"Current weight: %d\n", Elevator.weight);
        strcat(message, buf);

        /* For this part, we will list how many of each animal is on 
        the elevator. We cycle through the elevator list and count how 
        many of each animal there are. Once done, we will print out the 
        values */
        sprintf(buf,"Elevator status: ");
        strcat(message, buf);
        list_for_each(temp, &Elevator.passengerList) {
            p = list_entry(temp, Passenger, passengerList);
            if (p->type == 0) {
                FreshCounter++;
            }
            else if (p->type == 1) {
                SophCounter++;
            }
            else if (p->type == 2) {
                JunCounter++;
            }
            else if (p->type == 3) {
		        SenCounter++;
       	    }}
        sprintf(buf, "%d F, %d SO, %d J, %d S\n",FreshCounter, SophCounter, JunCounter, SenCounter);
        strcat(message, buf);

        /* The number of passengers on the elevator */
        sprintf(buf,"Number of passengers: %d\n", Elevator.totalOnBoard);
        strcat(message, buf);

        /* This is where we print the result from the for loop at the beginning */
        sprintf(buf,"Number of passengers waiting: %d\n", totalWaiting);
        strcat(message, buf);

        /*  Print out how many passengers have been serviced by the elevator.*/
        sprintf(buf,"Number passengers serviced: %d\n", serviced);
        strcat(message, buf);

        /* Now we set up the message to printout the building information*/
        sprintf(buf,"\n\n");
        strcat(message, buf);


        /* This loop will printout each floor and all of the information relevant to each floor. */
        for (i = 10; i > 0; i--)
        {
            strcat(message,"[");

            /* If the elevator is currently on a certain floor, then we will print 
            out a * to showcase this. */
            if(i == Elevator.currentLocation) {
                strcat(message,"*");
            }
            else {
                strcat(message, " ");
            }
            sprintf(buf,"] Floor %d: %d ", i, passengerWaitingOnFloor[i]);
            strcat(message, buf);

            /* This loop will list out all of the passengers on each floor. The setup of each 
            floor is the order in which the passengers arrived. */
            list_for_each(temp, &theBuilding[i]) {
                p = list_entry(temp, Passenger, passengerList);
                if (p->type == 0) {
                    strcat(message,"C ");
                }
                else if (p->type == 1) {
                    strcat(message,"D ");
                }
                else if (p->type == 2) {
                    strcat(message,"L ");
                }
            }
            strcat(message,"\n");

        }
       
        
        strcat(message,"\n");
        /* Free the memeory attached to buf before we leave */
        kfree(buf);

        mutex_unlock(&Elevator.mutex);
    }
    return 0;
}

/* THERE ARE FIVE POSSIBILITIES FOR STATUS
    1 = OFFLINE
    2 = IDLE
    3 = LOADING
    4 = UP
    5 = DOWN*/

/* For this function, it will run the actual elevator. The loop will continue as 
long as the thread is allowed to run. It will check the status and whether or not 
--stop has been called and then call the appropriate functions */
int my_elevator(void *data) {
    struct thread_parameter *parm = data;
    while(!kthread_should_stop()) {
        if(mutex_lock_interruptible(&parm->mutex) == 0) {
            
            if(Elevator.status != 1 && Elevator.status != 2) {
                /* For status 3, we will unload and load passengers. The first step is to 
                unload passengers. We call the function, and the function will run the 
                appropriate checks. */
                if(Elevator.status == 3) {
                    ssleep(1);
                    if(Elevator.totalOnBoard > 0) {
                        unloadPassenger();
                    }
                    /* If stop has been called and stop is set to 1, then we will not load 
                    any passengers. Otherwise, we will load passengers if any are waiting. */
                    if(stop != 1 && passengerWaitingOnFloor[Elevator.currentLocation] > 0) {
                        loadPassenger();
                    }
                    /* If the elevator is empty and stop has not been called then we will search 
                    for a passengers to pick up. This section of code is redundant and provides 
                    an additional check. */
                    if(Elevator.totalOnBoard == 0 && stop != 1 && 
                        Elevator.goal == Elevator.currentLocation && noPassengersWaiting == 0) {

                        if(Elevator.currentLocation < 10) {
                            Elevator.goal = Elevator.currentLocation++;
                            Elevator.status = 4;
                        }
                        else {
                            Elevator.goal = Elevator.currentLocation--;
                            Elevator.status = 5;
                        }
                    }
                    /* If stop has been called, then we take the first passenger from the 
                    elevator and go to where they need to be dropped off. */
                    else if (stop == 1 && Elevator.totalOnBoard != 0) {
                        Passenger *temp;
                        temp = list_first_entry(&Elevator.passengerList, Passenger, passengerList);
                        Elevator.goal = temp->destinationFloor;

                        if(Elevator.goal > Elevator.currentLocation) {
                            Elevator.status = 4;
                        }
                        else {
                            Elevator.status = 5;
                        }
                    }
                }
                /* If the status is UP then we sleep for 2 seconds and then move the elevator. */
                if(Elevator.status == 4) {
                    ssleep(2);
                    Elevator.currentLocation = Elevator.goal;
                    Elevator.status = 3;
                }
                /* Same for the the status of DOWN */
                if(Elevator.status == 5) {
                    ssleep(2);
                    Elevator.currentLocation = Elevator.goal;
                    Elevator.status = 3;
                }
            }
            /* Finally, if the elevator is empty and stop has been called, then we 
            set the elevator the OFFLINE and wait. */
            if(stop == 1 && Elevator.totalOnBoard == 0) {
                Elevator.status = 1;
                Elevator.goal = 1;
                Elevator.currentLocation = 1;
            }
            mutex_unlock(&parm->mutex);
        }
    }

    return 0;
}
/* This function will then handling the loading of passengers. This is done by 
transferring the list element from the building to the elevator. */
void loadPassenger(void){
    
    struct list_head *temp;
	struct list_head *dummy;
    Passenger *passengerTemp;
    
    printk(KERN_NOTICE "INSIDE load current floor %d\n",Elevator.currentLocation);
    
        if(mutex_lock_interruptible(&theBuildingMutex) == 0) {
            if(passengerWaitingOnFloor[Elevator.currentLocation] == 0) {
                mutex_unlock(&theBuildingMutex);
                return;
            }
            /* We will cycle through the list and since we are moving 
            information, we will use the safe search.*/
            list_for_each_safe(temp, dummy, &theBuilding[Elevator.currentLocation]) {
                passengerTemp = list_entry(temp, Passenger, passengerList);

                /* For each passenger on the current floor, we will check to see if 
                the weight of the passenger will put the elevator over the limit. If 
                not, then we move the passenger from the building list to the elevator list. */
                if((passengerTemp->weight + Elevator.weight) < WEIGHT_CAP)
                {
                    Elevator.totalOnBoard++;
                    Elevator.weight = Elevator.weight + passengerTemp->weight;
                    Elevator.goal = passengerTemp->destinationFloor;
                    list_move_tail(temp, &Elevator.passengerList);
                    passengerWaitingOnFloor[Elevator.currentLocation]--;

                    /* We will set the elevator goal to where the passenger wants to go and 
                    change the status. This will certainly happen several times throughout a 
                    floor, but is placed here in order to make sure if one passenger gets on 
                    from a floor, they are handled properly. */
                    if(Elevator.goal > Elevator.currentLocation) {
                        Elevator.status = 4;
                    }
                    else if(Elevator.goal < Elevator.currentLocation) {
                        Elevator.status = 5;
                    }
                }
            }
            mutex_unlock(&theBuildingMutex);
        
        }

}
/* The next function will unload passengers from the elevator. Once this happens, the elevator 
will remove the passenger from the elevator list by deleting it. The passenger does not need to 
exist past this point. Once it is deleted, we will free the memory.*/
void unloadPassenger(void){
    int i = 1;
    struct list_head *temp;
    struct list_head *dummy;
    struct list_head move_list;
    Passenger *passengerTemp;

    if(Elevator.totalOnBoard == 0) {
        return;
    }

    INIT_LIST_HEAD(&move_list);
    printk(KERN_NOTICE "INSIDE unload current floor %d\n",Elevator.currentLocation);
        /* We will cycle through each passenger on the elevator */
        list_for_each_safe(temp, dummy, &Elevator.passengerList) {
            printk(KERN_NOTICE "Inside unload loop, %d on board", Elevator.totalOnBoard);
            passengerTemp = list_entry(temp, Passenger, passengerList);

            /* For each passenger we will check if they need to get off on the current floor. 
            If so, then we will delete them and free the memory that was allocated for it. */
            if(passengerTemp->destinationFloor == Elevator.currentLocation) {
                list_move_tail(temp,&move_list);
                serviced++;
                Elevator.totalOnBoard--;
                Elevator.weight = Elevator.weight - passengerTemp->weight;
                list_del(temp);
                kfree(passengerTemp);
            }
        
        }
    printk(KERN_NOTICE "OUTSIDE unload loop, %d on board", Elevator.totalOnBoard);
    /* If the elevator is empty then we will check the building for the next location we need 
    to go to. We go from the bottom of the building to the top and will pick the highest floor */
        if(Elevator.totalOnBoard == 0) {
            while(i < 11) {
                
                if(passengerWaitingOnFloor[i] > 0) {
                    Elevator.goal = i;
                    if(Elevator.goal > Elevator.currentLocation) {
                        Elevator.status = 4;
                    }
                    else if (Elevator.goal < Elevator.currentLocation) {
                        Elevator.status = 5;
                    }
                }
                i++;
            }
            /* This check is for if there are no passengers at all throughout the building. 
            If that is the case, then the elevator status is set to IDLE and waits. */
            if(passengerWaitingOnFloor[Elevator.goal] == 0) {
                Elevator.status = 2;
                noPassengersWaiting = 1;
            }
        }
        /* If the elevator still has passengers on it, then we take the first entry in the 
        elevator list and set their destination as our goal.  */
        if(Elevator.totalOnBoard > 0)
        {
            Passenger *temp;
            temp = list_first_entry(&Elevator.passengerList, Passenger, passengerList);
            Elevator.goal = temp->destinationFloor;

            if(Elevator.goal > Elevator.currentLocation) {
                Elevator.status = 4;
            }
            else {
                Elevator.status = 5;
            }
        }
}

/* This is our syscall function to issue requests for the elevator. If the request is valid, 
then we will accept them and populate the building with the passengers. */
extern int(*STUB_issue_request)(int,int,int);
int issue_request(int start_floor, int destination_floor, int type) {
	printk(KERN_NOTICE "%s: the three values are %d %d %d\n",
        __FUNCTION__,start_floor,destination_floor,type);

	if(start_floor < 1 || start_floor > 10 || destination_floor > 10 || 
        destination_floor < 0 || type > 3 || type < 0)
	{
		return 1;
	}
    /* We need to make sure that stop has not been called. If so, then we do not need to 
    continue to populate the building.*/
	if(stop == 0) {
		Passenger *temp;
		temp = kmalloc(sizeof(Passenger), __GFP_RECLAIM);
        if(mutex_lock_interruptible(&Elevator.mutex) == 0) {

            temp->type = type;
            temp->startingFloor = start_floor;
            temp->destinationFloor = destination_floor;
            if(type == 0)
            {
                temp->weight = FRESH_WEIGHT;
            }
            else if (type == 1)
            {
                temp->weight = SOPH_WEIGHT;
            }
            else if ( type == 2)
            {
                temp->weight = JUN_WEIGHT;
            }
            else if ( type == 3)
            {
            	temp->weight = SEN_WEIGHT;
    	    }
        
    
            /*If the elevator is idle, then we will set the goal to the first 
            passenger's current location that we accepted into the building. */
            if(Elevator.status == 2)
            {
                Elevator.goal = temp->startingFloor;
                list_add_tail(&temp->passengerList, &theBuilding[start_floor]);
                passengerWaitingOnFloor[start_floor]++;
                noPassengersWaiting = 0;
                if(Elevator.currentLocation > temp->startingFloor)
                {
                    Elevator.status = 5;
                }
                else if (Elevator.currentLocation < temp -> startingFloor)
                {
                    Elevator.status = 4;
                }
                else
                {
                    Elevator.status = 3;
                }
            }
            /* If the elevator is moving, and operating, then we add the 
            passengers, but we don't change anything with the elevator. */
            else if (Elevator.status == 3 || Elevator.status == 4 || Elevator.status == 5) {
                
                    list_add_tail(&temp->passengerList, &theBuilding[start_floor]);
                    passengerWaitingOnFloor[start_floor]++;
                    noPassengersWaiting = 0;
                
            }
            mutex_unlock(&Elevator.mutex);
        }
	}
	return 0;
}
/* This is our syscall for starting the elevator. It will make sure the elevator 
is running and is ready. If the elevator is already functioning, then we return 1. */
extern int(*STUB_start_elevator)(void);
int start_elevator(void) {
    int i;
    int tempGoal = 0;
    if(mutex_lock_interruptible(&Elevator.mutex) == 0) {
        if(Elevator.status != 1)
        {
            mutex_unlock(&Elevator.mutex);
            return 1;
        }
        /* If the elevatorr is offline then we will allow the function to continue. */
        if(Elevator.status == 1) {
            /* If stop was called, then we search the building for a passenger that is waiting. */
            if(stop == 1) {
                for(i = 1; i < 11; i++) {
                    if(passengerWaitingOnFloor[i] > 0) {
                        tempGoal = i;
                    }
                }
            }
            /* Since the elevator was OFFLINE, then we set it to IDLE and make sure 
            the elevator information is set to the starting values. */
            printk(KERN_NOTICE "%s",__FUNCTION__);
            Elevator.currentLocation = 1;
            Elevator.goal = 1;
            Elevator.status = 2;
            Elevator.totalOnBoard = 0;
            Elevator.weight = 0;
            stop = 0;
            /* If stop was called, and a passenger was waiting, then we set the goal 
            of the elevator to pick up the location of one of the passengers. */
            if(tempGoal > 0) {
                Elevator.goal = tempGoal;
                if (Elevator.goal > Elevator.currentLocation) {
                    Elevator.status = 4;
                }
                else if (Elevator.goal == Elevator.currentLocation) {
                    Elevator.status = 3;

                }
            }
        }

        mutex_unlock(&Elevator.mutex);
    }
    
	return 0;
}

/* The syscall to stop the elevator. Stopping the elevator will make the elevator 
go to OFFLINE after it drops off all the passengers in the elevator when the stop 
was called. The elevator will not pick up any more passengers. This will not empty 
the building, but will leave it populated as is until the elevator is called to start 
again. Cleanup of the building is handled when the module is exited. */
extern int(*STUB_stop_elevator)(void);
int stop_elevator(void) {
	printk(KERN_NOTICE "%s",__FUNCTION__);
    if(mutex_lock_interruptible(&Elevator.mutex) == 0) {
        if(stop == 1) {
            mutex_unlock(&Elevator.mutex);
            return 1;
        }
        stop = 1;
        mutex_unlock(&Elevator.mutex);
    }
	
    
	return 0;
}
/* This will set up the kthread, for the elevator. */
void thread_init_parameter(struct thread_parameter *parm) {
    mutex_init(&parm->mutex);
    parm->kthread=kthread_run(my_elevator,parm,"ELEVATOR");
}
/* This will be called when the elevator module is first loaded. It makes sure the 
elevator is setup properly and the syscall functions are linked properly. */
int __init elevator_init(void) {
    int i;
    stop = 0;
    Elevator.status = 1;
    STUB_start_elevator = start_elevator;
	STUB_stop_elevator = stop_elevator;
	STUB_issue_request = issue_request;
    INIT_LIST_HEAD(&Elevator.passengerList);
    proc_entry = proc_create("elevator",0666,NULL,&elevator_fops);
    /* Setup checks in case something could not be loaded. */
    if(proc_entry == NULL)
    {
        STUB_start_elevator = NULL;
        STUB_stop_elevator = NULL;
        STUB_issue_request = NULL;
        return -ENOMEM;
    }
    /* Setup the building */
    for(i = 0; i < 11; i++)
    {
        INIT_LIST_HEAD(&theBuilding[i]);
        passengerWaitingOnFloor[i] = 0;
    }
    /* Call the thread init, and if it fails, then let the user know. */
    thread_init_parameter(&Elevator);
    if(IS_ERR(Elevator.kthread)) {
        printk(KERN_WARNING "ERROR: my_elevator thread\n");
        STUB_start_elevator = NULL;
	    STUB_stop_elevator = NULL;
	    STUB_issue_request = NULL;
        remove_proc_entry("elevator",NULL);
        return PTR_ERR(Elevator.kthread);
    }
    
    return 0;
}

module_init(elevator_init);
/* Exit the module of the elevator. Will remove the syscall pointers 
and clean up any memory that has been called. */
void __exit elevator_exit(void) {
    struct list_head *temp;
    struct list_head *dummy;
    struct list_head move_list;
    int i;
    Passenger *passengerTemp;

    INIT_LIST_HEAD(&move_list);
    /*  Freeing up memory from the building and elevator 
    if the exit module command was given before everything was empty*/
    if(mutex_lock_interruptible(&theBuildingMutex) == 0) {
        Elevator.status = 1;
        for (i = 1; i < 11; i++) {
            if(passengerWaitingOnFloor[i] > 0) {
                list_for_each_safe(temp, dummy, &theBuilding[i]) {
                    
                    passengerTemp = list_entry(temp, Passenger, passengerList);

                    list_move_tail(temp,&move_list);
                    list_del(temp);
                    kfree(passengerTemp);
                    passengerWaitingOnFloor[i]--;
                }
            }
        }
        if(Elevator.totalOnBoard > 0) {
            list_for_each_safe(temp, dummy, &Elevator.passengerList) {
                
                passengerTemp = list_entry(temp, Passenger, passengerList);

                list_move_tail(temp,&move_list);
                list_del(temp);
                kfree(passengerTemp);
                Elevator.totalOnBoard--;
            }
            
        }
        mutex_unlock(&Elevator.mutex);
    }
    kthread_stop(Elevator.kthread);
    remove_proc_entry("elevator", NULL);
    mutex_destroy(&Elevator.mutex);
    mutex_destroy(&theBuildingMutex);
    STUB_start_elevator = NULL;
	STUB_stop_elevator = NULL;
	STUB_issue_request = NULL;
}
module_exit(elevator_exit);