#include "directory.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
void directory_init(int inum, int parent)
{
    // printf("directory_init(%d, %d)\n", inum, parent);
    inode_t *di = get_inode(inum);
    di->refs = 1;
    di->mode = 040755;
    di->size = BLOCK_SIZE / sizeof(directory_entry_t);
    di->block = alloc_block();
    // printf("directory_init(%d, %d): ", inum, parent);
    print_inode(di);
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t);
    directory_entry_t *entries = blocks_get_block(di->block); // get the block
    for (int i = 0; i < num_entries; i++)                     // loop through the entries
    {
        memset(entries[i].name, 0, DIR_NAME_LENGTH);
        memset(entries[i]._reserved, 0, sizeof(entries[i]._reserved));
        entries[i].inum = -1;
    }
    directory_put(di, ".", inum);
    directory_put(di, "..", parent);
    if (parent > 0) // if the parent is not the root
    {
        inode_t *parent_di = get_inode(parent);
        parent_di->refs++; // increment the reference count
    }

    // printf("directory_init(%d, %d): ", inum, parent);
    print_inode(di);
    print_directory(di);
}

int directory_lookup(inode_t *di, const char *name)
{
    int size = di->size;
    int block = di->block;
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t); // number of entries in a block
    directory_entry_t *entries = blocks_get_block(block);
    for (int i = 0; i < num_entries; i++) // loop through the entries
    {
        if (entries[i].inum < 0)
        {
            continue; // if the entry is empty, continue
        }
        print_directory_entry(&entries[i]);
        if (strcmp(entries[i].name, name) == 0)
        {
            return entries[i].inum;
        }
    }
    return -1;
}
int directory_put(inode_t *di, const char *name, int inum)
{
    // printf("directory_put(%d, %s, %d)\n", di->inum, name ? name : "<null>", inum);
    int size = di->size;
    int block = di->block;
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t);
    directory_entry_t *entries = blocks_get_block(block);
    for (int i = 0; i < num_entries; i++)
    {
        // printf("directory_put(%d, %s, %d): entries[%d].inum = %d\n", di->inum, name, inum, i, entries[i].inum);
        // printf("directory_put(%d, %s, %d): entries[%d].name = %s\n", di->inum, name, inum, i, entries[i].name);
        if (entries[i].inum < 0)
        {
            entries[i].inum = inum;
            strncpy(entries[i].name, name, DIR_NAME_LENGTH);
            memset(entries[i]._reserved, 0, sizeof(entries[i]._reserved));
            // printf("directory_put(%d, %s, %d): entries[%d].inum = %d\n", di->inum, name, inum, i, entries[i].inum);
            // printf("directory_put(%d, %s, %d): entries[%d].name = %s\n", di->inum, name, inum, i, entries[i].name);
            return 0;
        }
    }
    return -1;
}
int directory_delete(inode_t *di, const char *name)
{
    int size = di->size;
    int block = di->block;
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t);
    directory_entry_t *entries = blocks_get_block(block);
    for (int i = 0; i < num_entries; i++)
    {
        if (entries[i].inum > 0 && strcmp(entries[i].name, name) == 0)
        {
            entries[i].inum = -1;
            return 0;
        }
    }
    return -1;
}
slist_t *directory_list(inode_t *di)
{
    int size = di->size;
    int block = di->block;
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t);
    directory_entry_t *entries = blocks_get_block(block);
    slist_t *list = slist_cons(".", NULL);
    for (int i = 0; i < num_entries; i++)
    {
        if (entries[i].inum > 0)
        {
            // added this line
            list = slist_cons(entries[i].name, list);
        }
    }
    return list;
}
void print_directory_entry(directory_entry_t *entry)
{
    printf("directory_entry_t{ ");
    printf("name: \"%s\", ", entry->name);
    printf("inum: %d", entry->inum);
    printf(" }\n");
}

void print_directory(inode_t *dd)
{
    // printf("print_directory(%d): ", dd->inum);
    print_inode(dd);

    int size = dd->size;
    int block = dd->block;
    int num_entries = BLOCK_SIZE / sizeof(directory_entry_t);
    directory_entry_t *entries = blocks_get_block(block);
    for (int i = 0; i < num_entries; i++)
    {
        if (entries[i].inum > 0)
        {
            fprintf(stderr, "name: %s, inum: %d\n", entries[i].name, entries[i].inum);
        }
    }
}
int path_to_inum(const char *path)
{
    if (strcmp(path, "/") == 0)
    {
        fprintf(stderr, "path_to_inum(\"%s\") -> %d\n", path, 0);
        return 0;
    }
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");
    int inum = 0;
    while (token != NULL)
    {
        // printf("path_to_inum(\"%s\") : inum = %d, token = \"%s\", ", path, inum, token);
        inode_t *di = get_inode(inum);
        print_inode(di);
        inum = directory_lookup(di, token);

        if (inum < 0)
        {
            fprintf(stderr, "path_to_inum(\"%s\") -> %d\n", path, -1);
            return -1;
        }
        token = strtok(NULL, "/");
    }
    free(path_copy);
    fprintf(stderr, "path_to_inum(\"%s\") -> %d\n", path, inum);
    return inum;
}
int remove_directory_entry(const char *path)
{
    if (strcmp(path, "/") == 0)
    {
        return -1;
    }
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");
    int inum = 0;
    while (token != NULL)
    {
        inode_t *di = get_inode(inum);
        inum = directory_lookup(di, token);
        if (inum < 0)
        {
            return -1;
        }
        token = strtok(NULL, "/");
    }
    free(path_copy);
    inode_t *node = get_inode(inum);
    node->refs--;
    if (node->refs == 0)
    {
        free_inode(inum);
    }
    int result = directory_delete(node, token);
    if (result != 0)
    {
        return result;
    }
    return 0;
}