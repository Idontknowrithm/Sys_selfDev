#include "header.h"

struct blockmap_str {
	int block_map[BLOCKS_PER_NAND];

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
	blockmap_str.block_map[lbn] = pbn;
}
static void copy_back(int lbn, int victim_block, int data){
	int i, read_data, copyback_block;

	copyback_block = blockmap_str.gc_free_block;
	// i for copyback_page_offset
	for(i = 0; i < PAGES_PER_BLOCK; ++i){
		read_data = nand_page_read(victim_block, i);
		if(read_data == data)
			--blockmap_stats.copyback_count;
		
		nand_page_program(copyback_block, i, read_data);
		++blockmap_stats.copyback_count;
	}
	nand_block_erase(victim_block);
	blockmap_str.gc_free_block = victim_block;
	blockmap_str.current_block = copyback_block;
	blockmap_set_pbn(lbn, copyback_block);
}
static void blockmap_write_op(int lpn, int data){
	
	int block_idx, page_offset, read_data;
	int pbn, lbn;
	
	page_offset = lpn % PAGES_PER_BLOCK;
	lbn = lpn / PAGES_PER_BLOCK;
	pbn = blockmap_get_pbn(lbn);

	if(pbn < 0){
		block_idx = blockmap_str.current_block++;

		nand_page_program(block_idx, page_offset, data);
		blockmap_set_pbn(lbn, block_idx);
	}
	else{
		if(nand_page_read(pbn, page_offset) != -1){
			copy_back(lbn, pbn, data);
		}
		else{
			nand_page_program(pbn, page_offset, data);
		}
	}
	pbn = blockmap_get_pbn(lbn);

	if (VERBOSE_MODE)
		printf("%s:%d lpn %d pbn %d\n", __func__, __LINE__, lpn, pbn);
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

	for(block_idx = 0; block_idx < BLOCKS_PER_NAND; ++block_idx)
		nand_block_erase(block_idx);

	memset(blockmap_str.block_map, -1, sizeof(blockmap_str.block_map));

	op->write_op = blockmap_write_op;
	op->read_op = blockmap_read_op;
	op->print_stats = blockmap_print_stats;
}
