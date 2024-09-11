#define FUSE_USE_VERSION 26
#include <fuse.h>
// Define the file structure
struct file
{
  char name[11];
  char data[4096];
  int size;
};

extern struct file filesystem[256];

int create_file(const char *path);
void list_files(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
ssize_t write_to_file(const char *path, const char *buf, size_t size, off_t offset);
ssize_t read_from_file(const char *path, char *buf, size_t size, off_t offset);
int rename_file(const char *old_path, const char *new_path);
int delete_file(const char *path);
int truncate_file(const char *path, off_t size);
int rename(const char *old_path, const char *new_path);
int mkdir(const char *path, mode_t mode);
int read_directory(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int rmdir(const char *path);
int create_file_in_directory(const char *path);
int move_file_between_directories(const char *old_path, const char *new_path);