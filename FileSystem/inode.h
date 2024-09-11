// Inode manipulation routines.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code
#ifndef INODE_H
#define INODE_H
#define N_DIRECT 12
#define N_INDIRECT (BLOCK_SIZE / sizeof(int))

#include "blocks.h"

typedef struct inode
{
  int inum;             // inode number
  int refs;             // reference count
  int mode;             // permission & type
  int size;             // bytes
  int direct[N_DIRECT]; // direct blocks
  int block;            // single block pointer (if max file size <= 4K)
  int indirect;         // single indirect block pointer
} inode_t;

void print_inode(inode_t *node);
int is_inode_free(int inum);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode();
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int file_bnum);
void print_superblock(SuperBlock *sb);

#endif
