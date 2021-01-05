#include<stdio.h>
#include<string.h>
#include "mymalloc.h"
static unsigned char memory[1024*1024*1];// 1MB size

struct mem_block{
	struct mem_block * next;
	struct mem_block * prev;
	int size;
	unsigned char * block;
};
int mem_block_size=sizeof(struct mem_block);
struct mem_block * free_head;

void init(){
	free_head=(struct mem_block *)memory;
	free_head->next = NULL;
	free_head->prev = NULL;
	free_head->size = sizeof(memory)-sizeof(struct mem_block);
	free_head->block = memory + sizeof(struct mem_block);
}

void myfree(void * free_memory){
	struct mem_block * temp_blk=free_memory - mem_block_size;
	if(temp_blk<free_head){
		temp_blk->next=free_head;
		temp_blk->prev=NULL;
		free_head->prev=temp_blk;
		free_head=temp_blk;
	}
	else if(temp_blk > free_head){
		struct mem_block * hover_free = free_head;
		while(temp_blk>hover_free){
			hover_free=hover_free->next;
		}
		temp_blk->next=hover_free;
		hover_free->prev->next=temp_blk;
		temp_blk->prev=hover_free->prev;
		hover_free->prev=temp_blk;		
	}

	unsigned char * start=temp_blk->block;
	int i = 0;
	while(i < temp_blk->size){
		start[i]=0;
		i++;
	}

	//Coalescing
	if(temp_blk->prev!=NULL){
		if((void *)temp_blk==temp_blk->prev->block+temp_blk->prev->size){
			unsigned char * start=(unsigned char *)temp_blk;
			temp_blk->prev->next=temp_blk->next;
			temp_blk->next->prev=temp_blk->prev;
			temp_blk->prev->size =temp_blk->prev->size + temp_blk->size + mem_block_size;
			temp_blk=temp_blk->prev;
			int i = 0;
			while(i < mem_block_size){
				start[i]=0;
				i++;
			}
		}
	}
	if(temp_blk->next!=NULL){
		if(temp_blk->block == (void *)temp_blk->next - temp_blk->size){
			unsigned char * start=(unsigned char *)temp_blk->next;
			if(temp_blk->next->next != NULL)
				temp_blk->next->next->prev=temp_blk;
			temp_blk->size = temp_blk->size + temp_blk->next->size + mem_block_size;
			temp_blk->next=temp_blk->next->next;
			int i = 0;
			while(i < mem_block_size){
				start[i]=0;
				i++;
			}
		}
	}
}

/*void print_free(){
	struct mem_block * temp_blk=free_head;
	while(temp_blk!=NULL){
		printf("Node  %p \tNext  %p \tPrev  %p \tsize- %d\n",temp_blk,temp_blk->next,temp_blk->prev,temp_blk->size);
		temp_blk=temp_blk->next;
	}printf("\n");
}*/

void * mymalloc(int size){
	struct mem_block * hover_free=free_head;
	struct mem_block * temp_blk;
	int flag_check=0;
	while(hover_free!=NULL && flag_check==0){
		temp_blk=hover_free;
		if(hover_free == free_head && hover_free->size >= size){
			if(hover_free->next != NULL)
				hover_free->next->prev=temp_blk+ (mem_block_size + size);
			free_head = (void *)temp_blk+ (mem_block_size + size);
			free_head->size = temp_blk->size - size - mem_block_size;
			free_head->block = (char *)free_head->block + size + mem_block_size;
			free_head->next=temp_blk->next;
			hover_free=free_head;
			flag_check=1;
		}
		else if(hover_free != free_head && hover_free->size >= size){
			hover_free = (void *)temp_blk + (mem_block_size + size);
			hover_free->next=temp_blk->next;
			hover_free->prev=temp_blk->prev;
			hover_free->size=temp_blk->size - size;
			temp_blk->prev->next=hover_free;
			flag_check=1;
		}
		hover_free = hover_free->next;
	}
	temp_blk->next=NULL;
	temp_blk->prev=NULL;
	temp_blk->size=size;
	temp_blk->block= (unsigned char *)temp_blk + mem_block_size;
	return temp_blk->block;
}
