#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "file.h"
#include "buffer.h"

#define SUCCESS 0
#define FAIL -1
// #define LEAF_ORDER 32
// #define INTERNAL_ORDER 249
#define LEAF_ORDER 3
#define INTERNAL_ORDER 3

//pinMax: 3
int db_insert(int tableId, int64_t key, char *value);
int startNewTree(int tableId, int64_t key, char* value);
int insertIntoLeaf(int tableId, pagenum_t pageNum, int64_t key, char* value);
int insertIntoLeafAfterSplitting(int tableId, pagenum_t leafPageNum, int64_t key, char* value);
int cut(int length);
int insertIntoParent(int tableId, pagenum_t leftChildPageNum, int64_t newKey, pagenum_t rightChildPageNum);

// make root that has left leaf as leftmost child, right leaf as first child
int insertIntoNewRoot(int tableId, pagenum_t leftLeafPageNum, int64_t newKey, pagenum_t rightLeafPageNum);
int getIndexOfLeft(int tableId, pagenum_t parentPageNum, pagenum_t childPageNum);
int insertIntoInternal(int tableId, pagenum_t parentPageNum, int leftChildIndex, int64_t newKey, pagenum_t rightChildPageNum);
int insertIntoInternalAfterSplitting(int tableId, pagenum_t parentPageNum, int leftChildIndex, int64_t newKey, pagenum_t rightChildPageNum);

//pinMax: 1
int db_find(int tableId, int64_t key, char *ret_val);
//pinMax: 1
pagenum_t findLeaf(int tableId, int64_t key);

//pinMax: 2
int db_delete(int tableId, int64_t key);
//delete entry from internal or leaf
int deleteEntry(int tableId, pagenum_t pageNum, int64_t key);
void removeEntryFromPage(int tableId, pagenum_t pageNum, int64_t key);
int adjustRoot(int tableId, pagenum_t pageNum);
int getNeighborIndex(int tableId, pagenum_t pageNum);
int coalescePages(int tableId, pagenum_t neighborPageNum , int64_t kPrime, pagenum_t pageNum, int neighborIndex);
int redistributePages(int tableId, pagenum_t neighborPageNum, int64_t kPrime, pagenum_t pageNum, int neighborIndex, int kPrimeIndex);

int open_table(char *pathname);

/* 
Allocate the buffer pool (array) with the given number of entries. 
• Initialize other fields (state info, LRU info..) with your own design.
• If success, return 0. Otherwise, return non-zero value.
*/
int init_db (int buf_num);
/*
 • Write all pages of this table from buffer to disk and discard the table id. 
 • If success, return 0. Otherwise, return non-zero value.
*/
int close_table(int table_id);

//flush all data from buffer and destroy allocated buffer.
//If success, return 0. Otherwise, return -1
int shutdown_db();


#endif /* __BPT_H__*/
