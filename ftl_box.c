#include "ftl_box.h"

_flash_block *flash_blocks;
_dictionary dict;
minHeap hp;


int32_t box_create() {
	cycle = 0;
	last_block = 0;
	last_page = 0;
	flash_blocks = (_flash_block*)malloc(sizeof(_flash_block) * NOB);
	memset(dict.block_num, 0xff, NUMKEY);
	memset(dict.page_num, 0xff, NUMKEY);
	for (int i=0; i<NOB; i++) {
		flash_blocks[i].min_page = -1;
		flash_blocks[i].pages = (int32_t*)malloc(sizeof(int32_t) * PPB);
		memset(flash_blocks[i].pages, 0xff, sizeof(int32_t) * PPB);
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
		printf("arr data : %d\n", block[i].valid_num);
		sleep(2);
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
        printf("Deleting node %d\n\n", hp->elem[0].data) ;
        hp->elem[0] = hp->elem[--(hp->size)] ;
        hp->elem = realloc(hp->elem, hp->size * sizeof(node)) ;
        heapify(hp, 0) ;
    } else {
        printf("\nMin Heap is empty!\n") ;
        free(hp->elem) ;
    }
}

void deleteMinHeap(minHeap *hp){
	free(hp->elem);
}

int32_t flash_page_read(int32_t b_idx, int32_t p_idx) {
	range_check(b_idx, p_idx, PAGE_READ);
	return flash_blocks[b_idx].pages[p_idx]; // if -1, read empty page..
}

int32_t flash_page_write(int32_t b_idx, int32_t p_idx, int32_t value) {
	range_check(b_idx, p_idx, PAGE_WRITE);
	if (p_idx > flash_blocks[b_idx].min_page) {
		flash_blocks[b_idx].pages[p_idx] = value;
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
		flash_page_write(last_block, last_page, trace_value);
		last_page += 1;
		if(flash_blocks[0].valid_num != 128){
			printf("trace key : %d problem occurs!\n", trace_key);
			sleep(1);
		}
		if(last_page == 128){
			last_page = 0;
			last_block += 1;
			if(last_block == NOB -2){
				cycle = 1;
				printf("flash_blocks[0].valid_num : %d\n", flash_blocks[0].valid_num);
				sleep(1);
				buildMinHeap(&hp, flash_blocks, NOB-1);
				printf("complete building!\n");
				sleep(1);
			}
		}
	}
	else{ // when blocks become full -> cycle == 1
		deleteNode(&hp);
		sleep(1);
	}
}



