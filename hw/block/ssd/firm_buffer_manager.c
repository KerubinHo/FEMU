// File: firm_buffer_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "common.h"
#include <pthread.h>

struct request *request_queue;
struct request *request_tail;
struct sub_request *subs_r_head;
struct sub_request *subs_w_head;
struct sub_request *subs_r_tail;
struct sub_request *subs_w_tail;
unsigned int request_queue_length;
struct buffer_info *buffer;
struct entry *map_entry;

void INIT_IO_BUFFER(struct ssdstate *ssd)
{
	//printf("INIT BUFFER");
    unsigned int page_num;
    request_queue = NULL;
    request_tail = NULL;
    request_queue_length = 0;;

    page_num = ssd->ssdparams.PAGES_IN_SSD;

    buffer = (tAVLTree *)avlTreeCreate((void*)keyCompareFunc , (void *)freeFunc);
    buffer->max_buffer_sector=ssd->ssdparams.WRITE_BUFFER_FRAME_NB;
    map_entry = (struct entry *)malloc(sizeof(struct entry) * page_num);
    memset(map_entry,0,sizeof(struct entry) * page_num);
}

void TERM_IO_BUFFER(void)
{
	/* Deallocate Buffer & Event queue */
    avlTreeDestroy(buffer);
    free(map_entry);
}

void ADD_REQUEST(int io_type, unsigned int size, int64_t lsn) {
    struct request *request1;
    request1 = (struct request*)malloc(sizeof(struct request));
    memset(request1,0, sizeof(struct request));

    request1->operation = io_type;
	request1->lsn = lsn;
	request1->size = size;
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;         //record the count of lsn served by buffer

	subs_r_head = NULL;
	subs_r_tail = NULL;
	subs_w_head = NULL;
	subs_w_tail = NULL;

    if(request_queue == NULL)          //The queue is empty
	{
		request_queue = request1;
		request_tail = request1;
		request_queue_length++;
	}
	else
	{			
		request_tail->next_node = request1;	
		request_tail = request1;			
		request_queue_length++;
	}
}

void BUFFER_MANAGEMENT(struct ssdstate *ssd) {
    unsigned int j,lsn,index,complete_flag=0, state,full_page;
	unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;
	int64_t lpn, last_lpn, first_lpn;
	struct request *new_request;
	struct buffer_group *buffer_node,key;
	unsigned int mask=0,offset1=0,offset2=0;

	full_page=~(0xffffffff<<ssd->ssdparams.SECTORS_PER_PAGE);
	
	new_request=request_tail;
	lsn=new_request->lsn;
	lpn=new_request->lsn/ssd->ssdparams.SECTORS_PER_PAGE;
	last_lpn=(new_request->lsn+new_request->size-1)/ssd->ssdparams.SECTORS_PER_PAGE;
	first_lpn=new_request->lsn/ssd->ssdparams.SECTORS_PER_PAGE;

	new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE/32+1));
	memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE/32+1));
	//printf("BUFFERMNG\n");
	if(new_request->operation==READ) 
	{		
		while(lpn<=last_lpn)      		
		{
			/************************************************************************************************
			 *need_distb_flag表示是否需要执行distribution函数，1表示需要执行，buffer中没有，0表示不需要执行
             *即1表示需要分发，0表示不需要分发，对应点初始全部赋为1
			*************************************************************************************************/
			need_distb_flag=full_page;   
			key.group=lpn;
			buffer_node= (struct buffer_group*)avlTreeFind(buffer, (TREE_NODE *)&key);		// buffer node 

			while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
			{             	
				lsn_flag=full_page;
				mask=1 << (lsn%ssd->ssdparams.SECTORS_PER_PAGE);
				if(mask>31)
				{
					//PRINTF("the subpage number is larger than 32!add some cases");
					//getchar(); 		   
				}
				else if((buffer_node->stored & mask)==mask)
				{
					flag=1;
					lsn_flag=lsn_flag&(~mask);
				}

				if(flag==1)				
				{	//如果该buffer节点不在buffer的队首，需要将这个节点提到队首，实现了LRU算法，这个是一个双向队列。		       		
					if(buffer->buffer_head!=buffer_node)     
					{		
						if(buffer->buffer_tail==buffer_node)								
						{			
							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							buffer->buffer_tail=buffer_node->LRU_link_pre;							
						}				
						else								
						{				
							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
						}								
						buffer_node->LRU_link_next=buffer->buffer_head;
						buffer->buffer_head->LRU_link_pre=buffer_node;
						buffer_node->LRU_link_pre=NULL;			
						buffer->buffer_head=buffer_node;													
					}						
					buffer->read_hit++;					
					new_request->complete_lsn_count++;											
				}		
				else if(flag==0)
					{
						buffer->read_miss_hit++;
					}

				need_distb_flag=need_distb_flag&lsn_flag;
				
				flag=0;		
				lsn++;						
			}	
				
			index=(lpn-first_lpn)/(32/ssd->ssdparams.SECTORS_PER_PAGE); 			
			new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->ssdparams.SECTORS_PER_PAGE))*ssd->ssdparams.SECTORS_PER_PAGE));	
			lpn++;
			
		}
	}  
	else if(new_request->operation==WRITE)
	{
		while(lpn<=last_lpn)           	
		{	
			need_distb_flag=full_page;
			mask=~(0xffffffff<<(ssd->ssdparams.SECTORS_PER_PAGE));
			state=mask;

			if(lpn==first_lpn)
			{
				offset1=ssd->ssdparams.SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE-new_request->lsn);
				state=state&(0xffffffff<<offset1);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->ssdparams.SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE-(new_request->lsn+new_request->size));
				state=state&(~(0xffffffff<<offset2));
			}
			//printf("BUFFER MNG WRITE: %d %d\n",lsn, lpn);
			INSERT_TO_BUFFER(ssd, lpn, state,new_request);
			lpn++;
		}
	}
	complete_flag = 1;
	for(j=0;j<=(last_lpn-first_lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE/32;j++)
	{
		if(new_request->need_distr_flag[j] != 0)
		{
			complete_flag = 0;
		}
	}

	/*************************************************************
	*如果请求已经被全部由buffer服务，该请求可以被直接响应，输出结果
	*这里假设dram的服务时间为1000ns
	**************************************************************/
	if((complete_flag == 1)&&(new_request->subs==NULL))               
	{
		//new_request->begin_time=ssd->current_time;
		new_request->response_time=1000;            
    }
}

void INSERT_TO_BUFFER(struct ssdstate *ssd, unsigned int lpn, int state, struct request *req) {
    	int write_back_count,flag=0;                                                             /*flag表示为写入新数据腾空间是否完成，0表示需要进一步腾，1表示已经腾空*/
	unsigned int i,lsn,hit_flag,add_flag,sector_count,active_region_flag=0,free_sector=0;
	struct buffer_group *buffer_node=NULL,*pt,*new_node=NULL,key;
	struct sub_request *sub_req=NULL,*update=NULL;
	
	
	unsigned int sub_req_state=0, sub_req_size=0,sub_req_lpn=0;

	sector_count=size(state);                                                                /*需要写到buffer的sector个数*/
	key.group=lpn;
	buffer_node= (struct buffer_group*)avlTreeFind(buffer, (TREE_NODE *)&key);    /*在平衡二叉树中寻找buffer node*/ 
    
	/************************************************************************************************
	*没有命中。
	*第一步根据这个lpn有多少子页需要写到buffer，去除已写回的lsn，为该lpn腾出位置，
	*首先即要计算出free sector（表示还有多少可以直接写的buffer节点）。
	*如果free_sector>=sector_count，即有多余的空间够lpn子请求写，不需要产生写回请求
	*否则，没有多余的空间供lpn子请求写，这时需要释放一部分空间，产生写回请求。就要creat_sub_request()
	*************************************************************************************************/
	if(buffer_node==NULL)
	{
		free_sector=buffer->max_buffer_sector-buffer->buffer_sector_count;
		//printf("INSERT NULL: %d %d %d\n", free_sector, sector_count, lpn);
		if(free_sector>=sector_count)
		{
			flag=1;    
		}
		if(flag==0)     
		{
			write_back_count=sector_count-free_sector;
			//printf("WRITE BACK COUNT: %d\n", write_back_count);
			buffer->write_miss_hit=buffer->write_miss_hit+write_back_count;
			while(write_back_count>0)
			{
				sub_req=NULL;
				sub_req_state=buffer->buffer_tail->stored; 
				sub_req_size=size(buffer->buffer_tail->stored);
				sub_req_lpn=buffer->buffer_tail->group;
				sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);
				buffer->buffer_sector_count=buffer->buffer_sector_count-sub_req->size;
				pt = buffer->buffer_tail;
				avlTreeDel(buffer, (TREE_NODE *) pt);
				if(buffer->buffer_head->LRU_link_next == NULL){
					buffer->buffer_head = NULL;
					buffer->buffer_tail = NULL;
				}else{
					buffer->buffer_tail=buffer->buffer_tail->LRU_link_pre;
					buffer->buffer_tail->LRU_link_next=NULL;
				}
				pt->LRU_link_next=NULL;
				pt->LRU_link_pre=NULL;
				AVL_TREENODE_FREE(buffer, (TREE_NODE *) pt);
				pt = NULL;
				
				write_back_count=write_back_count-sub_req->size;
			}
		}
		new_node=NULL;
		new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
		memset(new_node,0, sizeof(struct buffer_group));	   
		new_node->group=lpn;
		new_node->stored=state;
		new_node->dirty_clean=state;
		new_node->LRU_link_pre = NULL;
		new_node->LRU_link_next=buffer->buffer_head;
		if(buffer->buffer_head != NULL){
			buffer->buffer_head->LRU_link_pre=new_node;
		}else{
			buffer->buffer_tail = new_node;
		}
		buffer->buffer_head=new_node;
		new_node->LRU_link_pre=NULL;
		avlTreeAdd(buffer, (TREE_NODE *) new_node);
		buffer->buffer_sector_count += sector_count;
	}
	else
	{
		//printf("INSERT NOT NULL:, %d\n", lpn);
		for(i=0;i<ssd->ssdparams.SECTORS_PER_PAGE;i++)
		{
			if((state>>i)%2!=0)                                           
			{
				lsn=lpn*ssd->ssdparams.SECTORS_PER_PAGE+i;
				hit_flag=0;
				hit_flag=(buffer_node->stored)&(0x00000001<<i);
				
				if(hit_flag!=0)
				{	
					active_region_flag=1;                                 
					if(req!=NULL)
					{
						if(buffer->buffer_head!=buffer_node)     
						{				
							if(buffer->buffer_tail==buffer_node)
							{				
								buffer->buffer_tail=buffer_node->LRU_link_pre;
								buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							}				
							else if(buffer_node != buffer->buffer_head)
							{					
								buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
								buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;
							}				
							buffer_node->LRU_link_next=buffer->buffer_head;	
							buffer->buffer_head->LRU_link_pre=buffer_node;
							buffer_node->LRU_link_pre=NULL;				
							buffer->buffer_head=buffer_node;					
						}					
						buffer->write_hit++;
						req->complete_lsn_count++;		
					}
				}			
				else                 			
				{
					buffer->write_miss_hit++;	
					if(buffer->buffer_sector_count>=buffer->max_buffer_sector)
					{
						if (buffer_node==buffer->buffer_tail)
						{
							pt = buffer->buffer_tail->LRU_link_pre;
							buffer->buffer_tail->LRU_link_pre=pt->LRU_link_pre;
							buffer->buffer_tail->LRU_link_pre->LRU_link_next=buffer->buffer_tail;
							buffer->buffer_tail->LRU_link_next=pt;
							pt->LRU_link_next=NULL;
							pt->LRU_link_pre=buffer->buffer_tail;
							buffer->buffer_tail=pt;
							
						}
						sub_req=NULL;
						sub_req_state=buffer->buffer_tail->stored; 
						sub_req_size=size(buffer->buffer_tail->stored);
						sub_req_lpn=buffer->buffer_tail->group;
						sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);

						buffer->buffer_sector_count=buffer->buffer_sector_count-sub_req->size;
						pt = buffer->buffer_tail;	
						avlTreeDel(buffer, (TREE_NODE *) pt);
						    
						if(buffer->buffer_head->LRU_link_next == NULL)
						{
							buffer->buffer_head = NULL;
							buffer->buffer_tail = NULL;
						}else{
							buffer->buffer_tail=buffer->buffer_tail->LRU_link_pre;
							buffer->buffer_tail->LRU_link_next=NULL;
						}
						pt->LRU_link_next=NULL;
						pt->LRU_link_pre=NULL;
						AVL_TREENODE_FREE(buffer, (TREE_NODE *) pt);
						pt = NULL;	
					}

					                             
					add_flag=0x00000001<<(lsn%ssd->ssdparams.SECTORS_PER_PAGE);					
					if(buffer->buffer_head!=buffer_node)
					{				
						if(buffer->buffer_tail==buffer_node)
						{					
							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							buffer->buffer_tail=buffer_node->LRU_link_pre;
						}			
						else						
						{			
							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;						
							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
						}								
						buffer_node->LRU_link_next=buffer->buffer_head;			
						buffer->buffer_head->LRU_link_pre=buffer_node;
						buffer_node->LRU_link_pre=NULL;	
						buffer->buffer_head=buffer_node;							
					}					
					buffer_node->stored=buffer_node->stored|add_flag;		
					buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;	
					buffer->buffer_sector_count++;
				}			

			}
		}
	}
}

struct sub_request * creat_sub_request(struct ssdstate * ssd,unsigned int lpn,int size,unsigned int state,struct request * req, int operation)
{
	//printf("CREATE\n");
	static int seq_number = 0; 
    int num_flash = 0, num_channel = 0;
	struct sub_request* sub=NULL,* sub_r=NULL;
	struct channel_info * p_ch=NULL;
    int64_t ppn;
	//struct local * loc=NULL;
	unsigned int flag=0;

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));        
	memset(sub,0, sizeof(struct sub_request));

	if(sub==NULL)
	{
		return NULL;
	}
	sub->next_node=NULL;
	sub->next_subs=NULL;
	sub->update=NULL;
	
	
	if (operation == READ)
	{
		sub->lpn = lpn;
		sub->size=size;            
		sub->operation = READ;
		sub->state=(map_entry[lpn].state&0x7fffffff);
	
		if (subs_r_tail!=NULL) {
			subs_r_tail->next_node=sub;
			subs_r_tail=sub;
		} 
		else {		
			subs_r_head=sub;
			subs_r_tail=sub;
		}		
	}
	
	else if(operation == WRITE)
	{
		//printf("CREATE WRITE\n");
		sub->operation = WRITE;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		allocate_location(ssd ,sub);  			
	}	
	return sub;
}

void allocate_location(struct ssdstate * ssd ,struct sub_request *sub_req)
{

	struct sub_request * update=NULL;
    int64_t ppn;
    unsigned int lpn = sub_req->lpn;
    int num_flash = 0, num_channel = 0;
    int map_entry_new,map_entry_old,modify;

	//printf("ALLOCATE: \n", sub_req->lpn);
    if (map_entry[sub_req->lpn].state!=0)
    {            

        if ((sub_req->state&map_entry[sub_req->lpn].state)!=map_entry[sub_req->lpn].state)  
        {
            update=(struct sub_request *)malloc(sizeof(struct sub_request));
            update->next_node=NULL;
            update->next_subs=NULL;
            update->update=NULL;						
            update->lpn = sub_req->lpn;
            update->state=((map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
            update->size=size(update->state);
            update->operation = READ;
			
				
            if (subs_r_tail != NULL)
            {
                subs_r_tail->next_node=update;
				subs_r_tail=update;
            } 
            else
            {
                subs_r_tail=update;
                subs_r_head=update;
            }
        }

        if (update!=NULL)
        {
			
            sub_req->update=update;

            sub_req->state=(sub_req->state|update->state);
            sub_req->size=size(sub_req->state);
        }

    }

    if (subs_w_tail != NULL)
    {
        subs_w_tail->next_node=sub_req;
        subs_w_tail=sub_req;
    } 
    else
    {
        subs_w_tail=sub_req;
        subs_w_head=sub_req;
    }

    if(map_entry[lpn].state==0)               
    {   map_entry[lpn].pn=ppn;	
        map_entry[lpn].state=sub_req->state;   //0001
    }//if(map_entry[lpn].state==0)
    else if(map_entry[lpn].state>0)          
    {
        map_entry_new=sub_req->state;      
        map_entry_old=map_entry[lpn].state;
        modify=map_entry_new|map_entry_old;
        map_entry[lpn].state=modify; 
    }//else if(map_entry[lpn].state>0)
	//printf("ALLOCATE2\n");
}

void DISTRIBUTE(struct ssdstate *ssd) 
{
	unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0,full_page;
	unsigned int j, k, sub_size;
	int i=0;
	struct request *req;
	struct sub_request *sub;
	int* complt;

	full_page=~(0xffffffff<<ssd->ssdparams.SECTORS_PER_PAGE);

	req = request_tail;
	if(req->response_time != 0){
		return;
	}
	if (req->operation==WRITE)
	{
		return;
	}

	
	if(req != NULL)
	{
		if(req->distri_flag == 0)
		{
	
			if(req->complete_lsn_count != request_tail->size)
			{		
				first_lsn = req->lsn;				
				last_lsn = first_lsn + req->size;
				complt = req->need_distr_flag; // which subpages need to be transfered 
				start = first_lsn - first_lsn % ssd->ssdparams.SECTORS_PER_PAGE;
				end = (last_lsn/ssd->ssdparams.SECTORS_PER_PAGE + 1) * ssd->ssdparams.SECTORS_PER_PAGE;
				i = (end - start)/32;	
	
				while(i >= 0)
				{	
					for(j=0; j<32/ssd->ssdparams.SECTORS_PER_PAGE; j++)
					{	
					
						
						k = (complt[((end-start)/32-i)] >>(ssd->ssdparams.SECTORS_PER_PAGE*j)) & full_page;	  // k: which subpages need to be transfered 
						
						if (k !=0) 
						{
							lpn = start/ssd->ssdparams.SECTORS_PER_PAGE+ ((end-start)/32-i)*32/ssd->ssdparams.SECTORS_PER_PAGE + j;
							sub_size=transfer_size(ssd,k,lpn,req);    
							if (sub_size==0) 
							{
								continue;
							}
							else
							{
								sub=creat_sub_request(ssd,lpn,sub_size,0,req,req->operation);
							}	
						}
					}
					i = i-1;
				}

			}
		}
	}
	return;
}

void PROCESS(struct ssdstate *ssd) {
	unsigned int i,chan,random_num;          
    struct request *req=NULL, *p=NULL;
    //printf("PROCESS\n");

	PROCESS_READS(ssd);	
	PROCESS_WRITES(ssd);				

    req = request_queue;
    while (req != NULL) {
        p = req;
        req = req->next_node;
        free(p->need_distr_flag);
        free(p);
    }
	request_queue = NULL;
    request_tail = NULL;
	return;
}

void PROCESS_READS(struct ssdstate *ssd) {
    struct sub_request * sub=NULL, * p=NULL;
    sub=subs_r_head;

    while(sub!=NULL) {
        FTL_READ(ssd, sub->lpn * ssd->ssdparams.SECTORS_PER_PAGE, ssd->ssdparams.SECTORS_PER_PAGE);
		//printf("READ");
        p = sub;
        sub = sub->next_node;
        free(p);
    }
	subs_r_head = NULL;
	subs_r_tail = NULL;
}

void PROCESS_WRITES(struct ssdstate *ssd) {
    struct sub_request * sub=NULL, * p=NULL;
    sub=subs_w_head;

    while(sub!=NULL) {
        FTL_WRITE(ssd, sub->lpn * ssd->ssdparams.SECTORS_PER_PAGE, ssd->ssdparams.SECTORS_PER_PAGE);
		//printf("WRITE");
        p = sub;
        sub = sub->next_node;
        free(p);
    }
	subs_w_head = NULL;
	subs_w_tail = NULL;
}

unsigned int transfer_size(struct ssdstate *ssd,int need_distribute,unsigned int lpn,struct request *req)
{
	unsigned int first_lpn,last_lpn,state,trans_size;
	unsigned int mask=0,offset1=0,offset2=0;

	first_lpn=req->lsn/ssd->ssdparams.SECTORS_PER_PAGE;
	last_lpn=(req->lsn+req->size-1)/ssd->ssdparams.SECTORS_PER_PAGE;

	mask=~(0xffffffff<<(ssd->ssdparams.SECTORS_PER_PAGE));
	state=mask;
	if(lpn==first_lpn)
	{
		offset1=ssd->ssdparams.SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE-req->lsn);
		state=state&(0xffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2=ssd->ssdparams.SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams.SECTORS_PER_PAGE-(req->lsn+req->size));
		state=state&(~(0xffffffff<<offset2));
	}

	trans_size=size(state&need_distribute);

	return trans_size;
}

unsigned int size(unsigned int stored)
{
	unsigned int i,total=0,mask=0x80000000;

	for(i=1;i<=32;i++)
	{
		if(stored & mask) total++;
		stored<<=1;
	}
    return total;
}

int keyCompareFunc(TREE_NODE *p , TREE_NODE *p1)
{
	struct buffer_group *T1=NULL,*T2=NULL;

	T1=(struct buffer_group*)p;
	T2=(struct buffer_group*)p1;


	if(T1->group< T2->group) return 1;
	if(T1->group> T2->group) return -1;

	return 0;
}

int freeFunc(TREE_NODE *pNode)
{
	
	if(pNode!=NULL)
	{
		free((void *)pNode);
	}
	
	
	pNode=NULL;
	return 1;
}
