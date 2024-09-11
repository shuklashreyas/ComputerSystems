// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "filesystem.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"

char *disc_image_path = "disk.img";

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask)
{
  int rv = 0;

  // Only the root directory and our simulated file are accessible for now...
  if (strcmp(path, "/") == 0 || strcmp(path, "/hello.txt") == 0)
  {
    rv = 0;
  }
  else
  { // ...others do not exist
    rv = -ENOENT;
  }

  printf("access(\"%s\", %04o) -> %d\n", path, mask, rv);
  return rv;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st)
{
  int rv = 0;

  // Return some metadata for the root directory...
  if (strcmp(path, "/") == 0)
  {
    st->st_mode = 040755; // directory
    st->st_size = 0;
    st->st_uid = getuid();
  }
  // ...and the simulated file...
  else if (strcmp(path, "/hello.txt") == 0)
  {
    st->st_mode = 0100644; // regular file
    st->st_size = 6;
    st->st_uid = getuid();
  }
  else
  { // ...other files do not exist on this filesystem
    rv = -ENOENT;
  }
  printf("getattr(\"%s\") -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,
         st->st_size);
  return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  // The filler function is used to add an entry to the readdir output
  // filler(buf, ".", NULL, 0);
  // filler(buf, "..", NULL, 0);

  // Call your list_files function to add the files
  list_files(path, buf, filler, offset, fi);

  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t dev)
{
  int rv = create_file(path);
  if (rv == 0)
  {
    return 0;
  }
  else
  {
    // Return an error code to FUSE
    return -ENOSPC;
  }
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode)
{
  int rv = nufs_mknod(path, mode | 040000, 0);
  printf("mkdir(\"%s\") -> %d\n", path, rv);
  return rv;
}

int nufs_unlink(const char *path)
{
  // Convert the path to an inode number
  int inum = path_to_inum(path);
  if (inum < 0)
  {
    return -ENOENT; // No such file or directory
  }

  // Get the inode
  inode_t *node = get_inode(inum);
  if (node == NULL)
  {
    return -ENOENT; // No such file or directory
  }

  // Decrease the reference count of the inode
  node->refs--;
  if (node->refs == 0)
  {
    // Free the inode
    free_inode(inum);
  }

  // Remove the directory entry for the file
  int result = remove_directory_entry(path);
  if (result != 0)
  {
    return result; // Return the error code from remove_directory_entry
  }

  return 0; // Success
}

int nufs_link(const char *from, const char *to)
{
  int rv = -1;
  printf("link(\"%s\" => \"%s\") -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path)
{
  int rv = -1;
  printf("rmdir(\"%s\") -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *old_path, const char *new_path, unsigned int flags)
{
  int rv = rename_file(old_path, new_path);
  if (rv == 0)
  {
    return 0;
  }
  else
  {

    return -ENOENT;
  }
}
int nufs_chmod(const char *path, mode_t mode)
{
  int rv = -1;
  printf("chmod(\"%s\", %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size)
{
  int rv = truncate_file(path, size);
  printf("truncate(\"%s\", %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi)
{
  int rv = 0;
  printf("open(\"%s\") -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  // Get the inode number for the file
  int inum = path_to_inum(path);
  if (inum < 0)
  {
    return -ENOENT; // No such file or directory
  }

  // Get the inode for the file
  inode_t *node = get_inode(inum);

  if (offset >= node->size)
  {
    return 0;
  }

  size_t bytes_to_read = size;
  if (offset + size > node->size)
  {
    bytes_to_read = node->size - offset;
  }

  // Read the data from the file
  ssize_t bytes_read = read_from_file(path, buf, bytes_to_read, offset);
  if (bytes_read < 0)
  {
    return -EIO;
  }

  printf("read(%s, %ld bytes, @+%ld) -> %ld\n", path, size, offset, bytes_read);
  return bytes_read;
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  ssize_t bytes_written = write_to_file(path, buf, size, offset);
  if (bytes_written >= 0)
  {
    printf("write(\"%s\", %ld bytes, @+%ld) -> %ld\n", path, size, offset, bytes_written);
    return bytes_written;
  }
  else
  {
    // Return an error code to FUSE
    printf("write(\"%s\", %ld bytes, @+%ld) -> %d\n", path, size, offset, -EIO);
    return -EIO;
  }
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2])
{
  int rv = -1;
  printf("utimens(\"%s\", [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data)
{
  int rv = -1;
  printf("ioctl(\"%s\", %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void *nufs_init(struct fuse_conn_info *conn)
{
  printf("Initializing file system...\n");

  // blocks_fd = open("disk.img", O_RDWR);
  // if (blocks_fd == -1) {
  //     perror("open");
  //     exit(EXIT_FAILURE);
  // }

  // blocks_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
  // if (blocks_base == MAP_FAILED) {
  //     perror("mmap");
  //     exit(EXIT_FAILURE);
  // }
  blocks_init(disc_image_path);

  printf("File system initialized successfully.\n");

  return NULL;
}

void nufs_destroy(void *private_data)
{
  printf("destroying\n");
  if (munmap(blocks_base, NUFS_SIZE) == -1)
  {
    perror("munmap");
    exit(EXIT_FAILURE);
  }

  if (close(blocks_fd) == -1)
  {
    perror("close");
    exit(EXIT_FAILURE);
  }
  printf("destroyed\n");
}

void nufs_init_ops(struct fuse_operations *ops)
{
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  // ops->create   = nufs_create; // alternative to mknod
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = (int (*)(const char *, const char *))nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->init = nufs_init;
  ops->destroy = nufs_destroy;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[])
{
  disc_image_path = argv[--argc];

  struct fuse_operations nufs_oper = {
      .access = nufs_access,
      .getattr = nufs_getattr,
      .readdir = nufs_readdir,
      .mknod = nufs_mknod,
      // .create   = nufs_create, // alternative to mknod
      .mkdir = nufs_mkdir,
      .link = nufs_link,
      .unlink = nufs_unlink,
      .rmdir = nufs_rmdir,
      .rename = (int (*)(const char *, const char *))nufs_rename,
      .chmod = nufs_chmod,
      .truncate = nufs_truncate,
      .open = nufs_open,
      .read = nufs_read,
      .init = nufs_init,
      .destroy = nufs_destroy,
      .write = nufs_write,
      .utimens = nufs_utimens,
      .ioctl = nufs_ioctl,
  };

  return fuse_main(argc, argv, &nufs_oper, NULL);
}