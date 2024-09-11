#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "filesystem.h"
#include <sys/types.h>
#include "inode.h"
#include "blocks.h"
#include "directory.h"
#include <stdlib.h>

// Implement the create file function
int create_file(const char *path)
{
  fprintf(stderr, "create_file(\"%s\")\n", path);
  char *path_copy = strdup(path);

  char *this_token = strtok(path_copy, "/");
  char *next_token = strtok(NULL, "/");
  int parent_inum = 0;
  inode_t *parent = get_inode(parent_inum);
  while (next_token != NULL)
  {
    // printf("path_to_inum(\"%s\") : parent_inum = %d, this_token = \"%s\", ", path, parent_inum, this_token);
    print_inode(parent);
    int inum = directory_lookup(parent, this_token);

    if (inum < 0)
    {
      fprintf(stderr, "path_to_inum(\"%s\") -> %d\n", path, -1);
      return -1;
    }
    if ((parent->mode & 040000) != 040000)
    {
      return -1;
    }
    this_token = next_token;        // next_token is not NULL
    next_token = strtok(NULL, "/"); // next_token is NULL if this is the last token
    parent_inum = inum;             // parent_inum is the inum of the parent directory
    parent = get_inode(parent_inum);
  }
  char *file_name = strdup(this_token);

  free(path_copy);
  fprintf(stderr, "path_to_inum(\"%s\") -> %d\n", path, parent_inum);
  int inum = alloc_inode();
  if (inum < 0)
  {
    free(file_name);
    return -1;
  }
  inode_t *node = get_inode(inum); // get the inode of the new file
  node->refs = 1;
  node->mode = 0100644;
  node->size = 0;
  node->block = alloc_block(); // allocate a block for the new file

  if (node->block < 0)
  {
    free(file_name);
    return -1;
  }
  if (directory_put(parent, file_name, inum) < 0) // add the new file to the parent directory
  {
    free(file_name);
    return -1;
  }
  free(file_name);
  return 0;
}

int truncate_file(const char *path, off_t size) // truncate the file to the given size
{
  // printf("truncate_file(\"%s\", %ld bytes)\n", path, size);

  int inum = path_to_inum(path); // get the inode number of the file
  if (inum < 0)
  {
    return create_file(path);
  }
  inode_t *node = get_inode(inum);
  // printf("truncate_file(\"%s\", %ld bytes): inum = %d, ", path, size, inum);
  print_inode(node);
  if (size > node->size) // grow the file if the new size is larger than the current size
  {
    if (grow_inode(node, size - node->size) < 0)
    {
      return -1;
    }
  }
  else if (size < node->size) // shrink the file if the new size is smaller than the current size
  {
    if (shrink_inode(node, node->size - size) < 0)
    {
      return -1;
    }
  }
  return 0;
}

// Implement the list files function
void list_files(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  // printf("list_files(\"%s\")\n", path);
  SuperBlock *sb = blocks_get_block(0); // get the superblock
  print_superblock(sb);
  inode_t *root = get_inode(0);
  print_directory(root);

  int inum = path_to_inum(path); // get the inode number of the file
  if (inum < 0)
  {
    return;
  }
  // printf("list_files(\"%s\"): inum = %d\n", path, inum);
  inode_t *node = get_inode(inum);
  int size = node->size;
  int block = node->block;
  int num_entries = size / sizeof(directory_entry_t);
  // printf("list_files(\"%s\"): ", path);
  print_inode(node);
  directory_entry_t *entries = blocks_get_block(block);
  for (int i = 0; i < num_entries; i++) // iterate over the directory entries
  {
    if (entries[i].inum >= 0) // if the entry is valid
    {
      // printf("list_files(\"%s\"): \"%s\"\n", path, entries[i].name);
      filler(buf, entries[i].name, NULL, 0);
    }
  }
}

// Implement the write to file function
ssize_t write_to_file(const char *path, const char *buf, size_t size, off_t offset)
{
  int inum = path_to_inum(path); // get the inode number of the file
  if (inum < 0)
  {
    return -1;
  }
  inode_t *node = get_inode(inum);
  if (offset + size > node->size)
  { // grow the file if the new size is larger than the current size
    if (grow_inode(node, offset + size) < 0)
    {
      return -1;
    }
  }
  char *data;
  if (offset / BLOCK_SIZE < N_DIRECT)
  { // if the offset is within the direct blocks
    data = blocks_get_block(node->direct[offset / BLOCK_SIZE]);
  }
  else
  {
    int *indirect_block = blocks_get_block(node->indirect);
    data = blocks_get_block(indirect_block[offset / BLOCK_SIZE - N_DIRECT]);
  }
  memcpy(data + offset % BLOCK_SIZE, buf, size); // copy the data to the file
  return size;
}

// Implement the read from file function
ssize_t read_from_file(const char *path, char *buf, size_t size, off_t offset)
{
  int inum = path_to_inum(path);
  if (inum < 0)
  {
    return -1;
  }
  inode_t *node = get_inode(inum);
  if (offset + size > node->size)
  {
    size = node->size - offset;
    if (size < 0)
    {
      return -1;
    }
  }
  char *data;
  if (offset / BLOCK_SIZE < N_DIRECT)
  {
    data = blocks_get_block(node->direct[offset / BLOCK_SIZE]);
  }
  else
  {
    int *indirect_block = blocks_get_block(node->indirect);
    data = blocks_get_block(indirect_block[offset / BLOCK_SIZE - N_DIRECT]);
  }
  memcpy(buf, data + offset % BLOCK_SIZE, size);
  return size;
}

// Implement the rename file function
int rename_file(const char *old_path, const char *new_path)
{
  fprintf(stderr, "rename_file(\"%s\", \"%s\")\n", old_path, new_path); // print the old and new paths
  int old_inum = path_to_inum(old_path);
  if (old_inum < 0)
  {
    return -1;
  }

  char *new_path_copy = strdup(new_path);            // copy the new path
  char *new_file_name = strrchr(new_path_copy, '/'); // get the new file name
  if (new_file_name == NULL)
  {
    free(new_path_copy);
    return -1; // Invalid new path
  }
  *new_file_name = '\0';
  new_file_name++;

  int new_parent_inum = path_to_inum(new_path_copy);
  if (new_parent_inum < 0)
  { // get the inode number of the new parent directory
    free(new_path_copy);
    return -1;
  }
  inode_t *new_parent = get_inode(new_parent_inum);

  char *old_file_name = strrchr(old_path, '/') + 1;
  int old_parent_inum = old_inum;
  inode_t *old_parent = get_inode(old_parent_inum);

  if (directory_delete(old_parent, old_file_name) < 0)
  { // delete the old file from the old parent directory
    free(new_path_copy);
    return -1;
  }

  if (directory_put(new_parent, new_file_name, old_inum) < 0)
  {
    free(new_path_copy);
    return -1;
  }

  free(new_path_copy);
  return 0;
}

// Implement the delete file function
int delete_file(const char *path)
{
  fprintf(stderr, "delete_file(\"%s\")\n", path);

  int inum = path_to_inum(path);
  if (inum < 0)
  {
    return -1;
  }

  inode_t *node = get_inode(inum);
  if (node == NULL)
  {
    return -1;
  }

  char *path_copy = strdup(path);
  char *file_name = strrchr(path_copy, '/');
  if (file_name == NULL)
  {
    free(path_copy);
    return -1;
  }
  *file_name = '\0';
  file_name++;

  int parent_inum = path_to_inum(path_copy);
  if (parent_inum < 0)
  {
    free(path_copy);
    return -1;
  }
  inode_t *parent = get_inode(parent_inum);

  if (directory_delete(parent, file_name) < 0)
  {
    free(path_copy);
    return -1;
  }

  free_inode(inum);
  free_block(node->block);

  free(path_copy);
  return 0;
}

// Below is for extra credit, was not implemented

int rename(const char *old_path, const char *new_path)
{
  fprintf(stderr, "rename(\"%s\", \"%s\")\n", old_path, new_path);

  int old_inum = path_to_inum(old_path); // get the inode number of the old file
  if (old_inum < 0)
  {
    return -1; // Old file or directory does not exist
  }

  // Extract the old and new parent directories and file names
  char *old_parent_path = strdup(old_path);
  char *old_name = strrchr(old_parent_path, '/');
  if (old_name == NULL)
  {
    free(old_parent_path);
    return -1; // Old path is not valid
  }
  *old_name = '\0'; // Split the string to get the old parent directory path
  old_name++;

  char *new_parent_path = strdup(new_path);
  char *new_name = strrchr(new_parent_path, '/');
  if (new_name == NULL)
  {
    free(old_parent_path);
    free(new_parent_path);
    return -1;
  }
  *new_name = '\0'; // Split the string to get the new parent directory path
  new_name++;

  int old_parent_inum = path_to_inum(old_parent_path);
  int new_parent_inum = path_to_inum(new_parent_path);

  if (old_parent_inum < 0 || new_parent_inum < 0)
  {
    free(old_parent_path);
    free(new_parent_path);
    return -1;
  }

  inode_t *old_parent = get_inode(old_parent_inum);
  inode_t *new_parent = get_inode(new_parent_inum);

  if (directory_delete(old_parent, old_name) < 0)
  {
    free(old_parent_path);
    free(new_parent_path);
    return -1; // Could not remove old directory entry
  }

  if (directory_put(new_parent, new_name, old_inum) < 0)
  {
    free(old_parent_path);
    free(new_parent_path);
    return -1; // Could not add new directory entry
  }

  free(old_parent_path);
  free(new_parent_path);
  return 0;
}

int mkdir(const char *path, mode_t mode)
{
  fprintf(stderr, "mkdir(\"%s\", %o)\n", path, mode);

  // Check if the directory already exists
  if (path_to_inum(path) >= 0)
  {
    return -1; // Directory already exists
  }

  // Extract parent directory path and new directory name
  char *path_copy = strdup(path);
  char *dir_name = strrchr(path_copy, '/');
  if (dir_name == NULL)
  {
    free(path_copy);
    return -1; // Invalid path
  }
  *dir_name = '\0';
  dir_name++;

  // Find parent inode number
  int parent_inum = path_to_inum(path_copy);
  if (parent_inum < 0)
  {
    free(path_copy);
    return -1; // Parent directory does not exist
  }
  inode_t *parent = get_inode(parent_inum);

  // Allocate a new inode for the directory
  int inum = alloc_inode();
  if (inum < 0)
  {
    free(path_copy);
    return -1;
  }

  // Initialize the directory inode
  inode_t *dir_inode = get_inode(inum);
  dir_inode->refs = 1;
  dir_inode->mode = mode | 040000; // Directory mode
  dir_inode->size = 2 * sizeof(directory_entry_t);
  dir_inode->block = alloc_block();
  if (dir_inode->block < 0)
  {
    free_inode(inum);
    free(path_copy);
    return -1;
  }

  directory_put(dir_inode, ".", inum);
  directory_put(dir_inode, "..", parent_inum);

  if (directory_put(parent, dir_name, inum) < 0)
  { // Add the directory to its parent directory
    free_inode(inum);
    free_block(dir_inode->block);
    free(path_copy);
    return -1;
  }

  free(path_copy);
  return 0;
}

int read_directory(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  fprintf(stderr, "readdir(\"%s\")\n", path);

  // Find the inode number for the directory
  int dir_inum = path_to_inum(path);
  if (dir_inum < 0)
  {
    return -1;
  }

  // Get the inode for the directory
  inode_t *dir_inode = get_inode(dir_inum);
  if (dir_inode == NULL)
  {
    return -1;
  }

  if ((dir_inode->mode & 040000) != 040000)
  {
    return -1; // Not a directory
  }

  // Iterate over the directory entries
  int num_entries = dir_inode->size / sizeof(directory_entry_t);
  directory_entry_t *entries = blocks_get_block(dir_inode->block);
  for (int i = 0; i < num_entries; i++)
  {
    if (entries[i].inum >= 0)
    {
      if (filler(buf, entries[i].name, NULL, 0) != 0)
      {
        return -1;
      }
    }
  }

  return 0;
}

int rmdir(const char *path)
{
  fprintf(stderr, "rmdir(\"%s\")\n", path);

  // Find the inode number for the directory
  int dir_inum = path_to_inum(path);
  if (dir_inum < 0)
  {
    return -1; // Directory does not exist
  }

  // Get the inode for the directory
  inode_t *dir_inode = get_inode(dir_inum);
  if (dir_inode == NULL)
  {
    return -1;
  }

  // Ensure the inode is a directory
  if ((dir_inode->mode & 040000) != 040000)
  {
    return -1;
  }

  directory_entry_t *entries = blocks_get_block(dir_inode->block);
  int num_entries = dir_inode->size / sizeof(directory_entry_t);
  for (int i = 0; i < num_entries; i++)
  {
    if (entries[i].inum >= 0 && strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0)
    {
      return -1;
    }
  }

  // Extract parent directory path
  char *parent_path = strdup(path);
  char *last_slash = strrchr(parent_path, '/');
  if (last_slash == NULL)
  {
    free(parent_path);
    return -1;
  }
  *last_slash = '\0'; // Split the string to get the parent directory path

  // Find parent inode number
  int parent_inum = path_to_inum(parent_path);
  if (parent_inum < 0)
  {
    free(parent_path);
    return -1; // Parent directory does not exist
  }
  inode_t *parent_inode = get_inode(parent_inum);

  // Remove the directory's entry from its parent directory
  if (directory_delete(parent_inode, last_slash + 1) < 0)
  {
    free(parent_path);
    return -1;
  }

  // Free the inode and associated resources
  free_inode(dir_inum);
  free_block(dir_inode->block);

  free(parent_path);
  return 0;
}

int create_file_in_directory(const char *path)
{
  fprintf(stderr, "create_file_in_directory(\"%s\")\n", path);

  // Split the path into directory path and file name
  char *path_copy = strdup(path);
  char *file_name = strrchr(path_copy, '/');
  if (file_name == NULL)
  {
    free(path_copy);
    return -1;
  }
  *file_name = '\0'; // Separate directory path and file name
  file_name++;

  // Find the parent directory's inode number
  int parent_inum = path_to_inum(path_copy);
  if (parent_inum < 0)
  {
    free(path_copy);
    return -1;
  }

  inode_t *parent = get_inode(parent_inum);

  // Allocate an inode for the new file
  int inum = alloc_inode();
  if (inum < 0)
  {
    free(path_copy);
    return -1;
  }

  // Initialize the inode as a regular file
  inode_t *node = get_inode(inum);
  node->refs = 1;
  node->mode = 0100644;
  node->size = 0;
  node->block = alloc_block();

  if (node->block < 0)
  {
    free_inode(inum);
    free(path_copy);
    return -1;
  }

  // Add the file entry to the parent directory
  if (directory_put(parent, file_name, inum) < 0)
  {
    free_inode(inum);
    free_block(node->block);
    free(path_copy);
    return -1;
  }

  free(path_copy);
  return 0;
}

int move_file_between_directories(const char *old_path, const char *new_path)
{
  fprintf(stderr, "move_file_between_directories(\"%s\", \"%s\")\n", old_path, new_path);

  // Get the inode number of the file
  int file_inum = path_to_inum(old_path);
  if (file_inum < 0)
  {
    return -1;
  }

  // Extract the old and new parent directories and file names
  char *old_dir_path = strdup(old_path);
  char *old_file_name = strrchr(old_dir_path, '/');
  if (old_file_name == NULL)
  {
    free(old_dir_path);
    return -1;
  }
  *old_file_name = '\0'; // Split the string
  old_file_name++;

  char *new_dir_path = strdup(new_path);
  char *new_file_name = strrchr(new_dir_path, '/');
  if (new_file_name == NULL)
  {
    free(old_dir_path);
    free(new_dir_path);
    return -1;
  }
  *new_file_name = '\0';
  new_file_name++;

  // Find the parent directories' inode numbers
  int old_dir_inum = path_to_inum(old_dir_path);
  int new_dir_inum = path_to_inum(new_dir_path);
  if (old_dir_inum < 0 || new_dir_inum < 0)
  {
    free(old_dir_path);
    free(new_dir_path);
    return -1;
  }

  inode_t *old_dir = get_inode(old_dir_inum);
  inode_t *new_dir = get_inode(new_dir_inum);

  // Remove the file from the old directory
  if (directory_delete(old_dir, old_file_name) < 0)
  {
    free(old_dir_path);
    free(new_dir_path);
    return -1;
  }

  // Add the file to the new directory
  if (directory_put(new_dir, new_file_name, file_inum) < 0)
  {
    free(old_dir_path);
    free(new_dir_path);
    return -1;
  }

  free(old_dir_path);
  free(new_dir_path);
  return 0;
}
