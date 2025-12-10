#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/ktime.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define procfs_name "tsulab"

static struct proc_dir_entry *our_proc_file = NULL;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer,
    size_t buffer_length, loff_t *offset)
{
    char s[32];
    int len;
    time64_t now = ktime_get_real_seconds();
    time64_t perihelion = 859824000; // 1997-04-01 00:00:00
    unsigned long days = (now - perihelion) / (60 * 60 * 24);
    
    len = snprintf(s, sizeof(s), "%lu\n", days);
    
    if (*offset >= len) {
        return 0;
    }
    
    if (copy_to_user(buffer, s, len)) {
        pr_info("copy_to_user failed\n");
        return -EFAULT;
    } else {
        pr_info("procfile read %s\n", filePointer->f_path.dentry->d_name.name);
        *offset += len;
    }
    return len;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init procfs1_init(void)
{
    our_proc_file = proc_create(procfs_name, 0444, NULL, &proc_file_fops);

    if (NULL == our_proc_file) {
        pr_alert("Error: could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;
    }
    pr_info("Welcome to Tomsk State University\n");
    pr_info("/proc/%s was created\n", procfs_name);
    return 0;
}

static void __exit procfs1_exit(void)
{
    if (our_proc_file) {
        proc_remove(our_proc_file);
        pr_info("/proc/%s was removed\n", procfs_name);
        pr_info("Tomsk State University forever!\n");
    }
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");
