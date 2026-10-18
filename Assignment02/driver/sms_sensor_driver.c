/* Educational reference: mock sensor character device, Linux 5.15. */
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "sms_sensor.h"
#define SMS_PROC_NAME "sms_stats"
struct sms_file_state { u64 last_read_ms; unsigned int interval_ms; };
static DEFINE_MUTEX(stats_lock);
static u64 read_count;
static int last_temp_tenths, last_hum_tenths;
static u64 boot_ms;
static struct proc_dir_entry *stats_entry;

static int sms_open(struct inode *inode, struct file *file)
{
    struct sms_file_state *state = kzalloc(sizeof(*state), GFP_KERNEL);
    if (!state) return -ENOMEM;
    state->interval_ms = 100;
    file->private_data = state;
    return 0;
}
static int sms_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    return 0;
}
static ssize_t sms_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    struct sms_file_state *s = file->private_data;
    u64 now = ktime_to_ms(ktime_get());
    int t = 150 + (get_random_u32() % 301);
    int h = 300 + (get_random_u32() % 601);
    char line[96];
  int count;
    if (s->last_read_ms && now - s->last_read_ms < s->interval_ms)
        return -EAGAIN;
    count = scnprintf(line, sizeof(line), "%llu,%d.%01d,%d.%01d\n",
                     (unsigned long long)now, t / 10, t % 10, h / 10, h % 10);
    if (len < count) return -EINVAL;
    if (copy_to_user(buf, line, count)) return -EFAULT;
    s->last_read_ms = now;
    mutex_lock(&stats_lock);
    read_count++;
    last_temp_tenths = t;
    last_hum_tenths = h;
    mutex_unlock(&stats_lock);
    return count;
}
static long sms_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int interval;
    struct sms_file_state *s = file->private_data;
    if (cmd != SMS_SET_INTERVAL) return -ENOTTY;
    if (copy_from_user(&interval, (void __user *)arg, sizeof(interval))) return -EFAULT;
    if (interval < 1 || interval > 60000) return -EINVAL;
    s->interval_ms = interval;
    return 0;
}
static const struct file_operations sms_fops = {
    .owner = THIS_MODULE, .open = sms_open, .release = sms_release,
    .read = sms_read, .unlocked_ioctl = sms_ioctl, .llseek = no_llseek,
};
static struct miscdevice sms_dev = {
    .minor = MISC_DYNAMIC_MINOR, .name = "sms_sensor", .fops = &sms_fops,
    .mode = 0666,
};
static int stats_show(struct seq_file *m, void *v)
{
    u64 count;
    u64 uptime_ms;
    u64 uptime_s;
    int t, h;

    uptime_ms = ktime_to_ms(ktime_get()) - boot_ms;

    uptime_s = div_u64(uptime_ms, 1000);

    mutex_lock(&stats_lock);

    count = read_count;
    t = last_temp_tenths;
    h = last_hum_tenths;

    mutex_unlock(&stats_lock);

    seq_printf(m,
        "read_count: %llu\n"
        "last_temperature: %d.%01d\n"
        "last_humidity: %d.%01d\n"
        "uptime_seconds: %llu\n",
        (unsigned long long)count,
        t / 10, t % 10,
        h / 10, h % 10,
        (unsigned long long)uptime_s
    );

    return 0;
}
static int stats_open(struct inode *inode, struct file *file)
{ return single_open(file, stats_show, NULL); }
static const struct proc_ops stats_ops = {
    .proc_open = stats_open, .proc_read = seq_read,
    .proc_lseek = seq_lseek, .proc_release = single_release,
};
static int __init sms_init(void)
{
    int rc = misc_register(&sms_dev);
    if (rc) return rc;
    boot_ms = ktime_to_ms(ktime_get());
    stats_entry = proc_create(SMS_PROC_NAME, 0444, NULL, &stats_ops);
    if (!stats_entry) { misc_deregister(&sms_dev); return -ENOMEM; }
    pr_info("sms_sensor loaded\n");
    return 0;
}
static void __exit sms_exit(void)
{
    proc_remove(stats_entry);
    misc_deregister(&sms_dev);
    pr_info("sms_sensor unloaded\n");
}
module_init(sms_init);
module_exit(sms_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Educational sensor simulator");
MODULE_AUTHOR("Student reference - replace before submission");
