// 카피백 함수만 구현하면

#include "header.h"

struct blockmap_str {
	int block_map[BLOCKS_PER_NAND];
	int filled_pages[BLOCKS_PER_NAND][PAGES_PER_BLOCK];
	int num_invalid_pages[BLOCKS_PER_NAND];

	int current_block;	 /* current write block */
	int current_page_offset; /* current write page offset in block */
	int free_block_count; /* total free block count */
	int gc_free_block; /* free block index for GC */
};

struct blockmap_stats{
	int copyback_count;
};

struct blockmap_str blockmap_str;
struct blockmap_stats blockmap_stats;

static void blockmap_print_stats(void){
	printf("blockmap_stats: page copyback count:         %d\n",
	       blockmap_stats.copyback_count);
}

static int blockmap_get_pbn(int lbn){
	return blockmap_str.block_map[lbn];
}
static void blockmap_set_pbn(int lbn, int pbn){
	int old_pbn;

	assert(lbn < NUM_LBNS);
	assert(pbn < BLOCKS_PER_NAND);

	old_pbn = blockmap_str.block_map[lbn];
	if (old_pbn < 0)
		blockmap_str.block_map[lbn] = pbn;	
}

static void blockmap_write_op(int lpn, int data){
	int block_idx, page_offset;
	int pbn, lbn;

	page_offset = lpn % PAGES_PER_BLOCK;
	lbn = lpn / NUM_LBNS;
	pbn = blockmap_get_pbn(lbn);
	if(pbn < 0){
		block_idx = blockmap_str.current_block++;
		nand_page_program(block_idx, page_offset, data);
		blockmap_set_pbn(lbn, block_idx);
		blockmap_str.filled_pages[pbn][page_offset] = 1;
	}
	else{
		if(blockmap_str.filled_pages[pbn][page_offset]){
			copy_back();
		}
		else{
			nand_page_program(pbn, page_offset, data);
		}
	}

	if (VERBOSE_MODE)
		printf("%s:%d lpn %d ppn %d\n", __func__, __LINE__, lpn, pbn);
}

static int blockmap_read_op(int lpn){
	int pbn, page_offset, data;
	pbn = blockmap_get_pbn(lpn / PAGES_PER_BLOCK);
	page_offset = lpn % PAGES_PER_BLOCK;

	data = nand_page_read(pbn, page_offset);
	return data;
}

void blockmap_init(struct ftl_operation *op){
	int block_idx, page_idx;

	blockmap_str.current_block = 0;
	blockmap_str.current_page_offset = 0;

	blockmap_str.gc_free_block = BLOCKS_PER_NAND - 1;
	blockmap_str.free_block_count = BLOCKS_PER_NAND - 2;

	for (block_idx = 0; block_idx < BLOCKS_PER_NAND; block_idx++){
		blockmap_str.num_invalid_pages[block_idx] = 0;
		blockmap_str.block_map[page_idx] = -1;
		for(page_idx = 0; page_idx < PAGES_PER_BLOCK; ++page_idx)
			blockmap_str.filled_pages[block_idx][page_idx] = 0;
	}	

	op->write_op = blockmap_write_op;
	op->read_op = blockmap_read_op;
	op->print_stats = blockmap_print_stats;
}
