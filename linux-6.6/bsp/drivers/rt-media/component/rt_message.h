/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/
#ifndef __MESSAGE_H_8X3E__
#define __MESSAGE_H_8X3E__

#ifdef __cplusplus
extern "C" {
#endif

//ref platform headers
//#include "plat_type.h"
//#include <pthread.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#include "rt_common.h"
#include "rt_semaphore.h"

//cts test may send 1000 messages.
//we must make sure message queue longer than 10000.

#define MAX_MESSAGE_ELEMENTS (8)    //20000

typedef struct MessageReply {
	rt_sem_t ReplySem;
	int ReplyResult;
	void *pReplyExtra; //message processor kmalloc memory, message sender kfree memory.
	int nReplyExtraSize;
} MessageReply;
MessageReply *ConstructMessageReply(void);
void DestructMessageReply(MessageReply *pReply);

//typedef struct message_t message_t;
typedef struct message_t {
	int 				id;
	int 				command;
	//	int 				priority;
	int 				para0;
	int 				para1;
	void				*mpData; //kmalloc(), kfree()
	int 				mDataSize;
	MessageReply		*pReply;
	struct list_head	mList;
} message_t;
int InitMessage(message_t *pMsg);

typedef struct message_queue_t {
	struct list_head    mIdleMessageList;   //message_t
	struct list_head    mReadyMessageList;   //message_t
	//struct list_head    mMessageBufList;   //DynamicBuffer, sizeof(message_t)*MAX_MESSAGE_ELEMENTS
	int                 message_count;
	struct mutex        mutex;
	wait_queue_head_t   wait_msg;
	int                 mWaitMessageFlag;
} message_queue_t;

int  message_create(message_queue_t *message);
void message_destroy(message_queue_t *msg_queue);
void flush_message(message_queue_t *msg_queue);
int  put_message(message_queue_t *msg_queue, message_t *msg_in);
int  get_message(message_queue_t *msg_queue, message_t *msg_out);
int putMessageWithData(message_queue_t *msg_queue, message_t *msg_in);
int  get_message_count(message_queue_t *message);
int TMessage_WaitQueueNotEmpty(message_queue_t *msg_queue, unsigned int timeout);   //unit:ms

#ifdef __cplusplus
}
#endif

#endif

