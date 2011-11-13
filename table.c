#include "table.h"

#include "squashfuse.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

sqfs_err sqfs_table_init(sqfs_table *table, int fd, off_t start, size_t each,
		size_t count) {
	size_t nblocks = sqfs_divceil(each * count, SQUASHFS_METADATA_SIZE);
	size_t read = nblocks * sizeof(uint64_t);
	
	table->each = each;
	if (!(table->blocks = malloc(read)))
		goto err;
	if (pread(fd, table->blocks, read, start) != read)
		goto err;
	
	int i;
	for (i = 0; i < nblocks; ++i)
		table->blocks[i] = sqfs_swapin64(table->blocks[i]);
	
	return SQFS_OK;
	
err:
	free(table->blocks);
	table->blocks = NULL;
	return SQFS_ERR;
}

void sqfs_table_destroy(sqfs_table *table) {
	free(table->blocks);
	table->blocks = NULL;
}

sqfs_err sqfs_table_get(sqfs_table *table, sqfs *fs, size_t idx, void *buf) {
	size_t pos = idx * table->each;
	size_t bnum = pos / SQUASHFS_METADATA_SIZE,
		off = pos % SQUASHFS_METADATA_SIZE;
	
	off_t bpos = table->blocks[bnum];
	sqfs_block *block;
	if (sqfs_md_block_read(fs, &bpos, &block))
		return SQFS_ERR;
	
	memcpy(buf, (char*)(block->data) + off, table->each);
	sqfs_block_dispose(block);
	return SQFS_OK;
}