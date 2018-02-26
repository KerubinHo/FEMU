// File: firm_buffer_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "common.h"
#include <pthread.h>

#if 0

struct request *request_queue;
struct request *request_tail;
unsigned int request_queue_length;
struct buffer_info *buffer;
struct entry *map_entry;
struct channel_info *channel_head.

event_queue* e_queue;
event_queue* c_e_queue;

void INIT_IO_BUFFER(void)
{
    unsigned int page_num;
    request_queue = NULL;
    request_tail = NULL;
    request_queue_length = 0;;

    page_num = ssd->ssdparams->PAGES_IN_SSD;

    channel_head=(struct channel_info*)malloc(ssd->ssdparams->CHANNEL_NB * sizeof(struct channel_info));
    memset(channel_head,0,ssd->ssdparams->CHANNEL_NB * sizeof(struct channel_info));
    buffer = (tAVLTree *)avlTreeCreate((void*)keyCompareFunc , (void *)freeFunc);
    buffer->max_buffer_sector=WRITE_BUFFER_FRAME_NB;
    map_entry = (struct entry *)malloc(sizeof(struct entry) * page_num);
    memset(map_entry,0,sizeof(struct entry) * page_num);
}

void TERM_IO_BUFFER(void)
{
	/* Deallocate Buffer & Event queue */
    int avlTreeDestroy(buffer);
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
    unsigned int j,lsn,lpn,last_lpn,first_lpn,index,complete_flag=0, state,full_page;
	unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;           
	struct request *new_request;
	struct buffer_group *buffer_node,key;
	unsigned int mask=0,offset1=0,offset2=0;

	full_page=~(0xffffffff<<ssd->ssdparams->SECTORS_PER_PAGE);
	
	new_request=request_tail;
	lsn=new_request->lsn;
	lpn=new_request->lsn/ssd->ssdparams->SECTORS_PER_PAGE;
	last_lpn=(new_request->lsn+new_request->size-1)/ssd->ssdparams->SECTORS_PER_PAGE;
	first_lpn=new_request->lsn/ssd->ssdparams->SECTORS_PER_PAGE;

	new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE/32+1));
	memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE/32+1));
	
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

			while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
			{             	
				lsn_flag=full_page;
				mask=1 << (lsn%ssd->ssdparams->SECTORS_PER_PAGE);
				if(mask>31)
				{
					printf("the subpage number is larger than 32!add some cases");
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
				
			index=(lpn-first_lpn)/(32/ssd->ssdparams->SECTORS_PER_PAGE); 			
			new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->ssdparams->SECTORS_PER_PAGE))*ssd->ssdparams->SECTORS_PER_PAGE));	
			lpn++;
			
		}
	}  
	else if(new_request->operation==WRITE)
	{
		while(lpn<=last_lpn)           	
		{	
			need_distb_flag=full_page;
			mask=~(0xffffffff<<(ssd->ssdparams->SECTORS_PER_PAGE));
			state=mask;

			if(lpn==first_lpn)
			{
				offset1=ssd->ssdparams->SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE-new_request->lsn);
				state=state&(0xffffffff<<offset1);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->ssdparams->SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE-(new_request->lsn+new_request->size));
				state=state&(~(0xffffffff<<offset2));
			}
			
			ssd=INSERT_TO_BUFFER(ssd, lpn, state,NULL,new_request);
			lpn++;
		}
	}
	complete_flag = 1;
	for(j=0;j<=(last_lpn-first_lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE/32;j++)
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

void INSERT_TO_BUFFER(struct ssdstate *ssd, unsigned int lpn, int state, struct sub_request *sub, struct request *req) {
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
		if(free_sector>=sector_count)
		{
			flag=1;    
		}
		if(flag==0)     
		{
			write_back_count=sector_count-free_sector;
			buffer->write_miss_hit=buffer->write_miss_hit+write_back_count;
			while(write_back_count>0)
			{
				sub_req=NULL;
				sub_req_state=buffer->buffer_tail->stored; 
				sub_req_size=size(buffer->buffer_tail->stored);
				sub_req_lpn=buffer->buffer_tail->group;
				sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);
				
				
				/**********************************************************************************
				*req不为空，表示这个insert2buffer函数是在buffer_management中调用，传递了request进来
				*req为空，表示这个函数是在process函数中处理一对多映射关系的读的时候，需要将这个读出
				*的数据加到buffer中，这可能产生实时的写回操作，需要将这个实时的写回操作的子请求挂在
				*这个读请求的总请求上
				***********************************************************************************/
				if(req!=NULL)                                             
				{
				}
				else    
				{
					sub_req->next_subs=sub->next_subs;
					sub->next_subs=sub_req;
				}
                
				/*********************************************************************
				*写请求插入到了平衡二叉树，这时就要修改dram的buffer_sector_count；
				*维持平衡二叉树调用avlTreeDel()和AVL_TREENODE_FREE()函数；维持LRU算法；
				**********************************************************************/
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
				
				write_back_count=write_back_count-sub_req->size;                            /*因为产生了实时写回操作，需要将主动写回操作区域增加*/
			}
		}
		
		/******************************************************************************
		*生成一个buffer node，根据这个页的情况分别赋值个各个成员，添加到队首和二叉树中
		*******************************************************************************/
		new_node=NULL;
		new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
		//alloc_assert(new_node,"buffer_group_node");
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
	/****************************************************************************************
	*在buffer中命中的情况
	*算然命中了，但是命中的只是lpn，有可能新来的写请求，只是需要写lpn这一page的某几个sub_page
	*这时有需要进一步的判断
	*****************************************************************************************/
	else
	{
		for(i=0;i<ssd->ssdparams->SECTORS_PER_PAGE;i++)
		{
			/*************************************************************
			*判断state第i位是不是1
			*并且判断第i个sector是否存在buffer中，1表示存在，0表示不存在。
			**************************************************************/
			if((state>>i)%2!=0)                                                         
			{
				lsn=lpn*ssd->ssdparams->SECTORS_PER_PAGE+i;
				hit_flag=0;
				hit_flag=(buffer_node->stored)&(0x00000001<<i);
				
				if(hit_flag!=0)				                                          /*命中了，需要将该节点移到buffer的队首，并且将命中的lsn进行标记*/
				{	
					active_region_flag=1;                                             /*用来记录在这个buffer node中的lsn是否被命中，用于后面对阈值的判定*/

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
						req->complete_lsn_count++;                                        /*关键 当在buffer中命中时 就用req->complete_lsn_count++表示往buffer中写了数据。*/					
					}
					else
					{
					}				
				}			
				else                 			
				{
					/************************************************************************************************************
					*该lsn没有命中，但是节点在buffer中，需要将这个lsn加到buffer的对应节点中
					*从buffer的末端找一个节点，将一个已经写回的lsn从节点中删除(如果找到的话)，更改这个节点的状态，同时将这个新的
					*lsn加到相应的buffer节点中，该节点可能在buffer头，不在的话，将其移到头部。如果没有找到已经写回的lsn，在buffer
					*节点找一个group整体写回，将这个子请求挂在这个请求上。可以提前挂在一个channel上。
					*第一步:将buffer队尾的已经写回的节点删除一个，为新的lsn腾出空间，这里需要修改队尾某节点的stored状态这里还需要
					*       增加，当没有可以之间删除的lsn时，需要产生新的写子请求，写回LRU最后的节点。
					*第二步:将新的lsn加到所述的buffer节点中。
					*************************************************************************************************************/	
					buffer->write_miss_hit++;
					
					if(buffer->buffer_sector_count>=buffer->max_buffer_sector)
					{
						if (buffer_node==buffer->buffer_tail)                  /*如果命中的节点是buffer中最后一个节点，交换最后两个节点*/
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

						if(req!=NULL)           
						{
							
						}
						else if(req==NULL)   
						{
							sub_req->next_subs=sub->next_subs;
							sub->next_subs=sub_req;
						}

						buffer->buffer_sector_count=buffer->buffer_sector_count-sub_req->size;
						pt = buffer->buffer_tail;	
						avlTreeDel(buffer, (TREE_NODE *) pt);
							
						/************************************************************************/
						/* 改:  挂在了子请求，buffer的节点不应立即删除，						*/
						/*			需等到写回了之后才能删除									*/
						/************************************************************************/
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

					                                                                     /*第二步:将新的lsn加到所述的buffer节点中*/	
					add_flag=0x00000001<<(lsn%ssd->ssdparams->SECTORS_PER_PAGE);
					
					if(buffer->buffer_head!=buffer_node)                      /*如果该buffer节点不在buffer的队首，需要将这个节点提到队首*/
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

struct sub_request * creat_sub_request(struct ssdstate * ssd,unsigned int lpn,int size,unsigned int state,struct request * req, int io_type)
{

	static int seq_number = 0; 
    int num_flash = 0, num_channel = 0;
	struct sub_request* sub=NULL,* sub_r=NULL;
	struct channel_info * p_ch=NULL;
    int64_t ppn;
	//struct local * loc=NULL;
	unsigned int flag=0;

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));                       
	//alloc_assert(sub,"sub_request");
	memset(sub,0, sizeof(struct sub_request));

	if(sub==NULL)
	{
		return NULL;
	}
	//sub->location=NULL;
	sub->next_node=NULL;
	sub->next_subs=NULL;
	sub->update=NULL;
	
	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	
	
	if (operation == READ)
	{
#ifdef FTL_MAP_CACHE
		ppn = CACHE_GET_PPN(lpn);
#else
		ppn = GET_MAPPING_INFO(ssd, lpn);
#endif
        num_flash = CALC_FLASH(ssd, ppn);
        num_channel = num_flash %  CHANNEL_NB;
        
        sub->num_channel = num_channel;
		//loc = find_location(ssd,ssd->dram->map->map_entry[lpn].pn);
		//sub->location=loc;
		//sub->begin_time = ssd->current_time;
		//sub->current_state = SR_WAIT;
		//sub->current_time=MAX_INT64;
		//sub->next_state = SR_R_C_A_TRANSFER;
		//sub->next_state_predict_time=MAX_INT64;
		sub->lpn = lpn;
		sub->size=size;            
		
		
		p_ch = &channel_head[num_channel];	
		sub->ppn = map_entry[lpn].pn;
		sub->operation = READ;
		sub->state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
		sub_r=p_ch->subs_r_head;                                                      
		flag=0;
		while (sub_r!=NULL)
		{
			if (sub_r->ppn==sub->ppn)
			{
				flag=1;
				break;
			}
			sub_r=sub_r->next_node;
		}
		
			
			
		if (flag==0)
		{
			if (p_ch->subs_r_tail!=NULL)
			{
					
				p_ch->subs_r_tail->next_node=sub;
				p_ch->subs_r_tail=sub;
				
			} 
			else
			{
				
				p_ch->subs_r_head=sub;
				p_ch->subs_r_tail=sub;
				
			}
		}
		else
		{
		
			/*sub->current_state = SR_R_DATA_TRANSFER;
			sub->current_time=ssd->current_time;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time=ssd->current_time+1000;
			sub->complete_time=ssd->current_time+1000;*/
			// Narges 
			/*ssd->channel_head[loc->channel].read_count++; 
              ssd->channel_head[loc->channel].chip_head[loc->chip].read_count++;*/
		}
		
	}
	
	else if(operation == WRITE)
	{                                
		sub->ppn=0;
		sub->operation = WRITE;
		//sub->location=(struct local *)malloc(sizeof(struct local));
		//alloc_assert(sub->location,"sub->location");
		//memset(sub->location,0, sizeof(struct local));

		//sub->current_state=SR_WAIT;
		//sub->current_time=ssd->current_time;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		//sub->begin_time=ssd->current_time;
		
      
		if (allocate_location(ssd ,sub)==ERROR)
		{
			free(sub->location);
			sub->location=NULL;
			free(sub);
			sub=NULL;
			return NULL;
        }        			
	}
	else
	{
		/*free(sub->location);
		sub->location=NULL;
		free(sub);
		sub=NULL;
		printf("\nERROR ! Unexpected command.\n");
		return NULL;*/
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
	//unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;
	//struct local *location=NULL;

#ifdef FTL_MAP_CACHE
    ppn = CACHE_GET_PPN(sub_req->lpn);
#else
    ppn = GET_MAPPING_INFO(ssd, sub_req->lpn);
#endif
    num_flash = CALC_FLASH(ssd, ppn);
    num_channel = num_flash %  CHANNEL_NB;

    sub_req->num_channel = num_channel;        

    if (map_entry[sub_req->lpn].state!=0)
    {            

        if ((sub_req->state&map_entry[sub_req->lpn].state)!=map_entry[sub_req->lpn].state)  
        {
			
            //ssd->read_count++;
            //ssd->update_read_count++;
            update=(struct sub_request *)malloc(sizeof(struct sub_request));
            //alloc_assert(update,"update");
            //memset(update,0, sizeof(struct sub_request));

            //update->location=NULL;
            update->num_channel = num_channel;
            update->next_node=NULL;
            update->next_subs=NULL;
            update->update=NULL;						
            //location = find_location(ssd,map_entry[sub_req->lpn].pn);
            //update->location=location;
            //update->begin_time = ssd->current_time;
            //update->current_state = SR_WAIT;
            //update->current_time=MAX_INT64;
            //update->next_state = SR_R_C_A_TRANSFER;
            //update->next_state_predict_time=MAX_INT64;
            update->lpn = sub_req->lpn;
            update->state=((map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
            update->size=size(update->state);
            update->ppn = map_entry[sub_req->lpn].pn;
            update->operation = READ;
				
				
            if (channel_head[num_channel].subs_r_tail!=NULL)
            {
                channel_head[num_channel].subs_r_tail->next_node=update;
                channel_head[num_channel].subs_r_tail=update;
            } 
            else
            {
                channel_head[num_channel].subs_r_tail=update;
                channel_head[num_channel].subs_r_head=update;
            }
        }

        if (update!=NULL)
        {
			
            sub_req->update=update;

            sub_req->state=(sub_req->state|update->state);
            sub_req->size=size(sub_req->state);
        }

    }

    if (channel_head[sub_req->num_channel].subs_w_tail!=NULL)
    {
        channel_head[sub_req->num_channel].subs_w_tail->next_node=sub_req;
        channel_head[sub_req->num_channel].subs_w_tail=sub_req;
    } 
    else
    {
        channel_head[sub_req->num_channel].subs_w_tail=sub_req;
        channel_head[sub_req->num_channel].subs_w_head=sub_req;
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
}

void DISTRIBUTE(struct ssdstate *ssd) 
{
	unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0,full_page;
	unsigned int j, k, sub_size;
	int i=0;
	struct request *req;
	struct sub_request *sub;
	int* complt;

	full_page=~(0xffffffff<<ssd->ssdparams->SECTORS_PER_PAGE);

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
				start = first_lsn - first_lsn % ssd->ssdparams->SECTORS_PER_PAGE;
				end = (last_lsn/ssd->ssdparams->SECTORS_PER_PAGE + 1) * ssd->ssdparams->SECTORS_PER_PAGE;
				i = (end - start)/32;	
	
				while(i >= 0)
				{	
					for(j=0; j<32/ssd->ssdparams->SECTORS_PER_PAGE; j++)
					{	
					
						
						k = (complt[((end-start)/32-i)] >>(ssd->ssdparams->SECTORS_PER_PAGE*j)) & full_page;	  // k: which subpages need to be transfered 
						
						if (k !=0) 
						{
							lpn = start/ssd->ssdparams->SECTORS_PER_PAGE+ ((end-start)/32-i)*32/ssd->ssdparams->SECTORS_PER_PAGE + j;
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
			else
			{
				//req->begin_time=ssd->current_time;
				req->response_time=1000;   
			}

		}
	}
	return;
}

void PROCESS(struct ssdstate *ssd) {
	unsigned int i,chan,random_num;          
    struct request *req=NULL, *p=NULL;
    
    random_num=ssdstate->last_time%ssd->ssdparams->CHANNEL_NB;

    for(chan=0;chan<ssd->ssdparams->CHANNEL_NB;chan++)	     
	{
		i=(random_num+chan)%ssd->ssdparams->CHANNEL_NB;
  	
        if(channel_head[i].subs_r_head!=NULL)	
        {		     
            PROCESS_READS(ssd, i);                   
						
        }
        if(channel_head[i].subs_w_head!=NULL)
        {	
            PROCESS_WRITES(ssd, i);				
        }
    }

    req = request_queue;
    while (req != NULL) {
        p = req;
        req = req->next_node;
        free(need_distr_flag);
        free(p);
    }    
	return;
}

void PROCESS_READS(struct ssdstate *ssd, unsigned int channel) {
    struct sub_request * sub=NULL, * p=NULL;
    sub=ssd->channel_head[i].subs_r_head;

    while(sub!=NULL) {
        FTL_READ(ssd, sub->lpn * ssd->ssdparams->SECTORS_PER_PAGE, ssd->ssdparams->SECTORS_PER_PAGE);
        p = sub;
        sub = sub->next_node;
        free(p);
    }
}

void PROCESS_WRITES(struct ssdstate *ssd, unsigned int channel) {
    struct sub_request * sub=NULL, * p=NULL;
    sub=ssd->channel_head[i].subs_w_head;

    while(sub!=NULL) {
        FTL_WRITE(ssd, sub->lpn * ssd->ssdparams->SECTORS_PER_PAGE, ssd->ssdparams->SECTORS_PER_PAGE);
        p = sub;
        sub = sub->next_node;
        free(p);
    }
}

unsigned int transfer_size(struct ssdstate *ssd,int need_distribute,unsigned int lpn,struct request *req)
{
	unsigned int first_lpn,last_lpn,state,trans_size;
	unsigned int mask=0,offset1=0,offset2=0;

	first_lpn=req->lsn/ssd->ssdparams->SECTORS_PER_PAGE;
	last_lpn=(req->lsn+req->size-1)/ssd->ssdparams->SECTORS_PER_PAGE;

	mask=~(0xffffffff<<(ssd->ssdparams->SECTORS_PER_PAGE));
	state=mask;
	if(lpn==first_lpn)
	{
		offset1=ssd->ssdparams->SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE-req->lsn);
		state=state&(0xffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2=ssd->ssdparams->SECTORS_PER_PAGE-((lpn+1)*ssd->ssdparams->SECTORS_PER_PAGE-(req->lsn+req->size));
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

#endif
