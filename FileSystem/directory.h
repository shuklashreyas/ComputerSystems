// Directory manipulation functions.
//
// Feel free to use as inspiration. Provided as-is.

// Based on cs3650 starter code
#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include "blocks.h"
#include "inode.h"
#include "slist.h"

typedef struct directory_entry_t {
  char name[DIR_NAME_LENGTH];
  int inum;
  char _reserved[12];
} directory_entry_t;

void directory_init(int inum, int parentInum);
int directory_lookup(inode_t *di, const char *name);
int directory_put(inode_t *di, const char *name, int inum);
int directory_delete(inode_t *di, const char *name);
slist_t *directory_list(inode_t *di);
void print_directory(inode_t *dd);
int path_to_inum(const char *path);
int remove_directory_entry(const char *path);
void print_directory_entry(directory_entry_t *entry);

#endif
