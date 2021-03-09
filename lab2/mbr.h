#define DISK_SIZE_BYTES (55 * 1024 * 1024) // 50MiB

#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 
#define PARTITION_TABLE_SIZE 64 

#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55

typedef struct
{
	unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable part_table =
{
	{
		boot_type: 0x0,
		start_sec: 0x0,
		start_head: 0x0,
		start_cyl: 0x0,
		part_type: 0x83,
		end_head: 0x0,
		end_sec: 0x0,
		end_cyl: 0x0,
		abs_start_sec: 0x1,
		sec_in_part: 0x4C4C // 10Mb
	},
	{
		boot_type: 0x0,
		start_head: 0x0,
		start_sec: 0x0,
		start_cyl: 0x0,
		part_type: 0x83,
		end_sec: 0x0,
		end_head: 0x0,
		end_cyl: 0x0,
		abs_start_sec: 0x4C4D,
		sec_in_part: 0x74C22 // 25Mb
	},
        {
                boot_type: 0x0,
                start_head: 0x0,
                start_sec: 0x0,
                start_cyl: 0x0,
                part_type: 0x83,
                end_sec: 0x0,
                end_head: 0x0,
                end_cyl: 0x0,
                abs_start_sec: 0x7986F,
                sec_in_part: 0x4244B // 15 Mb
        }

};

inline void fill_partition_table(void* disk)
{
	memset(disk, 0x0, DISK_SIZE_BYTES);
	memcpy(disk + PARTITION_TABLE_OFFSET, &part_table, PARTITION_TABLE_SIZE);
	*(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
}
