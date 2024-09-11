#include "inode.h"
#include <stdio.h>
#include "bitmap.h"
void print_inode(inode_t *node)
{
  printf("inode{inum: %d, refs: %d, mode: 0%o, size: %d, block: %d}\n", node->inum, node->refs, node->mode, node->size, node->block);
}

int is_inode_free(int inum)
{
  void *ibm = get_inode_bitmap();
  return bitmap_get(ibm, inum);
}

inode_t *get_inode(int inum)
{
  SuperBlock *superblock = (SuperBlock *)blocks_get_block(0);
  int inode_start = superblock->inodestart;
  int inode_block = inum / (BLOCK_SIZE / sizeof(inode_t));
  inode_t *inode = (inode_t *)blocks_get_block(inode_start + inode_block);
  int inode_offset = inum % (BLOCK_SIZE / sizeof(inode_t));
  printf("+ get_inode(%d) : inode_start = %d, inode_block = %d, inode_offset = %d\n", inum, inode_start, inode_block, inode_offset);
  return inode + inode_offset;
}

int alloc_inode()
{
  void *ibm = get_blocks_bitmap();

  for (int ii = 0; ii < BLOCK_COUNT; ++ii)
  {
    if (!bitmap_get(ibm, ii))
    {
      bitmap_put(ibm, ii, 1);
      printf("+ alloc_inode() -> %d\n", ii);
      return ii;
    }
  }

  return -1;
}

void free_inode(int inum)
{
  printf("+ free_inode(%d)\n", inum);
  void *ibm = get_inode_bitmap();
  bitmap_put(ibm, inum, 0);
}

int grow_inode(inode_t *node, int size)
{
  // Allocate new blocks as needed
  while (node->size < size)
  {
    if (node->size / BLOCK_SIZE < N_DIRECT)
    {
      node->direct[node->size / BLOCK_SIZE] = alloc_block();
    }
    else
    {
      if (node->indirect == 0)
      {
        node->indirect = alloc_block();
      }
      int *indirect_block = blocks_get_block(node->indirect);
      indirect_block[node->size / BLOCK_SIZE - N_DIRECT] = alloc_block();
    }
    node->size += BLOCK_SIZE;
  }
  return 0;
}

int shrink_inode(inode_t *node, int size)
{
  // Deallocate blocks as needed
  while (node->size > size)
  {
    node->size -= BLOCK_SIZE;
    if (node->size / BLOCK_SIZE < N_DIRECT)
    {
      free_block(node->direct[node->size / BLOCK_SIZE]);
      node->direct[node->size / BLOCK_SIZE] = 0;
    }
    else
    {
      int *indirect_block = blocks_get_block(node->indirect);
      free_block(indirect_block[node->size / BLOCK_SIZE - N_DIRECT]);
      indirect_block[node->size / BLOCK_SIZE - N_DIRECT] = 0;
      if (node->size / BLOCK_SIZE == N_DIRECT)
      {
        // All indirect blocks have been deallocated, so deallocate the indirect block itself
        free_block(node->indirect);
        node->indirect = 0;
      }
    }
  }
  return 0;
}

int inode_get_bnum(inode_t *node, int file_bnum)
{
  if (file_bnum < N_DIRECT)
  {
    return node->direct[file_bnum];
  }
  else
  {
    if (node->indirect == 0)
    {
      return -1; // Block not allocated
    }
    int *indirect_block = blocks_get_block(node->indirect);
    return indirect_block[file_bnum - N_DIRECT];
  }
}