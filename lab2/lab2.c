#include <linux/init.h>
#include <linux/module.h>

#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>

MODULE_AUTHOR("P3402");
MODULE_DESCRIPTION("IOS Lab2 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

struct disk_state
{
	spinlock_t queue_lock;
	sector_t num_sectors;
	char* ramdisk;
};

struct gendisk* _gd;

static void handle_queue_request(struct request_queue* q)
{
	struct request* req;
	while ((req = blk_fetch_request(q)) != NULL)
	{
		printk(KERN_NOTICE "lab2: skipping request\n");
		__blk_end_request_all(req, 0);
	}
}

static struct block_device_operations disk_ops = {
	.owner = THIS_MODULE,
};

int __init lab2_init(void)
{
	const size_t disk_size_bytes = 50 * 1024 * 1024; // 50MiB

	int dev_major;
	struct disk_state* state;

	if ((state = vmalloc(sizeof(struct disk_state))) == NULL)
		goto alloc_state_failed;
	if ((dev_major = register_blkdev(0, "lbdv")) < 0)
		goto register_dev_failed;

	// 4 is the maximum number of minors (partitions); the first partition is the entire disk
	if ((_gd = alloc_disk(4)) == NULL)
		goto alloc_disk_failed;

	spin_lock_init(&state->queue_lock);
	if ((_gd->queue = blk_init_queue(handle_queue_request, &state->queue_lock)) == NULL)
		goto init_queue_failed;

	state->num_sectors = disk_size_bytes / SECTOR_SIZE;
	if ((state->ramdisk = vmalloc(disk_size_bytes)) == NULL)
		goto vmalloc_ramdisk_failed;

	_gd->major = dev_major;
	_gd->first_minor = 0;
	_gd->private_data = state;
	_gd->fops = &disk_ops;
	scnprintf(_gd->disk_name, DISK_NAME_LEN, "lbdv%d", _gd->queue->id);
	set_capacity(_gd, state->num_sectors);
	add_disk(_gd);

	printk(KERN_INFO "%s: init successful\n", THIS_MODULE->name);
	return 0;

vmalloc_ramdisk_failed:
	blk_cleanup_queue(_gd->queue);
init_queue_failed:
	del_gendisk(_gd);
	put_disk(_gd);
alloc_disk_failed:
	unregister_blkdev(dev_major, "lbdv");
register_dev_failed:
	vfree(state);
alloc_state_failed:
	return -1;
}

void __exit lab2_exit(void)
{
	del_gendisk(_gd);
	vfree(_gd->private_data);
	blk_cleanup_queue(_gd->queue);
	unregister_blkdev(_gd->major, "lbdv");
	put_disk(_gd);

	printk(KERN_INFO "%s: exit\n", THIS_MODULE->name);
}

module_init(lab2_init);
module_exit(lab2_exit);
