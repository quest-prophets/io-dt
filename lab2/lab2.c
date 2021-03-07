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

static int handle_transfer(struct request* req)
{
	sector_t sector_offset;
	struct disk_state* disk_state;
	struct bio_vec bv;
	struct req_iterator iter;

	sector_offset = blk_rq_pos(req);
	disk_state = req->rq_disk->private_data;

	rq_for_each_segment(bv, req, iter)
	{
		void* req_segment_start;
		void* ramdisk_segment_start;

		if (bv.bv_len % SECTOR_SIZE != 0)
		{
			printk(
				KERN_ERR "%s: IO request size (%u) is not a multiple of sector size\n", THIS_MODULE->name, bv.bv_len);
			return -EIO;
		}

		req_segment_start = page_address(bv.bv_page) + bv.bv_offset;
		ramdisk_segment_start = disk_state->ramdisk + sector_offset * SECTOR_SIZE;

		if (rq_data_dir(req) == WRITE)
			memcpy(ramdisk_segment_start, req_segment_start, bv.bv_len);
		else
			memcpy(req_segment_start, ramdisk_segment_start, bv.bv_len);

		printk(
			KERN_INFO "%s: %s %u bytes starting at segment %lu\n", THIS_MODULE->name,
			rq_data_dir(req) == WRITE ? "written" : "read", bv.bv_len, sector_offset);

		sector_offset += bv.bv_len / SECTOR_SIZE;
	}

	if (sector_offset - blk_rq_pos(req) != blk_rq_sectors(req))
	{
		printk(KERN_ERR "%s: Number of sectors in request does not match bio_vec contents\n", THIS_MODULE->name);
		return -EIO;
	}

	return 0;
}

static void handle_queue_request(struct request_queue* q)
{
	struct request* req;
	while ((req = blk_fetch_request(q)) != NULL)
		__blk_end_request_all(req, handle_transfer(req));
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
	struct disk_state* disk_state;
	disk_state = _gd->private_data;

	blk_cleanup_queue(_gd->queue);

	vfree(disk_state->ramdisk);
	vfree(disk_state);

	unregister_blkdev(_gd->major, "lbdv");

	del_gendisk(_gd);
	put_disk(_gd);

	printk(KERN_INFO "%s: exit\n", THIS_MODULE->name);
}

module_init(lab2_init);
module_exit(lab2_exit);
