// File: firm_buffer_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_BUFFER_MANAGER_H_
#define _SSD_BUFFER_MANAGER_H_

#include "common.h"

struct request{
	int64_t lsn;                  //请求的起始地址，逻辑地址
	unsigned int size;                 //请求的大小，既多少个扇区
	int operation;            //请求的种类，1为读，0为写
	unsigned int* need_distr_flag;
	unsigned int complete_lsn_count;   //record the count of lsn served by buffer
	int distri_flag;		           // indicate whether this request has been distributed already
	struct sub_request *subs;          //链接到属于该请求的所有子请求
	struct request *next_node;         //指向下一个请求结构体
    int response_time;
};

struct sub_request{
	int size;
    unsigned int lpn;
    int operation;
	unsigned int state;
    unsigned int ppn;                  //分配那个物理子页给这个子请求。在multi_chip_page_mapping中，产生子页请求时可能就知道psn的值，其他时候psn的值由page_map_read,page_map_write等FTL最底层函数产生。
	struct sub_request *next_subs;    //指向属于同一个request的子请求
	struct sub_request *update;
	struct sub_request *next_node;
    int num_channel
};

struct channel_info{
	int chip;                            //表示在该总线上有多少颗粒
	unsigned int token;                  //在动态分配中，为防止每次分配在第一个chip需要维持一个令牌，每次从令牌所指的位置开始分配

	struct sub_request *subs_r_head;     //channel上的读请求队列头，先服务处于队列头的子请求
	struct sub_request *subs_r_tail;     //channel上的读请求队列尾，新加进来的子请求加到队尾
	struct sub_request *subs_w_head;     //channel上的写请求队列头，先服务处于队列头的子请求
	struct sub_request *subs_w_tail;     //channel上的写请求队列，新加进来的子请求加到队尾   
};

struct entry{                       
	unsigned int pn;                //物理号，既可以表示物理页号，也可以表示物理子页号，也可以表示物理块号
	int state;                      //十六进制表示的话是0000-FFFF，每位表示相应的子页是否有效（页映射）。比如在这个页中，0，1号子页有效，2，3无效，这个应该是0x0003.
};

typedef struct buffer_group{
	TREE_NODE node;                     //树节点的结构一定要放在用户自定义结构的最前面，注意!
	struct buffer_group *LRU_link_next;	// next node in LRU list
	struct buffer_group *LRU_link_pre;	// previous node in LRU list

	unsigned int group;                 //the first data logic sector number of a group stored in buffer 
	unsigned int stored;                //indicate the sector is stored in buffer or not. 1 indicates the sector is stored and 0 indicate the sector isn't stored.EX.  00110011 indicates the first, second, fifth, sixth sector is stored in buffer.
	unsigned int dirty_clean;           //it is flag of the data has been modified, one bit indicates one subpage. EX. 0001 indicates the first subpage is dirty
	int flag;			                //indicates if this node is the last 20% of the LRU list	
}buf_node;

void INIT_IO_BUFFER(struct ssdstate *ssd);
void TERM_IO_BUFFER(void);

void ADD_REQUEST(int io_type, unsigned int size, int64_t lsn);

void BUFFER_MANAGEMENT(struct ssdstate *ssd);
void INSERT_TO_BUFFER(struct ssdstate *ssd, unsigned int lpn, int state, struct sub_request *sub, struct request *req);

unsigned int size(unsigned int stored);
int keyCompareFunc(TREE_NODE *p , TREE_NODE *p1);
struct sub_request * creat_sub_request(struct ssdstate * ssd,unsigned int lpn,int size,unsigned int state,struct request * req, int io_type);
void allocate_location(struct ssdstate * ssd ,struct sub_request *sub_req);
void DISTRIBUTE(struct ssdstate *ssd);
unsigned int transfer_size(struct ssdstate *ssd,int need_distribute,unsigned int lpn,struct request *req);
void PROCESS(struct ssdstate *ssd);
void PROCESS_READS(struct ssdstate *ssd, unsigned int channel);
void PROCESS_WRITES(struct ssdstate *ssd, unsigned int channel);
int freeFunc(TREE_NODE *pNode);
#endif
