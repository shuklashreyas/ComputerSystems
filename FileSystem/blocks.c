/**
 * @file blocks.c
 * @author CS3650 staff
 *
 * Implementatino of a block-based abstraction over a disk image file.
 */
#define _GNU_SOURCE
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bitmap.h"
#include "blocks.h"
#include "directory.h"

int blocks_fd = -1;
void *blocks_base = 0;

// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes)
{
  int quo = (bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
  return quo;
}

// Load and initialize the given disk image.
void blocks_init(const char *image_path)
{
  blocks_fd = open(image_path, O_CREAT | O_RDWR, 0644);
  assert(blocks_fd != -1);

  // make sure the disk image is exactly 1MB
  int rv = ftruncate(blocks_fd, NUFS_SIZE);
  assert(rv == 0);

  // map the image to memory
  blocks_base =
      mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
  assert(blocks_base != MAP_FAILED);

  // initialize the superblock
  SuperBlock *sb = blocks_get_block(0);
  if (sb->nblocks == 0)
  {

    void *bbm = get_blocks_bitmap();
    bitmap_put(bbm, 0, 1);

    sb->nblocks = BLOCK_COUNT;
    sb->ninodes = BLOCK_COUNT;
    sb->nlog = 2;
    sb->ndatablocks = BLOCK_COUNT - 2;
    sb->ninodesblocks = 1;
    sb->inodestart = 1;
    sb->datablockstart = 2;
    memset(sb->inodebitmap, 0, BLOCK_BITMAP_SIZE);
    memset(sb->databitmap, 0, BLOCK_BITMAP_SIZE);

    // block 1 stores the inode table
    void *ibm = get_inode_bitmap();
    bitmap_put(bbm, sb->inodestart, 1);

    int root_inum = alloc_inode();
    directory_init(root_inum, root_inum);
  }
  print_superblock(sb);
  inode_t *root = get_inode(0);
  print_directory(root);
}

void print_superblock(SuperBlock *sb)
{
  printf("SuperBlock{\n");
  printf("  nblocks: %d\n", sb->nblocks);
  printf("  ninodes: %d\n", sb->ninodes);
  printf("  nlog: %d\n", sb->nlog);
  printf("  ndatablocks: %d\n", sb->ndatablocks);
  printf("  ninodesblocks: %d\n", sb->ninodesblocks);
  printf("  inodestart: %d\n", sb->inodestart);
  printf("  datablockstart: %d\n", sb->datablockstart);
  printf("}\n");
}

// Close the disk image.
void blocks_free()
{
  int rv = munmap(blocks_base, NUFS_SIZE);
  blocks_base = 0;
  assert(rv == 0);
}

// Get the given block, returning a pointer to its start.
void *blocks_get_block(int bnum) { return blocks_base + BLOCK_SIZE * bnum; }

// Return a pointer to the beginning of the block bitmap.
// The size is BLOCK_BITMAP_SIZE bytes.
void *get_blocks_bitmap()
{
  SuperBlock *superblock = (SuperBlock *)blocks_get_block(0);
  return (void *)(superblock->databitmap);
}

// Return a pointer to the beginning of the inode table bitmap.
void *get_inode_bitmap()
{
  SuperBlock *superblock = (SuperBlock *)blocks_get_block(0);
  return (void *)(superblock->inodebitmap);
}

// Allocate a new block and return its index.
int alloc_block()
{
  void *bbm = get_blocks_bitmap();

  for (int ii = 1; ii < BLOCK_COUNT; ++ii)
  {
    if (!bitmap_get(bbm, ii))
    {
      bitmap_put(bbm, ii, 1);
      printf("+ alloc_block() -> %d\n", ii);
      return ii;
    }
  }

  return -1;
}

// Deallocate the block with the given index.
void free_block(int bnum)
{
  printf("+ free_block(%d)\n", bnum);
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, bnum, 0);
}
