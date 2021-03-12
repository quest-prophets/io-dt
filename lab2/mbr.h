#define DISK_SIZE_BYTES (50 * 1024 * 1024) // 50MiB

#define MIB_TO_SECTORS(mib) (mib * 1024 * 1024 / 512)

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
	unsigned char start_sec : 6;
	unsigned char start_cyl_hi : 2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec : 6;
	unsigned char end_cyl_hi : 2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable part_table = {
	{
		boot_type : 0,
		start_head : 0,
		start_sec : 0,
		start_cyl_hi : 0,
		start_cyl : 0,
		part_type : 0x83,
		end_head : 0,
		end_sec : 0,
		end_cyl_hi : 0,
		end_cyl : 0,
		abs_start_sec : 1,
		sec_in_part : 20480
	},
	{
		boot_type : 0,
		start_head : 0,
		start_sec : 0,
		start_cyl_hi : 0,
		start_cyl : 0,
		part_type : 0xF,
		end_head : 0,
		end_sec : 0,
		end_cyl_hi : 0,
		end_cyl : 0,
		abs_start_sec : 20481,
		sec_in_part : 81919
	}};

static unsigned int log_part_br_start_sector[] = {20481, 51201};
static const PartTable log_part_table[] = {
	{{
		 boot_type : 0,
		 start_head : 0,
		 start_sec : 0,
		 start_cyl_hi : 0,
		 start_cyl : 0,
		 part_type : 0x83,
		 end_head : 0,
		 end_sec : 0,
		 end_cyl_hi : 0,
		 end_cyl : 0,
		 abs_start_sec : 1,
		 sec_in_part : 30719
	 },
	 {
		 boot_type : 0,
		 start_head : 0,
		 start_sec : 0,
		 start_cyl_hi : 0,
		 start_cyl : 0,
		 part_type : 0x5,
		 end_head : 0,
		 end_sec : 0,
		 end_cyl_hi : 0,
		 end_cyl : 0,
		 abs_start_sec : 30720,
		 sec_in_part : 51199
	 }},
	{{
		 boot_type : 0,
		 start_head : 0,
		 start_sec : 0,
		 start_cyl_hi : 0,
		 start_cyl : 0,
		 part_type : 0x83,
		 end_head : 0,
		 end_sec : 0,
		 end_cyl_hi : 0,
		 end_cyl : 0,
		 abs_start_sec : 1,
		 sec_in_part : 51198
	 },
	 {
		 boot_type : 0,
		 start_head : 0,
		 start_sec : 0,
		 start_cyl_hi : 0,
		 start_cyl : 0,
		 part_type : 0,
		 end_head : 0,
		 end_sec : 0,
		 end_cyl_hi : 0,
		 end_cyl : 0,
		 abs_start_sec : 0,
		 sec_in_part : 0
	 }}};

inline void print_partition_table(void* disk)
{
	int i;
	int logical_offset, extpart_offset;

	for (i = 0; i < 4; ++i)
	{
		PartEntry* e;
		e = (PartEntry*)(disk + PARTITION_TABLE_OFFSET + 16 * i);
		printk(
			KERN_INFO "def_part_table[%d] = {\n"
					  "boot_type : %d,\n"
					  "start_head : %d,\n"
					  "start_sec : %d,\n"
					  "start_cyl_hi : %d,\n"
					  "start_cyl : %d,\n"
					  "part_type : %d,\n"
					  "end_head : %d,\n"
					  "end_sec : %d,\n"
					  "end_cyl_hi : %d,\n"
					  "end_cyl : %d,\n"
					  "abs_start_sec : %d,\n"
					  "sec_in_part : %d\n};\n",
			i, e->boot_type, e->start_head, e->start_sec, e->start_cyl_hi, e->start_cyl, e->part_type, e->end_head,
			e->end_sec, e->end_cyl_hi, e->end_cyl, e->abs_start_sec, e->sec_in_part);
	}

	extpart_offset = ((int)((PartEntry*)(disk + PARTITION_TABLE_OFFSET + 16))->abs_start_sec) * 512;

	logical_offset = extpart_offset + PARTITION_TABLE_OFFSET;
	while (1)
	{
		PartEntry* e;

		printk(
			KERN_INFO "Extended Boot Record Offset = (%d * 512 + %d)\n",
			(logical_offset - PARTITION_TABLE_OFFSET) / 512, PARTITION_TABLE_OFFSET);
		for (i = 0; i < 2; ++i)
		{
			e = (PartEntry*)(disk + logical_offset + 16 * i);
			printk(
				KERN_INFO "def_log_part_table[%d] = {\n"
						  "boot_type : %d,\n"
						  "start_head : %d,\n"
						  "start_sec : %d,\n"
						  "start_cyl_hi : %d,\n"
						  "start_cyl : %d,\n"
						  "part_type : %d,\n"
						  "end_head : %d,\n"
						  "end_sec : %d,\n"
						  "end_cyl_hi : %d,\n"
						  "end_cyl : %d,\n"
						  "abs_start_sec : %d,\n"
						  "sec_in_part : %d\n};\n",
				i, e->boot_type, e->start_head, e->start_sec, e->start_cyl_hi, e->start_cyl, e->part_type, e->end_head,
				e->end_sec, e->end_cyl_hi, e->end_cyl, e->abs_start_sec, e->sec_in_part);
		}
		if (e->abs_start_sec == 0)
			break;
		logical_offset = extpart_offset + ((int)(e->abs_start_sec)) * 512 + PARTITION_TABLE_OFFSET;
	}
}

inline void fill_partition_table(void* disk)
{
	int ebr_i;

	memset(disk, 0, DISK_SIZE_BYTES);

	memcpy(disk + PARTITION_TABLE_OFFSET, &part_table, PARTITION_TABLE_SIZE);
	*(unsigned short*)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;

	for (ebr_i = 0; ebr_i < ARRAY_SIZE(log_part_table); ++ebr_i)
	{
		void* ebr_start = disk + log_part_br_start_sector[ebr_i] * SECTOR_SIZE;
		memcpy(ebr_start + PARTITION_TABLE_OFFSET, &log_part_table[ebr_i], PARTITION_TABLE_SIZE);
		*(unsigned short*)(ebr_start + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
	}
}
