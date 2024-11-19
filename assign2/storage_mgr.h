#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dberror.h"

/************************************************************
 *                    Administer data structures                *
 ************************************************************/
typedef struct STORAGE_MANAGER_FILEHANDLE {
	char *FILE_NAME;
	int TOTAL_NUM_PAGES;
	int CUR_PAGE_POS;
	void *mgmtInfo;
} STORAGE_MANAGER_FILEHANDLE;

typedef char* STORAGE_MANAGER_PAGEHANDLE;

/************************************************************
 *                    interface                             *
 ************************************************************/
/* manipulating page files */
extern void INIT_STORAGE_MANAGER (void);
extern RC CREATE_PAGE_FILE (char *FILE_NAME);
extern RC OPEN_PAGE_FILE (char *FILE_NAME, STORAGE_MANAGER_FILEHANDLE *fHandle);
extern RC CLOSE_PAGE_FILE (STORAGE_MANAGER_FILEHANDLE *fHandle);
extern RC DESTROY_PAGE_FILE (char *FILE_NAME);

/* reading blocks from disc */
extern RC READ_BLOCK (int pageNum, STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern int GET_BLOCK_POS (STORAGE_MANAGER_FILEHANDLE *fHandle);
extern RC READ_FIRST_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC READ_PREVIOUS_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC READ_CURRENT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC READ_NEXT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC READ_LAST_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);

/* writing blocks to a page file */
extern RC WRITE_BLOCK (int pageNum, STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC WRITE_CURRENT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage);
extern RC APPEND_EMPTY_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle);
extern RC ENSURE_CAPACITY (int numberOfPages, STORAGE_MANAGER_FILEHANDLE *fHandle);

#endif
