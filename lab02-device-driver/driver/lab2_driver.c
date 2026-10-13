#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>      // may ham nhu copy_to_user... kernel <> userspace
#include <linux/cdev.h>
#include <linux/device.h>       //tao sys/class/lab2_class
#include <linux/proc_fs.h>      // tao proc/lab2_info
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>


// =====may cai nay in thong tin khi chay modinfo lab2_driver.ko =============
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02: Simple Character Device Driver");
MODULE_VERSION("1.0");
//=============================================================================


//===== khai bao thong tin cua buffer va du lieu==============================
#define DEVICE_NAME "lab2"
#define CLASS_NAME "lab2_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 240
//=============================================================================




//===== khai bao cac bien =============================================================
static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;                  // cho biet hien co bao nhieu byte hop le
static int open_count = 0;                  // cho biet so lan mo
static struct class *lab2_class = NULL;     // day la con tro den lab2_class trong /sys/class/lab2_class/
static struct device *lab2_device = NULL;   // day la con tro den device cu the la cai driver nay, 
                                            // tuc la trong class co lab2_class, trong do lai co nhieu dev,
                                            // nhung bai lab nay chi co moi lab2_device thoi
                                            // day la cac to chuc class / dev de quan ly
                                            // => /sys/class/lab2_class/lab2_device

static DEFINE_MUTEX(lab2_mutex);            // nay tao mutex de bao ve may data dung chung
static struct proc_dir_entry *proc_entry;   // day la con tro den entry /proc/lab2_info trong lab2
//=========================================================================================




//=========cac ham lien quan proc ====================================================


//===========ham proc dung cho entry /proc/lab2_info =====
static int lab2_proc_show(struct seq_file *m, void *v)          // seq_file la 1 struct co san cua linux,
                                                                // no nhu 1 buffer co manager, de luu thong tin hien tai
                                                                // sau do ta co the in ra khi goi ham show nay
                                                                // ham nay nam o day /proc/lab2_info
{
    seq_printf(m, "LAB-02 Driver Statistics\n");
    seq_printf(m, "======================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number  : %d\n", major_number);
    seq_printf(m, "Buffer size   : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    seq_printf(m, "Open count    : %d\n", open_count);

    if (buffer_len > 0)
        seq_printf(m, "Last data     : [%s]\n", device_buffer);

    return 0;
}
//========================================================


//===========ham 2 proc dung cho entry /proc/lab2_info mo file=======
static int lab2_proc_open(struct inode *inode, struct file *file)   // *file la con tro den entry file lab2_info
                                                                    // lab2_proc_show() ghi du lieu vao seq_file *m
                                                                    // *m nay dang link voi cai *file nen ta co the 
                                                                    // cat thong tin tu cai *file nay
{
    return single_open(file, lab2_proc_show, NULL); 
}
//========================================================


//============struct cho 
static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,        // goi ham lab2_proc_open tao san file de ham show ghi vao sau nay
    .proc_read    = seq_read,           // khi goi proc_read = lab2_proc_show
    .proc_lseek   = seq_lseek,          // thay doi vi tri den 1 character khac
    .proc_release = single_release,
};
//===========================operation struct==============
//=====================end proc====================================================





//=====================ham trong sys/class/lab2_class/lab2/=====================

//=====================================================
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", buffer_len);
}

static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", open_count);
}

static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    if (buffer_len > 0)
        return sprintf(buf, "%s\n", device_buffer);

    return sprintf(buf, "(empty)\n");
}
//==========================================================

//===========cac attribute tuong duong cho 3 ham tren=======
static DEVICE_ATTR_RO(buffer_len);      // linux se them _show => trung ten voi ham show ben tren
static DEVICE_ATTR_RO(open_count);      // 3 att build nay se nam trong sys/class/lab2_class/lab2/, khi cat
static DEVICE_ATTR_RO(last_data);       // thi chung goi dung ham tuong ung ben tren
//==========================================================


// tao struct gom 3 file vao, sau do moi dua vao fops===========
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

static struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};
//===================================================================
//==========================rnd sys===================================================



//===== fops chinh open > write/read > close cua lab2==================

//==================open======================================
static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);
    open_count++;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: device opened (count=%d)\n", open_count);
    return 0;
}
//=============================================================


//=================realease=====================================
static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2_driver: device closed\n");
    return 0;
}
//==============================================================


//======================read===================================
static ssize_t lab2_read(struct file *file, char __user *buf,
                         size_t len, loff_t *offset)
{
    int bytes_read = 0;

    if (*offset >= buffer_len)
        return 0;

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        pr_err("lab2_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;

    pr_info("lab2_driver: sent %d bytes to user\n", bytes_read);

    return bytes_read;
}
//==============================================================


//=========================write===============================
static ssize_t lab2_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE - 1) {
        pr_warn("lab2_driver: write too large, truncating\n");
        len = BUFFER_SIZE - 1;
    }

    mutex_lock(&lab2_mutex);

    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    buffer_len = len;
    device_buffer[buffer_len] = '\0';

    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes: [%s]\n",
            len, device_buffer);

    return len;
}
//===========================================================

//=======struct fops ==========================
static struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .release = lab2_release,
    .read    = lab2_read,
    .write   = lab2_write,
};
//=============================================





//=============file cap module /driver=========
// file init nay se tao may thu khi bat dau goi modele
// dang ky thiet bi bang register_chrdev (tu quan ly major)
// tao lab2_class
//tao lab2_device
// tao may cai lien quan proc
//                       sys

static int __init lab2_init(void)
{
    int ret;

    pr_info("lab2_driver: initializing module\n");

    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);

    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    lab2_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }

    lab2_device = device_create(
        lab2_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }

    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);

    if (ret)
        pr_warn("lab2_driver: sysfs_create_group failed\n");

    pr_info("lab2_driver: loaded, major=%d\n", major_number);

    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_fops);

    if (!proc_entry)
        pr_warn("lab2_driver: failed to create /proc/lab2_info\n");
    else
        pr_info("lab2_driver: /proc/lab2_info created\n");

    return 0;
}
//============================================


//==============exit=======================
//init tao xong, gio truoc khi thoat thi exit thao bo may cai da tao====================
static void __exit lab2_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);

    device_destroy(lab2_class, MKDEV(major_number, 0));
    class_destroy(lab2_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("lab2_driver: module unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);
//======================================================================================

//==========================end init/exit====================================================