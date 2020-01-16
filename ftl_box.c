#include "ftl_box.h"

_flash_block *flash_blocks;
_dictionary dict;
minHeap hp;


int32_t box_create() {
	cycle = 0;
	last_block = 0;
	last_page = 0;
	flash_blocks = (_flash_block*)malloc(sizeof(_flash_block) * NOB);
	memset(dict.block_num, 0xff, sizeof(dict.block_num));
	memset(dict.page_num, 0xff, sizeof(dict.page_num));
	for (int i=0; i<NOB; i++) {
		flash_blocks[i].min_page = -1;
		flash_blocks[i].pages = (int32_t*)malloc(sizeof(int32_t) * PPB);
		flash_blocks[i].parent = (int32_t*)malloc(sizeof(int32_t) * PPB);
		memset(flash_blocks[i].pages, 0xff, sizeof(int32_t) * PPB);
		memset(flash_blocks[i].pages, 0xff, sizeof(int32_t * PPB);
		flash_blocks[i].valid_num = 0;
	}
}
	
int32_t box_destroy() {
	for (int i=0; i<NOB; i++) {
		free(flash_blocks[i].pages);
	}
	free(flash_blocks);
	return 1;
}

void swap(node *n1, node *n2) {
	node temp = *n1 ;
    *n1 = *n2 ;
    *n2 = temp ;
}

void heapify(minHeap *hp, int i){
	int smallest = (LCHILD(i) < hp->size && hp->elem[LCHILD(i)].data < hp->elem[i].data) ? LCHILD(i) : i ;
    if(RCHILD(i) < hp->size && hp->elem[RCHILD(i)].data < hp->elem[smallest].data) {
        smallest = RCHILD(i) ;
    }
    if(smallest != i) {
        swap(&(hp->elem[i]), &(hp->elem[smallest])) ;
        heapify(hp, smallest) ;
    }
}

void buildMinHeap(minHeap *hp, _flash_block *block, int size){
	int i ;

    // Insertion into the heap without violating the shape property
    for(i = 0; i < size; i++) {
        if(hp->size) {
            hp->elem = realloc(hp->elem, (hp->size + 1) * sizeof(node)) ;
        } else {
            hp->elem = malloc(sizeof(node)) ;
        }
        node nd ;
        nd.data = block[i].valid_num;
		nd.num = i;
        hp->elem[(hp->size)++] = nd ;
    }

    // Making sure that heap property is also satisfied
    for(i = (hp->size - 1) / 2; i >= 0; i--) {
        heapify(hp, i) ;
    }
}

void insertNode(minHeap *hp, int data, int num){
	if(hp->size) {
        hp->elem = realloc(hp->elem, (hp->size + 1) * sizeof(node)) ;
    } else {
        hp->elem = malloc(sizeof(node)) ;
    }

    node nd ;
    nd.data = data;
	nd.num = num;

    int i = (hp->size)++ ;
    while(i && nd.data < hp->elem[PARENT(i)].data) {
        hp->elem[i] = hp->elem[PARENT(i)] ;
        i = PARENT(i) ;
    }
    hp->elem[i] = nd ;
}

void deleteNode(minHeap *hp){
	if(hp->size) {
        printf("Deleting node %d block num %d\n\n", hp->elem[0].data, hp->elem[0].num);
		copy_paste_block(hp->elem[0].num);
        hp->elem[0] = hp->elem[--(hp->size)] ;
        hp->elem = realloc(hp->elem, hp->size * sizeof(node)) ;
        heapify(hp, 0) ;
    } else {
        printf("\nMin Heap is empty!\n") ;
        free(hp->elem) ;
    }
}

void copy_paste_block(int block_num){
	last_block = block_num;
	for(int i = 0; i < PPB; i++){
		int32_t a = flash_block[block_num].parent[i];
		if(dict.block_num[a] == block_num){//this is valid page.
			if(dict.page_num[a] == i){
				flash_page_write(NOB-1, last_page, flash_block[block_num].pages[i],a);
				last_page += 1;
			}
		}
	}
	flash_block_erase(block_num);
	for(int i = 0; i < last_page; i++){
		flash_page_write(block_num, i, flash_block[NOB-1].pages[i],flash_block[NOB-1].parent[i]);
	}
	flash_block_erase(NOB-1);	
}

void deleteMinHeap(minHeap *hp){
	free(hp->elem);
}

int32_t flash_page_read(int32_t b_idx, int32_t p_idx) {
	range_check(b_idx, p_idx, PAGE_READ);
	return flash_blocks[b_idx].pages[p_idx]; // if -1, read empty page..
}

int32_t flash_page_write(int32_t b_idx, int32_t p_idx, int32_t value, int32_t trace_key) {
	range_check(b_idx, p_idx, PAGE_WRITE);
	if (p_idx > flash_blocks[b_idx].min_page) {
		flash_blocks[b_idx].pages[p_idx] = value;
		flash_blocks[b_idx].parent[p_idx] = trace_key;
		flash_blocks[b_idx].min_page = p_idx;
		flash_blocks[b_idx].valid_num += 1;
		return 1;
	}
	else {
		write_collision(b_idx, p_idx);
		return 0;
	}
}


int32_t flash_block_erase(int32_t b_idx) {
	range_check(b_idx, NIL, BLOCK_ERASE);
	flash_blocks[b_idx].min_page = -1;
	memset(flash_blocks[b_idx].pages, 0xff, sizeof(int32_t) * PPB); // Initiailize with -1
	memset(flash_blocks[b_idx].parents, 0xff, sizeof(int32_t) * PPB);
	flash_blocks[b_idx].valid_num = 0;
	return 1;
}

int32_t page_read(int32_t trace_key){ //using hash
	int32_t b_idx = dict.block_num[trace_key];
	int32_t p_idx = dict.page_num[trace_key];
	flash_page_read(b_idx, p_idx);
}

int32_t page_write(int32_t trace_key, int32_t trace_value){ //using hash
	if(cycle == 0){
		if(dict.block_num[trace_key] >= 0 && dict.block_num[trace_key] < NOB){
			if(dict.page_num[trace_key] >= 0 && dict.page_num[trace_key] < PPB){
			flash_blocks[dict.block_num[trace_key]].valid_num -= 1;
			}
		}
		dict.block_num[trace_key] = last_block; 
		dict.page_num[trace_key] = last_page;
		flash_page_write(last_block, last_page, trace_value, trace_key);
		last_page += 1;
		if(last_page == 128){
			last_page = 0;
			last_block += 1;
			if(last_block == NOB -1){
				cycle = 1;
				buildMinHeap(&hp, flash_blocks, NOB-1);
				last_block = 0;
				last_page = 0;
				deleteNode(&hp);
			}
		}
	}
	else{ // when blocks become full -> cycle == 1
		if(last_page == 128){
			
		}
		sleep(1);
	}
}



