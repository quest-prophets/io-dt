#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	// Read proc filename argument
	if (argc < 2)
	{
		printf("Missing filename argument\n");
		return -1;
	}
	char* filename = argv[1];

	// Open driver file
	int fd = open("/dev/lab1_ch_dev", O_RDWR);
	if (ioctl(fd, 0, filename) != 0)
	{
		fprintf(stderr, "ioctl failed: unable to create /proc/%s\n", filename);
		return -1;
	}

	int READ_BUF_SIZE = 1024;
	char read_buf[READ_BUF_SIZE];
	int bytes = 0;
	while ((bytes = read(0, read_buf, READ_BUF_SIZE)) > 0)
	{
		write(fd, read_buf, bytes);
	}

	// Open proc file
	char fd_proc_name[512];
	fd_proc_name[0] = '\0';
	strcat(fd_proc_name, "/proc/");
	strcat(fd_proc_name, filename);

	int fd_proc = open(fd_proc_name, O_RDONLY);
	if (fd_proc < 0)
	{
		fprintf(stderr, "unable to open %s\n", fd_proc_name);
		return -1;
	}

	// Output proc file contents
	char buf[512];
	int r = 0, total_r = 0;
	while ((r = read(fd_proc, buf + total_r, 512)) > 0)
		total_r += r;

	buf[total_r] = '\0';
	puts(buf);

	close(fd);
	return 0;
}
