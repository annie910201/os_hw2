#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
#include <linux/minmax.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS
#endif

#define PROC_MAX_SIZE 2048UL
#define PROCFS_ENTRY_FILENAME "thread_info"

static struct proc_dir_entry *proc_file;
static char proc_buffer[PROC_MAX_SIZE];
static unsigned long proc_buffer_size = 0;

static ssize_t procfs_read(struct file *fp, char __user *buffer, size_t len, loff_t *offset){
    if(*offset || proc_buffer_size ==0){//nothing to read
        // pr_debug("procfs_read: END\n");
        *offset = 0;
        return 0;
    }
    // proc_buffer_size = min(proc_buffer_size,len);
    if(copy_to_user(buffer,proc_buffer,proc_buffer_size))
        return -EFAULT;
    *offset += proc_buffer_size;

    // pr_debug("proc_read: read %lu bytes\n", proc_buffer_size);
    return proc_buffer_size;
}
static ssize_t procfs_write(struct file *fp, const char __user *buffer, size_t len, loff_t *offset)
{
    
    // proc_buffer_size = min(PROC_MAX_SIZE,len);

    /* take data from user mode, and put it in tmp array */
    char tmp[3000];
    if(copy_from_user(tmp,buffer,len))
        return -EFAULT;

    /* create task_truct to get the data of pid. utime. nvcsw. nivcsw */
    struct task_struct *task = get_current();
    int message =0;
    message = sprintf(proc_buffer+proc_buffer_size, "\tThreadID: %d Time: %lld(ms) context switch times: %d\n",task->pid, task->utime/100000, (int)(task->nvcsw + task->nivcsw));
    proc_buffer_size += message;
    *offset += proc_buffer_size;

    return proc_buffer_size;
    
}
static int procfs_open(struct inode *inode, struct file *fp){
    
    try_module_get(THIS_MODULE);
    return 0;
}
static int procfs_close(struct inode *inode, struct file *fp){
    
    module_put(THIS_MODULE);
    return 0;
}

#ifdef HAVE_PROC_OPS
static struct proc_ops op_4_proc = {
    .proc_read = procfs_read,
    .proc_write = procfs_write,
    .proc_open = procfs_open,
    .proc_release = procfs_close,
};
#else
static const struct file_operations op_4_proc = {
    .read = procfs_read,
    .write = procfs_write,
    .open = procfs_open,
    .release = procfs_close,
};
#endif

static int __init procfs3_init(void){
    proc_file = proc_create(PROCFS_ENTRY_FILENAME, 0666, NULL, &op_4_proc);
    if(proc_file == NULL){
        remove_proc_entry(PROCFS_ENTRY_FILENAME, NULL);
        return -ENOMEM;
    }
    
    
    proc_set_size(proc_file, 80);
    proc_set_user(proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);
    return 0;
}

static void __exit procfs3_exit(void){
    remove_proc_entry(PROCFS_ENTRY_FILENAME, NULL);
}

module_init(procfs3_init);
module_exit(procfs3_exit);

MODULE_LICENSE("GPL");

