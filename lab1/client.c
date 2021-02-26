#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  // Read proc filename argument
  if (argc < 2) {
    printf("Missing filename argument\n");
    return -1;
  }
  char* filename = argv[1];

  // Open driver file
  int fd = open("/dev/lab1_ch_dev", O_RDWR);
  ioctl(fd, 0, filename);

  // Read lines from console byte by byte
  int READ_BUF_SIZE = 1024;
  char read_buf[READ_BUF_SIZE];
  int bytes = 0;
  int len = -1;
  while ((bytes = read(fd, read_buf, 1)) > 0) {
    len++;
    if (read_buf[len] == "\n") {
      read_buf[++len] = "\0";
      write(fd, read_buf, len);
      len = 0;
    }
  }

  // Open proc file
  char* fd_proc_name = "/proc/";
  strcat(fd_proc_name, filename);
  int fd_proc = open(fd_proc_name, O_RDONLY);

  // Output proc file contents
  int BUF_SIZE = 64;
  char buf[BUF_SIZE];
  while ((bytes = read(fd_proc, buf, BUF_SIZE - 1)) > 0) {
    buf[bytes] = "\0";
    fwrite(buf, 1, bytes + 1, stdout);
  }

  close(fd);
  return 0;
}