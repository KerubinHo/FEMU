
// File: firm_buffer_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_BUFFER_MANAGER_H_
#define _SSD_BUFFER_MANAGER_H_

#ifdef SSD_THREAD
extern int r_queue_full;
extern int w_queue_full;
extern pthread_cond_t eq_ready;
extern pthread_mutex_t eq_lock;
extern pthread_mutex_t cq_lock;
#endif

typedef struct event_queue_entry
{
	int io_type;
	int valid;
	int64_t sector_nb;
	unsigned int length;
	void* buf;
	struct event_queue_entry* next;
}event_queue_entry;

typedef struct event_queue
{
	int entry_nb;
	event_queue_entry* head;
	event_queue_entry* tail;
}event_queue;

void INIT_IO_BUFFER(struct ssdstate *ssd);
void TERM_IO_BUFFER(struct ssdstate *ssd);
void INIT_WB_VALID_ARRAY(struct ssdstate *ssd);

void *SSD_THREAD_MAIN_LOOP(struct ssdstate *ssd, void *arg);
int64_t ENQUEUE_IO(struct ssdstate *ssd, int io_type, int64_t sector_nb, unsigned int length);
int64_t ENQUEUE_READ(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);
void ENQUEUE_WRITE(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);

int64_t DEQUEUE_IO(struct ssdstate *ssd);
void DEQUEUE_COMPLETED_READ(struct ssdstate *ssd);

event_queue_entry* ALLOC_NEW_EVENT(int io_type, int64_t sector_nb, unsigned int length, void* buf);

void WRITE_DATA_TO_BUFFER(struct ssdstate *ssd, unsigned int length);
void READ_DATA_FROM_BUFFER_TO_HOST(struct ssdstate *ssd, event_queue_entry* c_e_q_entry);
void COPY_DATA_TO_READ_BUFFER(struct ssdstate *ssd, event_queue_entry* dst_entry, event_queue_entry* src_entry);
int64_t FLUSH_EVENT_QUEUE_UNTIL(struct ssdstate *ssd, event_queue_entry* e_q_entry);

int EVENT_QUEUE_IS_FULL(int io_type, unsigned int length);
int64_t SECURE_WRITE_BUFFER(struct ssdstate *ssd);
int64_t SECURE_READ_BUFFER(struct ssdstate *ssd);

/* Check Event */
int CHECK_OVERWRITE(event_queue_entry* e_q_entry, int64_t sector_nb, unsigned int length);
int CHECK_SEQUENTIALITY(event_queue_entry* e_q_entry, int64_t sector_nb);
event_queue_entry* CHECK_IO_DEPENDENCY_FOR_READ(int64_t sector_nb, unsigned int length);
int CHECK_IO_DEPENDENCY_FOR_WRITE(event_queue_entry* e_q_entry, int64_t sector_nb, unsigned int length);

/* Manipulate Write Buffer Valid Array */
char GET_WB_VALID_ARRAY_ENTRY(struct ssdstate *ssd, void* buffer_pointer);
void UPDATE_WB_VALID_ARRAY(struct ssdstate *ssd, event_queue_entry* e_q_entry, char new_value);
void UPDATE_WB_VALID_ARRAY_ENTRY(struct ssdstate *ssd, void* buffer_pointer, char new_value);
void UPDATE_WB_VALID_ARRAY_PARTIAL(struct ssdstate *ssd, event_queue_entry* e_q_entry, char new_value, int length, int mode);

/* Move Buffer Frame Pointer */
void INCREASE_WB_SATA_POINTER(struct ssdstate *ssd, int entry_nb);
void INCREASE_RB_SATA_POINTER(struct ssdstate *ssd, int entry_nb);
void INCREASE_WB_FTL_POINTER(struct ssdstate *ssd, int entry_nb);
void INCREASE_RB_FTL_POINTER(struct ssdstate *ssd, int entry_nb);
void INCREASE_WB_LIMIT_POINTER(struct ssdstate *ssd);
void INCREASE_RB_LIMIT_POINTER(struct ssdstate *ssd);

/* Test IO BUFFER */
int COUNT_READ_EVENT(void);

#endif
