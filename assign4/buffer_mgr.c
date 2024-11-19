#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>


typedef struct PageFrame
{
	SM_PageHandle data;
	PageNumber pageNum;
	int isDirtyBit; 
	int fixCount; 
	int lruCnt;   
	int lfuCnt; 
} PageFrame;


int HIT = 0;
int CLOCK_POINTER = 0;
int LFU_POINTER = 0;
int BUFFER_SIZE = 0;
int REAR_INDEX = 0;
int WRITE_COUNT = 0;


/**
 * @brief Retrieves the number of pages read from disk since the buffer pool was initialized.
 *
 * This function calculates the number of pages read from disk by adding one to the REAR_INDEX value,
 * which represents the index of the last page read into the buffer pool.
 *
 * @param bm Pointer to the buffer pool structure.
 * @return The number of pages read from disk.
 */
extern int getNumReadIO(BM_BufferPool *const bm)
{
    // Adding one to REAR_INDEX because indexing starts from 0
    return (REAR_INDEX + 1);
}


/**
 * @brief Retrieves the number of pages written to the page file since the buffer pool was initialized.
 *
 * This function simply returns the value of WRITE_COUNT, which records the number of writes done by the buffer manager.
 *
 * @param bm Pointer to the buffer pool structure.
 * @return The number of pages written to the page file.
 */
extern int getNumWriteIO(BM_BufferPool *const bm)
{
    return WRITE_COUNT;
}


/**
 * @brief Retrieves an array of page numbers representing the contents of the buffer pool.
 *
 * This function allocates memory for an array of page numbers and populates it with the page numbers of pages currently stored in the buffer pool.
 * If a page frame is empty (unpinned), it is represented by the value NO_PAGE.
 *
 * @param bm Pointer to the buffer pool structure.
 * @return An array of page numbers representing the contents of the buffer pool.
 */
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    // Allocate memory for the array of page numbers
    PageNumber *frmContents = malloc(sizeof(PageNumber) * BUFFER_SIZE);
    // Retrieve the page frames from the buffer pool's management data
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    
    int i = 0;
    // Iterate through all the pages in the buffer pool
    while (i < BUFFER_SIZE)
    {
        // Check if the page frame is occupied
        if (pageFrame[i].pageNum != -1)
        {
            // If occupied, assign the page number to the corresponding index in frmContents
            frmContents[i] = pageFrame[i].pageNum;
        }
        else
        {
            // If not occupied, assign the value NO_PAGE to indicate an empty page frame
            frmContents[i] = NO_PAGE;
        }
        i++;
    }
    return frmContents;
}

/**
 * @brief Implementation of the CLOCK (Second Chance) replacement algorithm.
 *
 * This function replaces a page frame in the buffer pool with a new page using the CLOCK algorithm.
 * The CLOCK algorithm simulates a clock hand that iterates over the page frames in a circular manner.
 * It gives pages a second chance by marking them as referenced (lruCnt = 0) when they are visited.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the new page to be added to the buffer pool.
 */
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page)
{   
    // Retrieve the array of page frames from the buffer pool's management data
    PageFrame *pageFrames = (PageFrame *) bm->mgmtData;
    
    // Iterate over the page frames using the CLOCK algorithm
    while(1)
    {
        // Ensure CLOCK_POINTER wraps around when reaching the end of the page frames array
        CLOCK_POINTER = (CLOCK_POINTER % BUFFER_SIZE == 0) ? 0 : CLOCK_POINTER;

        // Check if the current page frame has not been referenced (lruCnt == 0)
        if(pageFrames[CLOCK_POINTER].lruCnt == 0)
        {
            // If the page in memory has been modified (isDirtyBit = 1), write the page to disk
            if(pageFrames[CLOCK_POINTER].isDirtyBit == 1)
            {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrames[CLOCK_POINTER].pageNum, &fh, pageFrames[CLOCK_POINTER].data);
                
                // Increase the WRITE_COUNT which records the number of writes done by the buffer manager.
                WRITE_COUNT++;
            }
            
            // Set the content of the current page frame to the new page's content
            pageFrames[CLOCK_POINTER].data = page->data;
            pageFrames[CLOCK_POINTER].pageNum = page->pageNum;
            pageFrames[CLOCK_POINTER].isDirtyBit = page->isDirtyBit;
            pageFrames[CLOCK_POINTER].fixCount = page->fixCount;
            pageFrames[CLOCK_POINTER].lruCnt = page->lruCnt;
            CLOCK_POINTER++; // Move CLOCK_POINTER to the next page frame location
            break; // Exit the loop
        }
        else
        {
            // Mark the current page frame as unreferenced (lruCnt = 0) and move to the next page frame
            pageFrames[CLOCK_POINTER++].lruCnt = 0;     
        }
    }
}


/**
 * @brief FIFO (First In First Out) replacement algorithm implementation.
 *
 * This function replaces a page frame in the buffer pool with a new page using the FIFO algorithm.
 * Pages are replaced based on the order they were added to the buffer pool.
 *
 * @param bufferPool Pointer to the buffer pool structure.
 * @param newPage Pointer to the new page to be added to the buffer pool.
 */
extern void FIFO(BM_BufferPool *const bufferPool, PageFrame *newPage)
{
    // Retrieve the array of page frames from the buffer pool's management data
    PageFrame *pageFrames = (PageFrame *) bufferPool->mgmtData;
    
    // Initialize variables for indexing the page frames
    int index, frontIdx;
    frontIdx = REAR_INDEX % BUFFER_SIZE;

    // Iterate through all the page frames in the buffer pool
    for(index = 0; index < BUFFER_SIZE; index++)
    {
        // Check if the fix count for the current page frame is 0, indicating it's not pinned
        if(pageFrames[frontIdx].fixCount == 0)
        {
            // If the page in memory has been modified (isDirtyBit = 1), write the page to disk
            if(pageFrames[frontIdx].isDirtyBit == 1)
            {
                // Open the page file for writing
                SM_FileHandle fileHandle;
                openPageFile(bufferPool->pageFile, &fileHandle);
                
                // Write the page to disk
                writeBlock(pageFrames[frontIdx].pageNum, &fileHandle, pageFrames[frontIdx].data);
                
                // Increase the WRITE_COUNT which records the number of writes done by the buffer manager.
                WRITE_COUNT++;
            }
            
            // Replace the content of the page frame with the content of the new page
            pageFrames[frontIdx].isDirtyBit = newPage->isDirtyBit;
            pageFrames[frontIdx].fixCount = newPage->fixCount;
            pageFrames[frontIdx].data = newPage->data;
            pageFrames[frontIdx].pageNum = newPage->pageNum;
            break; // Exit the loop after finding a suitable page frame for replacement
        }
        else
        {
            // Move to the next page frame
            frontIdx++;
            frontIdx = (frontIdx % BUFFER_SIZE == 0) ? 0 : frontIdx; // Wrap around if necessary
        }
    }
}


/**
 * @brief Implementation of the LFU (Least Frequently Used) replacement algorithm.
 *
 * This function replaces a page frame in the buffer pool with a new page using the LFU algorithm.
 * Pages are replaced based on the least frequently used criterion.
 *
 * @param bufferPool Pointer to the buffer pool structure.
 * @param newPage Pointer to the new page to be added to the buffer pool.
 */
extern void LFU(BM_BufferPool *const bufferPool, PageFrame *newPage)
{

    // Retrieve the array of page frames from the buffer pool's management data
    PageFrame *pageFrames = (PageFrame *) bufferPool->mgmtData;
    
    int index, leastFreqIndex, leastFreqCount;
    leastFreqIndex = LFU_POINTER;	
    
    // Iterate through all the page frames in the buffer pool to find the least frequently used page frame
    for(index = 0; index < BUFFER_SIZE; index++)
    {
        if(pageFrames[leastFreqIndex].fixCount == 0)
        {
            leastFreqIndex = (leastFreqIndex + index) % BUFFER_SIZE;
            leastFreqCount = pageFrames[leastFreqIndex].lruCnt;
            break;
        }
    }

    index = (leastFreqIndex + 1) % BUFFER_SIZE;

    // Find the page frame having the minimum lruCnt (i.e., it is used the least frequent)
    for(int i = 0; i < BUFFER_SIZE; i++)
    {
        if(pageFrames[index].lruCnt < leastFreqCount)
        {
            leastFreqIndex = index;
            leastFreqCount = pageFrames[index].lruCnt;
        }
        index = (index + 1) % BUFFER_SIZE;
    }
        
    // If the page in memory has been modified (isDirtyBit = 1), write the page to disk	
    if(pageFrames[leastFreqIndex].isDirtyBit == 1)
    {
        SM_FileHandle fileHandle;
        openPageFile(bufferPool->pageFile, &fileHandle);
        writeBlock(pageFrames[leastFreqIndex].pageNum, &fileHandle, pageFrames[leastFreqIndex].data);
        
        // Increase the WRITE_COUNT which records the number of writes done by the buffer manager.
        WRITE_COUNT++;
    }
    
    // Set the content of the page frame to the new page's content		
    pageFrames[leastFreqIndex].data = newPage->data;
    pageFrames[leastFreqIndex].pageNum = newPage->pageNum;
    pageFrames[leastFreqIndex].isDirtyBit = newPage->isDirtyBit;
    pageFrames[leastFreqIndex].fixCount = newPage->fixCount;
    LFU_POINTER = leastFreqIndex + 1;
}


/**
 * @brief Implementation of the LRU (Least Recently Used) replacement algorithm.
 *
 * This function replaces a page frame in the buffer pool with a new page using the LRU algorithm.
 * Pages are replaced based on the least recently used criterion.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the new page to be added to the buffer pool.
 */
extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{	
    // Retrieve the array of page frames from the buffer pool's management data
    PageFrame *pageFrames = (PageFrame *) bm->mgmtData;
    int leastLRUIndex = 0;
    
    // Find the least recently used page frame
    for(int i = 0; i < BUFFER_SIZE; i++)
    {
        // Find the first page frame with fixCount = 0, indicating it's not pinned
        if(pageFrames[i].fixCount == 0)
        {
            leastLRUIndex = i;
            break;
        }
    }	

    // Iterate through the remaining page frames to find the least recently used page frame
    for(int i = leastLRUIndex + 1; i < BUFFER_SIZE; i++)
    {
        if(pageFrames[i].lruCnt < pageFrames[leastLRUIndex].lruCnt)
        {
            leastLRUIndex = i;
        }
    }

    // If the page in memory has been modified (isDirtyBit = 1), write the page to disk
    if(pageFrames[leastLRUIndex].isDirtyBit == 1)
    {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrames[leastLRUIndex].pageNum, &fh, pageFrames[leastLRUIndex].data);
        
        // Increase the WRITE_COUNT which records the number of writes done by the buffer manager.
        WRITE_COUNT++;
    }
    
    // Set the content of the least recently used page frame to the new page's content
    pageFrames[leastLRUIndex].data = page->data;
    pageFrames[leastLRUIndex].pageNum = page->pageNum;
    pageFrames[leastLRUIndex].isDirtyBit = page->isDirtyBit;
    pageFrames[leastLRUIndex].fixCount = page->fixCount;
    pageFrames[leastLRUIndex].lruCnt = page->lruCnt;
}




/** 
 * @brief Initializes a buffer pool with numPages page frames.
 *
 * This function initializes a buffer pool with a specified number of page frames.
 * It assigns the given page file name, number of pages, and page replacement strategy to the buffer pool.
 * Each page frame is initialized with default values.
 *
 * @param bm Pointer to the buffer pool structure to be initialized.
 * @param pageFileName Name of the page file whose pages will be cached in memory.
 * @param numPages Number of page frames in the buffer pool.
 * @param strategy Page replacement strategy (FIFO, LRU, LFU, CLOCK) to be used by the buffer pool.
 * @param stratData Pointer to parameters needed for the page replacement strategy (not used in this implementation).
 *
 * @return RC_OK on success, or an error code on failure.
 */
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData)
{
	// Assign page file name, number of pages, and page replacement strategy to the buffer pool
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;

	// Allocate memory space = number of pages x space required for one page
	PageFrame *page = malloc(sizeof(PageFrame) * numPages);
	
	// Set the total number of pages in memory or the buffer pool
	BUFFER_SIZE = numPages;	
	
	int i;

	// Initialize all pages in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// Set fields (variables) of each page to default values
		page[i].data = NULL;           // No data initially
		page[i].pageNum = -1;          // Page number not assigned
		page[i].isDirtyBit = 0;        // Not modified initially
		page[i].fixCount = 0;          // Not fixed initially
		page[i].lruCnt = 0;            // Least Recently Used count
		page[i].lfuCnt = 0;            // Least Frequently Used count
	}

	// Set the buffer pool's management data to point to the allocated memory for page frames
	bm->mgmtData = page;

	// Initialize counters and pointers used by replacement algorithms
	WRITE_COUNT = CLOCK_POINTER = LFU_POINTER = 0;

	return RC_OK;
		
}


/** 
 * @brief Shuts down the buffer pool, releasing all resources and freeing memory.
 *
 * This function closes the buffer pool, removing all pages from memory and releasing all associated resources.
 * It ensures that all dirty pages (modified pages) are written back to disk before shutting down.
 * If any pages are still pinned (being used by clients), it returns an error.
 *
 * @param bm Pointer to the buffer pool structure to be shut down.
 *
 * @return RC_OK on success, or an error code (RC_PINNED_PAGES_IN_BUFFER) if there are still pinned pages.
 */
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	// Write all dirty pages back to disk before shutting down
	forceFlushPool(bm);

	int i;

	// Check if there are any pinned pages in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// If fixCount != 0, it means the page is still pinned by some client
		if(pageFrame[i].fixCount != 0)
		{
			return RC_PINNED_PAGES_IN_BUFFER;
		}
	}

	// Free memory occupied by the page frames and set mgmtData to NULL
	free(pageFrame);
	bm->mgmtData = NULL;

	return RC_OK;
}


/**
 * @brief Writes all dirty pages (pages with fixCount = 0 and isDirtyBit = 1) to disk.
 *
 * This function forces the buffer pool to write all dirty pages (modified pages) back to the page file on disk.
 * It iterates through all page frames in the buffer pool, checks if a page is dirty and unpinned, and writes it to disk.
 * After writing, it marks the page as not dirty and increments the WRITE_COUNT.
 *
 * @param bm Pointer to the buffer pool structure.
 *
 * @return RC_OK on success.
 */
extern RC forceFlushPool(BM_BufferPool *const bm)
{
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
	int i;
	// Iterate through all page frames in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// Check if the page is dirty and unpinned
		if(pageFrame[i].fixCount == 0 && pageFrame[i].isDirtyBit == 1)
		{
			SM_FileHandle filehandle;
			// Open the page file on disk
			openPageFile(bm->pageFile, &filehandle);
			// Write the block of data to the page file on disk
			writeBlock(pageFrame[i].pageNum, &filehandle, pageFrame[i].data);
			// Mark the page as not dirty
			pageFrame[i].isDirtyBit = 0;
			// Increment the WRITE_COUNT which records the number of writes done by the buffer manager
			WRITE_COUNT++;
		}
	}	
	return RC_OK;
}


/**
 * @brief Marks a page as dirty, indicating that its data has been modified by the client.
 *
 * This function sets the isDirtyBit flag to 1 for the specified page in the buffer pool, indicating that the page's data has been modified.
 * It iterates through all pages in the buffer pool to find the page with the specified page number and marks it as dirty.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the page handle structure containing information about the page to be marked as dirty.
 *
 * @return RC_OK if the page is successfully marked as dirty, RC_ERROR if the page is not found in the buffer pool.
 */
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
	int i;
	// Iterate through all pages in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// If the current page is the page to be marked dirty, set isDirtyBit = 1 for that page
		if(pageFrame[i].pageNum == page->pageNum)
		{
			pageFrame[i].isDirtyBit = 1;
			return RC_OK;		
		}			
	}		
	// Return error if the page is not found in the buffer pool
	return RC_ERROR;
}


/**
 * @brief Unpins a page from memory, indicating that the client has completed work on the page.
 *
 * This function decreases the fixCount for the specified page in the buffer pool, indicating that the client has completed its work with the page.
 * It iterates through all pages in the buffer pool to find the page with the specified page number and decreases its fixCount.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the page handle structure containing information about the page to be unpinned.
 *
 * @return RC_OK once the page is successfully unpinned.
 */
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const pg)
{	
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
	int i;
	// Iterate through all pages in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// If the current page is the page to be unpinned, decrease its fixCount and exit loop
		if(pageFrame[i].pageNum == pg->pageNum)
		{
			pageFrame[i].fixCount--;
			break;		
		}		
	}
	return RC_OK;
}


/**
 * @brief Writes the contents of the modified pages back to the page file on disk.
 *
 * This function iterates through all pages in the buffer pool to find the page with the specified page number.
 * If the page is found and marked dirty (indicating modification), it writes the page's data back to the disk using the storage manager functions.
 * After writing, it marks the page as not dirty and increments the WRITE_COUNT to record the write operation.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the page handle structure containing information about the page to be written back to disk.
 *
 * @return RC_OK once the page has been successfully written back to disk.
 */
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const pg)
{
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
	int i;
	// Iterate through all pages in the buffer pool
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		// If the current page matches the page to be written to disk
		if(pageFrame[i].pageNum == pg->pageNum)
		{		
			// Open the page file on disk
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			
			// Write the page's data to the disk
			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
		
			// Mark the page as not dirty because the modified page has been written to disk
			pageFrame[i].isDirtyBit = 0;
			
			// Increment the WRITE_COUNT which records the number of writes done by the buffer manager
			WRITE_COUNT++;
		}
	}	
	return RC_OK;
}


/**
 * @brief Pins a page with the specified page number in the buffer pool.
 *
 * This function adds the page with the given page number to the buffer pool. If the buffer pool is full, it uses the specified page replacement strategy
 * to replace an existing page in memory with the new page being pinned. The function also handles cases where the buffer pool is empty or where the page
 * is already present in memory.
 *
 * @param bm Pointer to the buffer pool structure.
 * @param page Pointer to the page handle structure that will store information about the pinned page.
 * @param pageNum Page number of the page to be pinned.
 *
 * @return RC_OK if the page is successfully pinned in the buffer pool, or an appropriate error code otherwise.
 */
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	// Get pointer to the array of page frames in the buffer pool
	PageFrame *pgFrame = (PageFrame *)bm->mgmtData;
	
	// Checking if buffer pool is empty and this is the first page to be pinned
	if (pgFrame[0].pageNum == -1)
	{
		// Reading page from disk and initializing page frame's content in the buffer pool
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		pgFrame[0].data = (SM_PageHandle)malloc(PAGE_SIZE);
		ensureCapacity(pageNum, &fh);
		readBlock(pageNum, &fh, pgFrame[0].data);
		pgFrame[0].pageNum = pageNum;
		pgFrame[0].fixCount++;
		REAR_INDEX = HIT = 0;
		pgFrame[0].lruCnt = HIT;
		pgFrame[0].lfuCnt = 0;
		page->pageNum = pageNum;
		page->data = pgFrame[0].data;
		return RC_OK;
	}
	else
	{
		int i;
		bool isBufferFull = true;

		for (i = 0; i < BUFFER_SIZE; i++)
		{
			if (pgFrame[i].pageNum != -1)
			{
				// Checking if page is in memory
				if (pgFrame[i].pageNum == pageNum)
				{
					// Increasing fixCount i.e. now there is one more client accessing this page
					pgFrame[i].fixCount++;
					isBufferFull = false;
					HIT++; // Incrementing HIT (HIT is used by LRU algorithm to determine the least recently used page)

					// Update page's reference count based on replacement strategy
					if (bm->strategy == RS_LRU)
						pgFrame[i].lruCnt = HIT;
					else if (bm->strategy == RS_CLOCK)
						pgFrame[i].lruCnt = 1; // lruCnt = 1 to indicate that this was the last page frame examined (added to the buffer pool)
					else if (bm->strategy == RS_LFU)
						pgFrame[i].lfuCnt++; // Incrementing lfuCnt to add one more to the count of number of times the page is used (referenced)

					page->pageNum = pageNum;
					page->data = pgFrame[i].data;

					CLOCK_POINTER++;
					break;
				}
			}
			else
			{
				SM_FileHandle fh;
				openPageFile(bm->pageFile, &fh);
				pgFrame[i].data = (SM_PageHandle)malloc(PAGE_SIZE);
				readBlock(pageNum, &fh, pgFrame[i].data);
				pgFrame[i].pageNum = pageNum;
				pgFrame[i].fixCount = 1;
				pgFrame[i].lfuCnt = 0;
				REAR_INDEX++;
				HIT++; // Incrementing HIT (HIT is used by LRU algorithm to determine the least recently used page)

				// Update page's reference count based on replacement strategy
				if (bm->strategy == RS_LRU)
					pgFrame[i].lruCnt = HIT;
				else if (bm->strategy == RS_CLOCK)
					pgFrame[i].lruCnt = 1;

				page->pageNum = pageNum;
				page->data = pgFrame[i].data;

				isBufferFull = false;
				break;
			}
		}

		// If isBufferFull = true, then it means that the buffer is full and we must replace an existing page using page replacement strategy
		if (isBufferFull == true)
		{
			// Create a new page to store data read from the file.
			PageFrame *new_Page = (PageFrame *)malloc(sizeof(PageFrame));

			// Reading page from disk and initializing page frame's content in the buffer pool
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			new_Page->data = (SM_PageHandle)malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, new_Page->data);
			new_Page->pageNum = pageNum;
			new_Page->isDirtyBit = 0;
			new_Page->fixCount = 1;
			new_Page->lfuCnt = 0;
			REAR_INDEX++;
			HIT++;

			// Update page's reference count based on replacement strategy
			if (bm->strategy == RS_LRU)
				new_Page->lruCnt = HIT;
			else if (bm->strategy == RS_CLOCK)
				new_Page->lruCnt = 1;

			page->pageNum = pageNum;
			page->data = new_Page->data;
		

			// Call appropriate algorithm's function depending on the page replacement strategy selected (passed through parameters)
			switch(bm->strategy)
			{			
				case RS_FIFO:
					FIFO(bm, new_Page);
					break;
				
				case RS_LRU:
					LRU(bm, new_Page);
					break;
				
				case RS_CLOCK: 
					CLOCK(bm, new_Page);
					break;
  				
				case RS_LFU: 
					LFU(bm, new_Page);
					break;
  				
				case RS_LRU_K:
					printf("\n LRU-k algorithm is not implemented");
					break;
				
				default:
					printf("\nAlgorithm is Not Implemented\n");
					break;
			}
						
		}		
		return RC_OK;
	}	
}






/**
 * @brief Retrieves an array of boolean values indicating whether each page in the buffer pool is dirty.
 *
 * This function allocates memory for an array of booleans and populates it with the dirty status (isDirtyBit) of each page.
 *
 * @param bm Pointer to the buffer pool structure.
 * @return An array of boolean values representing the dirty status of each page.
 */
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
    // Allocate memory for the array of boolean values
    bool *dirtyFlags = malloc(sizeof(bool) * BUFFER_SIZE);
    // Retrieve the page frames from the buffer pool's management data
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    
    int i;
    // Iterate through all the pages in the buffer pool
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        // Set the value of dirtyFlags based on the isDirtyBit of each page
        dirtyFlags[i] = (pageFrame[i].isDirtyBit == 1) ? true : false;
    }   
    return dirtyFlags;
}


/**
 * @brief Retrieves an array of integers representing the fix count of each page in the buffer pool.
 *
 * This function allocates memory for an array of integers and populates it with the fix count of each page.
 *
 * @param bm Pointer to the buffer pool structure.
 * @return An array of integers representing the fix count of each page.
 */
extern int *getFixCounts(BM_BufferPool *const bm)
{
    // Allocate memory for the array of integers
    int *fixCounts = malloc(sizeof(int) * BUFFER_SIZE);
    // Retrieve the page frames from the buffer pool's management data
    PageFrame *pgFrame = (PageFrame *)bm->mgmtData;
    
    int i = 0;
    // Iterate through all the pages in the buffer pool
    while (i < BUFFER_SIZE)
    {
        // Set the value of fixCounts based on the fixCount of each page
        fixCounts[i] = (pgFrame[i].fixCount != -1) ? pgFrame[i].fixCount : 0;
        i++;
    }   
    return fixCounts;
}






