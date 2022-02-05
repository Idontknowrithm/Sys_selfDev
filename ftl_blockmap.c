#include "header.h"

struct blockmap_str {
	int block_map[BLOCKS_PER_NAND];

	/* ... */
};

struct blockmap_stats{
	int copyback_count;
};

struct blockmap_str blockmap_str;
struct blockmap_stats blockmap_stats;

static void blockmap_print_stats(void){
	printf("pagemap_stats: page copyback count:         %d\n",
	       blockmap_stats.copyback_count);
}

int blockmap_get_pbn(int lpn){
	return 0;
}

static int blockmap_read_op(int lpn){
	int pbn, page_offset, data;
	pbn = blockmap_get_pbn(lpn);
	page_offset = lpn % PAGES_PER_BLOCK;

	data = nand_page_read(pbn, page_offset);
	return data;
}

static void blockmap_write_op(int lpn, int data){
	/* ... */
}

void blockmap_init(struct ftl_operation *op){
	op->write_op = blockmap_write_op;
	op->read_op = blockmap_read_op;
	op->print_stats = blockmap_print_stats;
}
