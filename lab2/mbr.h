#define DISK_SIZE_BYTES (50 * 1024 * 1024) // 50MiB

inline void fill_partition_table(void* disk)
{
	// TODO
	memset(disk, 0, DISK_SIZE_BYTES);
}
