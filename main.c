#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>

static struct semaphore sema;

unsigned char stateWord =0;
static int num;
static int caps;
static int scroll;
int leds [3];// array of states of lesds

// read status of the controller
unsigned char  kbd_read_status(void)
{
    unsigned char status ;
    status =inb(0x64);
    return status;
}

unsigned char kbd_read_data(void)
{
    unsigned char data;
    // read status register
    unsigned char status = kbd_read_status();
    //cheeck bit 0 of the status reg untile become 1
    while (((status) & 0x01) != 1)
    {
        status = kbd_read_status();
    }
    // read data
    data = inb(0x60);
    return data;
}

void kbd_write_data(unsigned char data)
{
    // read status register
    unsigned char status = kbd_read_status();
    //cheeck bit 1 of the status reg untile become 0
    while (((status) & 0x02) != 0)
    {
        status = kbd_read_status();
    }
    // ready to recive data into input buffer
    outb(data, 0x60);
}

int update_leds(unsigned char led_status_word)
{
    disable_irq(1);
    pid_t pid;

// send ’Set LEDs’ command
    printk(KERN_INFO "pid %d : 2-SENDING 0xED COMMAND.", current->pid);
    kbd_write_data(0xED);

// wait for ACK
    printk(KERN_INFO "pid %d : 3-RECIVED ACK.", current->pid);
    if (kbd_read_data() != 0xFA)
    {
        enable_irq(1);
        return -1;
    }
// sleep for 500 ms
    printk( KERN_INFO"pid %d : 4-SLEPP FOR 500ms.", current->pid);
    msleep(500);
    printk(KERN_INFO "pid %d : 5-WAKE UP",current->pid);

// now send LED states
    printk(KERN_INFO "pid %d : 6-SENDING KEYBOARD DATA.",current->pid);
    kbd_write_data(led_status_word);
// wait for ACK
    printk(KERN_INFO "pid %d : 7-RECEIVED ANOTHER ACK.",current->pid);
    if (kbd_read_data() != 0xFA)
    {
        enable_irq(1);
        return -1;
    }
// success
    printk(KERN_INFO "pid %d : 8-EXIT.",current->pid);
    enable_irq(1);
    return 0;
}


void set_led_state(int led, int state)
{
    down(&sema);// enter critical region
    leds[led]=state;// set state in led[led]
    if(state == 1)// if state is one
    {
        if(led == 0){// scroll lock
            // set first bit is one
            stateWord =(stateWord | 1) ;
        }
        else if(led == 1){// num lock
             // set second bit is one
            stateWord =(stateWord | 2) ;
        }
        else if(led ==2){// caps lock
             // set third bit is one
            stateWord =(stateWord | 4) ;
        }
    }
    else if (state == 0)// if state is one
    {
        if(led == 0){
             // set first bit is zero
            stateWord =(stateWord & 6) ;
        }
        else if(led == 1){
            // set second bit is zero
            stateWord =(stateWord & 5) ;
        }
        else if(led ==2){
            // set third bit is zero
            stateWord =(stateWord & 3) ;
        }
    }
    pid_t pid;// id of current process
    printk(KERN_INFO "pid %d : 1-START   ",current->pid);
    sup(&sema);// exit critical region
    down(&sema);// enter critical region
    update_leds(stateWord);
    up(&sema);// exit critical region
}

int get_led_state(int led)
{
    // return state of led
    return leds[led];

}

static ssize_t num_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
    // get state of num lock
    int state = get_led_state(1);
    // write state of num lock on buffer
    return sprintf(buf, "%d\n", state);
}

static ssize_t num_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,size_t count)
{
    int ret;
    // set state from buffer into num lock
    ret =kstrtoint(buf,10,&num);// convert string to num
    if(ret < 0)
    {
        return ret;
    }
    // set number of led and it is state
    set_led_state(1,num);
    return count;
}

static struct kobj_attribute num_attribute = __ATTR(num, 0664, num_show, num_store);

static ssize_t sc_show (struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
    int var;
    // if attr equal to caps
    if(strcmp(attr->attr.name,"caps")==0)
    {
        // get state of caps lock
        var =get_led_state(2);// caps
    }
    else
    {
        // get state of scroll lock
        var = get_led_state(0);// scroll
    }
     // write state of var lock on buffer
    return sprintf(buf, "%d\n", var);
}
static ssize_t sc_store(struct kobject *kobj, struct kobj_attribute *attr,
                        const char *buf, size_t count)
{
    int var,ret;
    // set state from buffer into var lock
    ret=kstrtoint(buf, 10, &var);  // convert string to number
    if (ret < 0)
    {
        return ret;
    }
    if (strcmp(attr->attr.name, "caps") == 0)
    {
        caps= var;
         // set number of led and it is state
        set_led_state(2,caps);
    }
    else
    {
        scroll = var;
         // set number of led and it is state
        set_led_state(0,scroll);
    }
    return count;
}
static struct kobj_attribute caps_attribute = __ATTR(caps, 0664,sc_show , sc_store);

static struct kobj_attribute scroll_attribute = __ATTR(scroll, 0664, sc_show, sc_store);

static struct attribute *attrs[] =
{
    &num_attribute.attr,
    &caps_attribute.attr,
    &scroll_attribute.attr,
    NULL,
};
static struct attribute_group attr_group =
{
    .attrs = attrs,
};
// initaite semaphore
void init(void)
{
    sema_init(&sema, 1);
}

static struct kobject *module_kobj;

int __init example_init(void)
{
    int retval;
    // creat kobj_module folder contain three files of leds
    module_kobj= kobject_create_and_add("kobj_module",kernel_kobj);
    if (!module_kobj)
    {
        return -ENOMEM;
    }
    retval = sysfs_create_group(module_kobj, &attr_group);
    if (retval)
    {
        kobject_put(module_kobj);
    }
    // call init semaphore
    init();
    return retval;
}
void __exit exit_module(void)
{
    // exit kernel module
    kobject_put(module_kobj);
}

module_init(example_init);
module_exit(exit_module);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Greg Kroah-Hartman <greg@kroah.com>");
