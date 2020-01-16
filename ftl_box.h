#ifndef __H_FTL_BOX__
#define __H_FTL_BOX__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NOB (151552)
#define PPB (128)
#define NOP ((NOB)*(PPB)) // 19398656
#define PAGESIZE (4)

#define NUMKEY 16777216

#define NIL -1
#define PAGE_READ 0
#define PAGE_WRITE 1
#define BLOCK_ERASE 2

#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x-1)/2


typedef struct flash_block {
	int32_t min_page;
	int32_t *pages;
	int32_t *parent;
	int32_t erase_cnt;
	int32_t valid_num;
} _flash_block;

typedef struct dictionary {
	int32_t block_num[NUMKEY];
	int32_t page_num[NUMKEY];	
} _dictionary;

typedef struct node {
	int data; //the number of valid page in block
	int num; //block_number
} node;

typedef struct minHeap {
	int size;
	node *elem;
} minHeap;

static inline void error_print(int8_t op_type) {
	printf("[ERROR] ");
	switch (op_type) {
		case PAGE_READ:
			printf("flash_page_read error: ");
			break;
		case PAGE_WRITE:
			printf("flash_page_write error: ");
			break;
		case BLOCK_ERASE:
			printf("flash_block_erase error: ");
			break;
	}
}

static inline void range_check(int32_t b_idx, int32_t p_idx, int8_t op_type) {

	if ((b_idx < 0) || (b_idx >= NOB)) {
		error_print(op_type);
		printf("block index range\n");
		abort();
	}
	if (op_type != BLOCK_ERASE) {
		if ((p_idx < 0) || (p_idx >= PPB)) {
			error_print(op_type);
			printf("page index range\n");
			abort();
		}
	}
}

static inline void write_collision(int32_t b_idx, int32_t p_idx) {
	error_print(PAGE_WRITE);
	printf("write collision (b_idx: %d, p_idx: %d)\n", b_idx, p_idx);
	abort();
}

int32_t box_create();
int32_t box_destroy();
int32_t flash_page_write(int32_t b_idx, int32_t p_idx, int32_t value, int32_t trace_key);
int32_t flash_page_read(int32_t b_idx, int32_t p_idx);
int32_t flash_block_erase(int32_t b_idx);
int32_t page_read(int32_t trace_key);
int page_write(int trace_key, int trace_value);
void copy_paste_block(int block_num);
int32_t last_block;
int32_t last_page;
int cycle;
minHeap initMinHeap(int size);
void swap(node *n1, node *n2);
void heapify(minHeap *hp, int i);
void buildMinHeap(minHeap *hp, _flash_block *block, int size);
void insertNode(minHeap *hp, int data, int num);
void deleteNode(minHeap *hp);
void deleteMinHeap(minHeap *hp);
#endif // !__H_FTL_BOX__
