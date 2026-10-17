#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>      // may ham nhu copy_to_user... kernel <> userspace
#include <linux/cdev.h>
#include <linux/device.h>       //tao sys/class/asgn1_class
#include <linux/proc_fs.h>      // tao proc/asgn1_info
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/string.h>       // lib dung cho memset reset buffer
//======================asgn1======================
#include <linux/ioctl.h>
#include "asgn1_ioctl.h"        // khai bao cac lenh ioctl cho driver



// =====may cai nay in thong tin khi chay modinfo asgn1_driver.ko =============
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SE200494 - Nguyen Duy Khoa");
MODULE_DESCRIPTION("ASSIGNMENT-01: Character Device Driver with ioctl Interface");
MODULE_VERSION("1.0");


//===== khai bao thong tin cua buffer va du lieu==============================
#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241
#define DRIVER_VERSION "version v1 SE200494"

//===== khai bao cac bien =============================================================
static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static struct class *asgn1_class = NULL;     // day la con tro den asgn1_class trong /sys/class/asgn1_class/
static struct device *asgn1_device = NULL;   // day la con tro den device cu the la cai driver nay, 
                                            // tuc la trong class co asgn1_class, trong do lai co nhieu dev,
                                            // nhung bai lab nay chi co moi asgn1_device thoi
                                            // day la cac to chuc class / dev de quan ly
                                            // => /sys/class/asgn1_class/asgn1_device
static DEFINE_MUTEX(asgn1_mutex);            // nay tao mutex de bao ve may data dung chung
static struct proc_dir_entry *proc_entry;   // day la con tro den entry /proc/asgn1_info trong asgn1


//=====================cac bien thong ke========================================================
static int buffer_len = 0;                  // cho biet hien co bao nhieu byte hop le
static int open_count = 0;                  // cho biet so lan mo
static int write_count = 0;                 // cho biet so lan ghi
static int read_count = 0;                  // cho biet so lan doc
static int last_write_size = 0;
static int current_mode = 1; 















//       A CAC HAM CHINH CUA DRIVER, NHUNG HAM NAY DUOC GOI TU FILE DEVICE /dev/asgn1


//===========ham proc dung cho entry /proc/asgn1_info =====
static int asgn1_proc_show(struct seq_file *m, void *v)          // seq_file la 1 struct co san cua linux,
                                                                // no nhu 1 buffer co manager, de luu thong tin hien tai
                                                                // sau do ta co the in ra khi goi ham show nay
                                                                // ham nay nam o day /proc/asgn1_info
{
    mutex_lock(&asgn1_mutex);
    seq_printf(m, "ASGN1 Driver Statistics\n");
    seq_printf(m, "======================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number  : %d\n", major_number);
    seq_printf(m, "Buffer size   : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    seq_printf(m, "Open count    : %d\n", open_count);
    if (buffer_len > 0)
        seq_printf(m, "Last data     : [%s]\n", device_buffer);

    mutex_unlock(&asgn1_mutex);
    return 0;
}


//===========ham 2 proc dung cho entry /proc/asgn1_info mo file=======
static int asgn1_proc_open(struct inode *inode, struct file *file)   // *file la con tro den entry file asgn1_info
                                                                    // asgn1_proc_show() ghi du lieu vao seq_file *m
                                                                    // *m nay dang link voi cai *file nen ta co the 
                                                                    // cat thong tin tu cai *file nay
{
    return single_open(file, asgn1_proc_show, NULL); 
}





//=====================ham trong sys/class/asgn1_class/asgn1/=====================
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    int value;
    mutex_lock(&asgn1_mutex);
    value = buffer_len;
    mutex_unlock(&asgn1_mutex);

    return sprintf(buf, "%d\n", value);
}

static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    int value;
    mutex_lock(&asgn1_mutex);
    value = open_count;
    mutex_unlock(&asgn1_mutex);

    return sprintf(buf, "%d\n", value);
}

static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    if (buffer_len > 0)
        ret = sprintf(buf, "%s\n", device_buffer);
    else
        ret = sprintf(buf, "(empty)\n");

    mutex_unlock(&asgn1_mutex);

    return ret;
}




//===== fops chinh open > write/read > close cua asgn1==================

//==================open======================================
static int asgn1_open(struct inode *inode, struct file *file)
{
    mutex_lock(&asgn1_mutex);
    open_count++;
    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: device opened (count=%d)\n", open_count);
    return 0;
}


//=================realease=====================================
static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1_driver: device closed\n");
    return 0;
}


//======================read===================================
static ssize_t asgn1_read(struct file *file, char __user *buf,
                         size_t len, loff_t *offset)
{
    int bytes_read = 0;

    mutex_lock(&asgn1_mutex);
    if (*offset >= buffer_len){
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&asgn1_mutex);
        pr_err("asgn1_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;
    read_count++;
    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: sent %d bytes to user\n", bytes_read);

    return bytes_read;
}


//=========================write===============================
static ssize_t asgn1_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    const char kernel_prefix[] = "[KERNEL] ";
    size_t kernel_prefix_len = sizeof(kernel_prefix) - 1;  // tru ky tu /0 neu ko ham sizeof copy luon /0 cua tu [KERNEL] /0

    mutex_lock(&asgn1_mutex);
    if (current_mode == 0) {  // kernel mode

        size_t BUF_SIZE_kernel_mode = BUFFER_SIZE - kernel_prefix_len;

        if (len > BUF_SIZE_kernel_mode -1) {
            pr_warn("asgn1_driver: write too large in kernel mode, truncating\n");
            len = BUF_SIZE_kernel_mode -1;
        }
    }
    else {  // normal mode
        if (len > BUFFER_SIZE - 1 ) {
            pr_warn("asgn1_driver: write too large in normal mode, truncating\n");
            len = BUFFER_SIZE - 1;
        }  
    }


    if (current_mode == 0) {
        memcpy (device_buffer, kernel_prefix, kernel_prefix_len);    // them chu KERNEL vao dau buffer khi ghi tu kernel mode
        if (copy_from_user(device_buffer + kernel_prefix_len, buf, len)) {
            mutex_unlock(&asgn1_mutex);
            return -EFAULT;
        }
        buffer_len = kernel_prefix_len + len;
    }
    else {
        if (copy_from_user(device_buffer, buf, len)) {
            mutex_unlock(&asgn1_mutex);
            return -EFAULT;
        }
        buffer_len = len;
    }


    write_count++;
    last_write_size = len;
    device_buffer[buffer_len] = '\0';

    pr_info("asgn1_driver: received %zu bytes: [%s]\n",
            len, device_buffer);

    mutex_unlock(&asgn1_mutex);
    

    return len;
}


//======them ioctl struct =========================

// luu y cach userspace gui lenh xuong driver 
// ioctl(fd, ASGN1_xxx_xxx, &arg);
// fd la code cua driver, khi mknod, linux tra 1 code dai dien trong driver 
// ASGN1_xxx_xxx la 1 trong 4 cmd
//&arg, user gui add arg xuong, driver co the access truc tiep bien do de xem gia tri la gi



static long asgn1_ioctl(struct file *file,      // day la file device khi goi ioctl, linux tao 1 file cho divice , chinh la no
                        unsigned int cmd,       // day la cmd de phan biet 4 lenh trong group magic 
                        unsigned long arg ){    // ham nay chua tham so, dung long de khi tham so la add thi co du dung luong de chua

    switch(cmd) {
        case ASGN1_RESET_BUFFER: {
            pr_info("asgn1: RESET_BUFFER called\n");
            mutex_lock(&asgn1_mutex);
                memset(device_buffer, 0, sizeof(device_buffer));
                buffer_len = 0;
                write_count = 0;
                read_count = 0;
            mutex_unlock(&asgn1_mutex);
            break;
        }

        case ASGN1_GET_STATS: {
            struct asgn1_stats stats;
            pr_info("asgn1: GET_STATS called\n");

            mutex_lock(&asgn1_mutex);                       // user: ioctl(fd, ASGN1_GET_STATS, &info); arg la 1 pointer
                stats.buffer_len = buffer_len;              // arg = add mem userspace, ta ghi stats vao do
                stats.last_write_size = last_write_size;    // copy_to_user can pointer tro den arg, nen ta ep kieu arg thanh con tro void
                stats.open_count = open_count;              // vi arg = dia chi vung nho userspace
                stats.read_count = read_count;
                stats.write_count = write_count;
            mutex_unlock(&asgn1_mutex);
            if( copy_to_user( (void __user *)arg, &stats, sizeof(stats))) 
                return -EFAULT;
            break;  
        }   

        case ASGN1_SET_MODE: {
            int mode_tmp = 0;           
            pr_info("asgn1: SET_MODE called\n");
            
            if ( copy_from_user( &mode_tmp, (void __user *)arg, sizeof(mode_tmp) ))
                return -EFAULT;
            if (mode_tmp != 0 && mode_tmp != 1)
                return -EINVAL;

            mutex_lock(&asgn1_mutex);
                current_mode = mode_tmp;
            mutex_unlock(&asgn1_mutex);
            break;
        }

        case ASGN1_GET_VERSION:{
                pr_info("asgn1: GET_VERSION called\n");

                if( copy_to_user( (void __user *)arg, DRIVER_VERSION, sizeof(DRIVER_VERSION))) 
                    return -EFAULT;
                break;
         }

        default: return -ENOTTY;

    }
    

    return 0;
}














//              INIT / EXIT CUA MODULE 

//=======struct fops ==========================
static struct file_operations asgn1_fops = {
    .owner          = THIS_MODULE,
    .open           = asgn1_open,
    .release        = asgn1_release,
    .read           = asgn1_read,
    .write          = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl
};
//=============================================

//============struct cho porc entry /proc/asgn1_info=========================
static const struct proc_ops asgn1_proc_fops = {
    .proc_open    = asgn1_proc_open,        // goi ham asgn1_proc_open tao san file de ham show ghi vao sau nay
    .proc_read    = seq_read,           // khi goi proc_read = asgn1_proc_show
    .proc_lseek   = seq_lseek,          // thay doi vi tri den 1 character khac
    .proc_release = single_release,
};
//===========================operation struct==============


//===========cac attribute tuong duong cho 3 ham tren=======
static DEVICE_ATTR_RO(buffer_len);      // linux se them _show => trung ten voi ham show ben tren
static DEVICE_ATTR_RO(open_count);      // 3 att build nay se nam trong sys/class/asgn1_class/asgn1/, khi cat
static DEVICE_ATTR_RO(last_data);       // thi chung goi dung ham tuong ung ben tren
//==========================================================


// tao struct gom 3 file vao, sau do moi dua vao fops===========
static struct attribute *asgn1_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

static struct attribute_group asgn1_attr_group = {
    .attrs = asgn1_attrs,
};
//===================================================================





























//=============file cap module /driver=========
// file init nay se tao may thu khi bat dau goi modele
// dang ky thiet bi bang register_chrdev (tu quan ly major)
// tao asgn1_class
//tao asgn1_device
// tao may cai lien quan proc
//                       sys

static int __init asgn1_init(void)
{
    int ret;

    pr_info("asgn1_driver: initializing module\n");

    ret = register_chrdev(major_number, DEVICE_NAME, &asgn1_fops);

    if (ret < 0) {
        pr_err("asgn1_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(asgn1_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_class);
    }

    asgn1_device = device_create(
        asgn1_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(asgn1_device)) {
        class_destroy(asgn1_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_device);
    }

    ret = sysfs_create_group(&asgn1_device->kobj, &asgn1_attr_group);

    if (ret)
        pr_warn("asgn1_driver: sysfs_create_group failed\n");

    pr_info("asgn1_driver: loaded, major=%d\n", major_number);

    proc_entry = proc_create("asgn1_info", 0444, NULL, &asgn1_proc_fops);

    if (!proc_entry)
        pr_warn("asgn1_driver: failed to create /proc/asgn1_info\n");
    else
        pr_info("asgn1_driver: /proc/asgn1_info created\n");

    return 0;
}


//==============exit=======================
//init tao xong, gio truoc khi thoat thi exit thao bo may cai da tao====================
static void __exit asgn1_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    sysfs_remove_group(&asgn1_device->kobj, &asgn1_attr_group);

    device_destroy(asgn1_class, MKDEV(major_number, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("asgn1_driver: module unloaded\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
