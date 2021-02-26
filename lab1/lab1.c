#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_AUTHOR("P3402");
MODULE_DESCRIPTION("IOS Lab1 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

const char* CH_DEV_NAME = "lab1_ch_dev";
const char* PROC_FILE_NAME = "var4";

/* === result array === */

struct darray
{
	int* darray_data;
	size_t darray_count;
	size_t darray_capacity;
};

struct darray darray_new(void)
{
	int init_cap = 1;
	return (struct darray){
		.darray_data = vmalloc(init_cap * sizeof(int)),
		.darray_count = 0,
		.darray_capacity = init_cap,
	};
}

void darray_append(int item, struct darray* d)
{
	if (d->darray_count == d->darray_capacity)
	{
		int* new_data;
		d->darray_capacity *= 2;
		new_data = vmalloc(d->darray_capacity * sizeof(int));
		memcpy(new_data, d->darray_data, d->darray_count * sizeof(int));
		vfree(d->darray_data);
		d->darray_data = new_data;
	}
	d->darray_data[d->darray_count++] = item;
}

/* === char device === */

#define BUF_SIZE ((size_t)256)

struct darray ch_result_array;

ssize_t ch_dev_read(struct file* f, char __user* buf, size_t count, loff_t* ppos)
{
	size_t i;
	for (i = 0; i < ch_result_array.darray_count; ++i)
	{
		printk(KERN_DEBUG "%lu) %d\n", i + 1, ch_result_array.darray_data[i]);
	}
	return 0;
}

ssize_t ch_dev_write(struct file* f, const char __user* buf, size_t count, loff_t* ppos)
{
	int num_spaces = 0;

	size_t offset;
	for (offset = 0; offset < count; offset += BUF_SIZE)
	{
		char write_buf[BUF_SIZE];
		size_t i, len;
		len = min(BUF_SIZE, count - offset);

		if (copy_from_user(write_buf, buf + offset, len) != 0)
			return -EFAULT;

		for (i = 0; i < len; ++i)
			if (write_buf[i] == ' ')
				num_spaces++;
	}

	darray_append(num_spaces, &ch_result_array);

	*ppos += count;
	return count;
}

struct file_operations ch_dev_ops = {
	.owner = THIS_MODULE,
	.read = ch_dev_read,
	.write = ch_dev_write,
};

/* === proc device === */

struct proc_state
{
	char read_buf[BUF_SIZE];
	size_t read_i, read_buf_i, read_buf_len;
};

ssize_t proc_read(struct file* f, char __user* buf, size_t count, loff_t* ppos)
{
	struct proc_state* state;
	size_t out_len, snprintf_len;

	state = (struct proc_state*)f->private_data;

	if (state->read_buf_i < state->read_buf_len)
	{
		out_len = min(count, state->read_buf_len - state->read_buf_i);
		if (copy_to_user(buf, state->read_buf, out_len) != 0)
			return -EFAULT;
		state->read_buf_i += out_len;
		*ppos += out_len;
		return out_len;
	}

	if (state->read_i == ch_result_array.darray_count)
		return 0;

	snprintf_len =
		snprintf(state->read_buf, BUF_SIZE, "%lu) %d\n", state->read_i + 1, ch_result_array.darray_data[state->read_i]);
	if (snprintf_len < 0 || snprintf_len >= BUF_SIZE)
		return -EFAULT;

	state->read_i += 1;
	state->read_buf_i = 0;
	state->read_buf_len = snprintf_len;

	out_len = min(count, state->read_buf_len);
	if (copy_to_user(buf, state->read_buf, out_len) != 0)
		return -EFAULT;
	state->read_buf_i += out_len;
	*ppos += out_len;
	return out_len;
}

ssize_t proc_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos)
{
	printk(KERN_DEBUG "Attempt to write proc file");
	return -1;
}

int proc_open(struct inode* i, struct file* f)
{
	struct proc_state* state;
	state = vmalloc(sizeof(struct proc_state));
	memset(state, 0, sizeof(struct proc_state));
	f->private_data = state;

	return 0;
}

int proc_release(struct inode* i, struct file* f)
{
	if (f->private_data != NULL)
		vfree(f->private_data);

	return 0;
}

struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
	.read = proc_read,
	.write = proc_write,
	.open = proc_open,
	.release = proc_release,
};

struct proc_dir_entry* lab1_proc_entry;
dev_t ch_dev_first;
struct cdev ch_dev;
struct class* ch_dev_cls;

int __init lab1_init(void)
{
	ch_result_array = darray_new();

	lab1_proc_entry = proc_create(PROC_FILE_NAME, 0444, NULL, &proc_file_ops);
	if (lab1_proc_entry == NULL)
	{
		printk(KERN_ERR "Unable to create proc file\n");
		return -1;
	}

	if (alloc_chrdev_region(&ch_dev_first, 0 /* first minor */, 1 /* num minor */, "ch_dev") != 0)
	{
		printk(KERN_ERR "Unable to create character device driver\n");
		goto fail_proc_destroy;
	}
	if ((ch_dev_cls = class_create(THIS_MODULE, "chardrv")) == NULL)
		goto fail_region_destroy;
	if (device_create(ch_dev_cls, NULL /* no parent */, ch_dev_first, NULL /* data for callbacks */, CH_DEV_NAME) ==
		NULL)
		goto fail_cls_destroy;
	cdev_init(&ch_dev, &ch_dev_ops);
	if (cdev_add(&ch_dev, ch_dev_first, 1) != 0)
		goto fail_dev_destroy;

	printk(KERN_INFO "%s: initialized\n", THIS_MODULE->name);

	return 0;

fail_dev_destroy:
	device_destroy(ch_dev_cls, ch_dev_first);
fail_cls_destroy:
	class_destroy(ch_dev_cls);
fail_region_destroy:
	unregister_chrdev_region(ch_dev_first, 1);
fail_proc_destroy:
	proc_remove(lab1_proc_entry);
	return -1;
}

void __exit lab1_exit(void)
{
	proc_remove(lab1_proc_entry);
	cdev_del(&ch_dev);
	device_destroy(ch_dev_cls, ch_dev_first);
	class_destroy(ch_dev_cls);
	unregister_chrdev_region(ch_dev_first, 1);
	printk(KERN_INFO "%s: exited\n", THIS_MODULE->name);
}

module_init(lab1_init);
module_exit(lab1_exit);
