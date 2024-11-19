#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H


// Incorporate return codes and mechanisms for logging errors
#include "dberror.h"

// Incorporate a boolean variable named DT.
#include "dt.h"

// Replacement Strategies
typedef enum REPLACEMENT_STRATEGY {
	RS_FIFO = 0,
	RS_LRU = 1,
	RS_CLOCK = 2,
	RS_LFU = 3,
	RS_LRU_K = 4
} REPLACEMENT_STRATEGY;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BUFFER_MANAGER_BUFFERPOOL {
	char *PAGE_FILE;
	int NUM_PAGES;
	REPLACEMENT_STRATEGY STRATEGY;
	void *mgmtData; // Utilize this one to maintain the administrative details within your buffer
	// manager requires for a buffer pool
} BUFFER_MANAGER_BUFFERPOOL;

typedef struct BUFFER_MANAGER_PAGEHANDLE {
	PageNumber PAGE_NUM;
	char *data;
} BUFFER_MANAGER_PAGEHANDLE;

// convenience macros
#define MAKE_POOL()					\
		((BUFFER_MANAGER_BUFFERPOOL *) malloc (sizeof(BUFFER_MANAGER_BUFFERPOOL)))

#define MAKE_PAGE_HANDLE()				\
		((BUFFER_MANAGER_PAGEHANDLE *) malloc (sizeof(BUFFER_MANAGER_PAGEHANDLE)))

// Buffer Manager Interface for Pool Management
RC INIT_BUFFER_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM, const char *const PAGE_FILEName, 
		const int NUM_PAGES, REPLACEMENT_STRATEGY STRATEGY,
		void *stratData);
RC SHUTDOWN_BUFFER_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM);
RC FORCE_FLUSH_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM);


// Buffer Manager Interface for Page Access
RC MARK_DIRTY (BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page);
RC UNPIN_PAGE (BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page);
RC FORCE_PAGE (BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page);
RC PIN_PAGE (BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page, 
		const PageNumber PAGE_NUM);

// Statistics Interface
PageNumber *GET_FRAME_CONTENTS (BUFFER_MANAGER_BUFFERPOOL *const BM);
bool *GET_DIRTY_FLAGS (BUFFER_MANAGER_BUFFERPOOL *const BM);
int *GET_FIX_COUNTS (BUFFER_MANAGER_BUFFERPOOL *const BM);
int GET_NUM_READ_IO (BUFFER_MANAGER_BUFFERPOOL *const BM);
int GET_NUM_WRITE_IO (BUFFER_MANAGER_BUFFERPOOL *const BM);

#endif
