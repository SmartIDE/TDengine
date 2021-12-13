/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TD_TQ_H_
#define _TD_TQ_H_

#include "os.h"
#include "tutil.h"
#include "mallocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TmqMsgHead {
  int32_t protoVer;
  int32_t msgType;
  int64_t cgId;
  int64_t clientId;
} TmqMsgHead;

typedef struct TmqOneAck {
  int64_t topicId;
  int64_t consumeOffset;
} TmqOneAck;

typedef struct TmqAcks {
  int32_t ackNum;
  // should be sorted
  TmqOneAck acks[];
} TmqAcks;

// TODO: put msgs into common
typedef struct TmqConnectReq {
  TmqMsgHead head;
  TmqAcks    acks;
} TmqConnectReq;

typedef struct TmqConnectRsp {
  TmqMsgHead head;
  int8_t     status;
} TmqConnectRsp;

typedef struct TmqDisconnectReq {
  TmqMsgHead head;
} TmqDiscconectReq;

typedef struct TmqDisconnectRsp {
  TmqMsgHead head;
  int8_t     status;
} TmqDisconnectRsp;

typedef struct TmqConsumeReq {
  TmqMsgHead head;
  TmqAcks    acks;
} TmqConsumeReq;

typedef struct TmqMsgContent {
  int64_t topicId;
  int64_t msgLen;
  char    msg[];
} TmqMsgContent;

typedef struct TmqConsumeRsp {
  TmqMsgHead    head;
  int64_t       bodySize;
  TmqMsgContent msgs[];
} TmqConsumeRsp;

typedef struct TmqSubscribeReq {
  TmqMsgHead head;
  int32_t    topicNum;
  int64_t    topic[];
} TmqSubscribeReq;

typedef struct tmqSubscribeRsp {
  TmqMsgHead head;
  int64_t    vgId;
  char       ep[TSDB_EP_LEN];  // TSDB_EP_LEN
} TmqSubscribeRsp;

typedef struct TmqHeartbeatReq {
} TmqHeartbeatReq;

typedef struct TmqHeartbeatRsp {
} TmqHeartbeatRsp;

typedef struct TqTopicVhandle {
  int64_t topicId;
  // executor for filter
  void* filterExec;
  // callback for mnode
  // trigger when vnode list associated topic change
  void* (*mCallback)(void*, void*);
} TqTopicVhandle;


#define TQ_BUFFER_SIZE 8

typedef struct TqBufferItem {
  int64_t offset;
  // executors are identical but not concurrent
  // so there must be a copy in each item
  void*   executor;
  int64_t size;
  void*   content;
} TqBufferItem;

typedef struct TqBufferHandle {
  // char* topic; //c style, end with '\0'
  // int64_t cgId;
  // void* ahandle;
  int64_t      nextConsumeOffset;
  int64_t      topicId;
  int32_t      head;
  int32_t      tail;
  TqBufferItem buffer[TQ_BUFFER_SIZE];
} TqBufferHandle;

typedef struct TqListHandle {
  TqBufferHandle       bufHandle;
  struct TqListHandle* next;
} TqListHandle;

typedef struct TqGroupHandle {
  int64_t       cId;
  int64_t       cgId;
  void*         ahandle;
  int32_t       topicNum;
  TqListHandle* head;
} TqGroupHandle;

typedef struct TqQueryExec {
  void*         src;
  TqBufferItem* dest;
  void*         executor;
} TqQueryExec;

typedef struct TqQueryMsg {
  TqQueryExec*       exec;
  struct TqQueryMsg* next;
} TqQueryMsg;

typedef struct TqLogReader {
  void* logHandle;
  int32_t (*logRead)(void* logHandle, void** data, int64_t ver);
  int64_t (*logGetFirstVer)(void* logHandle);
  int64_t (*logGetSnapshotVer)(void* logHandle);
  int64_t (*logGetLastVer)(void* logHandle);
} TqLogReader;

typedef struct STqCfg {
  // TODO
} STqCfg;

typedef struct TqMemRef {
  SMemAllocatorFactory *pAlloctorFactory;
  SMemAllocator *pAllocator;
} TqMemRef;

typedef struct TqSerializedHead {
  int16_t ver;
  int16_t action;
  int32_t checksum;
  int64_t ssize;
  char    content[];
} TqSerializedHead;

typedef int (*TqSerializeFun)(const void* pObj, TqSerializedHead** ppHead);
typedef const void* (*TqDeserializeFun)(const TqSerializedHead* pHead, void** ppObj);
typedef void (*TqDeleteFun)(void*);

#define TQ_BUCKET_MASK 0xFF
#define TQ_BUCKET_SIZE 256

#define TQ_PAGE_SIZE 4096
//key + offset + size
#define TQ_IDX_SIZE 24
//4096 / 24
#define TQ_MAX_IDX_ONE_PAGE 170
//24 * 170
#define TQ_IDX_PAGE_BODY_SIZE 4080
//4096 - 4080
#define TQ_IDX_PAGE_HEAD_SIZE 16

#define TQ_ACTION_CONST      0
#define TQ_ACTION_INUSE      1
#define TQ_ACTION_INUSE_CONT 2
#define TQ_ACTION_INTXN      3

#define TQ_SVER              0

//TODO: inplace mode is not implemented
#define TQ_UPDATE_INPLACE    0
#define TQ_UPDATE_APPEND     1

#define TQ_DUP_INTXN_REWRITE 0
#define TQ_DUP_INTXN_REJECT  2

static inline bool TqUpdateAppend(int32_t tqConfigFlag) {
  return tqConfigFlag & TQ_UPDATE_APPEND;
}

static inline bool TqDupIntxnReject(int32_t tqConfigFlag) {
  return tqConfigFlag & TQ_DUP_INTXN_REJECT;
}

static const int8_t TQ_CONST_DELETE = TQ_ACTION_CONST;
#define TQ_DELETE_TOKEN  (void*)&TQ_CONST_DELETE

typedef struct TqMetaHandle {
  int64_t key;
  int64_t offset;
  int64_t serializedSize;
  void*   valueInUse;
  void*   valueInTxn;
} TqMetaHandle;

typedef struct TqMetaList {
  TqMetaHandle handle;
  struct TqMetaList* next;
  //struct TqMetaList* inTxnPrev;
  //struct TqMetaList* inTxnNext;
  struct TqMetaList* unpersistPrev;
  struct TqMetaList* unpersistNext;
} TqMetaList;

typedef struct TqMetaStore {
  TqMetaList*      bucket[TQ_BUCKET_SIZE];
  //a table head
  TqMetaList*      unpersistHead;
 //TODO:temporaral use, to be replaced by unified tfile
  int              fileFd;
  //TODO:temporaral use, to be replaced by unified tfile
  int              idxFd;
  char*            dirPath;
  int32_t          tqConfigFlag;
  TqSerializeFun   pSerializer;
  TqDeserializeFun pDeserializer;
  TqDeleteFun      pDeleter;
} TqMetaStore;

typedef struct STQ {
  // the collection of group handle
  // the handle of kvstore
  char*  path;
  STqCfg*      tqConfig;
  TqLogReader* tqLogReader; 
  TqMemRef     tqMemRef;
  TqMetaStore* tqMeta;
} STQ;

// open in each vnode
STQ* tqOpen(const char* path, STqCfg* tqConfig, TqLogReader* tqLogReader, SMemAllocatorFactory *allocFac);
void tqDestroy(STQ*);

// void* will be replace by a msg type
int tqPushMsg(STQ*, void* msg, int64_t version);
int tqCommit(STQ*);

int tqConsume(STQ*, TmqConsumeReq*);

TqGroupHandle* tqGetGroupHandle(STQ*, int64_t cId);

TqGroupHandle* tqOpenTCGroup(STQ*, int64_t topicId, int64_t cgId, int64_t cId);
int tqCloseTCGroup(STQ*, int64_t topicId, int64_t cgId, int64_t cId);
int tqMoveOffsetToNext(TqGroupHandle*);
int tqResetOffset(STQ*, int64_t topicId, int64_t cgId, int64_t offset);
int tqRegisterContext(TqGroupHandle*, void* ahandle);
int tqLaunchQuery(TqGroupHandle*);
int tqSendLaunchQuery(TqGroupHandle*);

int   tqSerializeGroupHandle(const TqGroupHandle* gHandle, TqSerializedHead** ppHead);

const void* tqDeserializeGroupHandle(const TqSerializedHead* pHead, TqGroupHandle** gHandle);

#ifdef __cplusplus
}
#endif

#endif /*_TD_TQ_H_*/
