#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("SE200494 - Nguyen Duy Khoa");
MODULE_DESCRIPTION("Assignment 01 Task 2 - Misc Counter Device Driver");


static atomic_t counter = ATOMIC_INIT(0);
static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);





static ssize_t counter_read(struct file *file,
                            char __user *buf,
                            size_t len,
                            loff_t *ppos)
{
    char temp[32];
    int value;
    int size;

    if (*ppos != 0)             // ppos tranh cat doc vo han lan, khi moi open ppos = 0 nen duoc read bien couter
        return 0;               // sau do ++ ppos => ppos khac 0, nen stop read, => trick tranh cat vo han lan

    value = atomic_read(&counter);
    size = snprintf(temp, sizeof(temp), "%d\n", value); // bien value tu int => string, va luu string va temp
    if (len < size)
    return -EINVAL;

    if (copy_to_user(buf, temp, size))   return -EFAULT;

    atomic_inc(&counter);
    atomic_inc(&read_count);
    *ppos += size;

    return size;
}




static ssize_t counter_write(struct file *file,
                             const char __user *buf,
                             size_t len,
                             loff_t *ppos)
{
    char temp[32];
    int value;

    if (len >= sizeof(temp))            // check doi khi len yeu cau lon hon buffer nen can bao loi
    return -EINVAL;

    if (copy_from_user(temp, buf, len))
    return -EFAULT;

    temp[len] = '\0';

    if (kstrtoint(temp, 10, &value) < 0)      // chuyen chuoi nhan tu user temp sang he thap phan va luu value
    return -EINVAL;

    if (value < 0)                         // kt so am
    return -EINVAL;

    atomic_set(&counter, value);
    atomic_inc(&write_count);

    return len;
}




static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read  = counter_read,
    .write = counter_write,
};

static struct miscdevice counter_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,        // ticap phat minor tu dong
    .name  = "counter",
    .fops  = &counter_fops,
};



//============= ham cho proc/counter_info

static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Counter value : %d\n", atomic_read(&counter));
    seq_printf(m, "Read count    : %d\n", atomic_read(&read_count));
    seq_printf(m, "Write count   : %d\n", atomic_read(&write_count));

    return 0;
}


static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}


static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};





static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_miscdev);      // misdevice dung cho character dev, no giup chung ta dk don gian hon
                                                // ko can class create/ device create../ mac dinh misc co major = 10
    if (ret) {
        pr_err("counter_driver: misc_register failed\n");
        return ret;
    }

    if (!proc_create("counter_info", 0444, NULL, &counter_proc_ops)) {      // 0444 la read only
        pr_err("counter_driver: proc_create failed\n");
        misc_deregister(&counter_miscdev);
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded\n");
    return 0;
}


static void __exit counter_exit(void)
{
    remove_proc_entry("counter_info", NULL);
    misc_deregister(&counter_miscdev);

    pr_info("counter_driver: unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);