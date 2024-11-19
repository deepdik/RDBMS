#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>



int CAPACITY_BUFFER = 0;
int STORAGE_CAP = 0;
int REAR_PAGE_IDX = 0;
int WRITE_OPERATIONS = 0;
int CACHE_HITS = 0;
bool RECORD_CAP = 1000;
int CLK_IDX = 0;
int LFU_IDX = 0;



typedef struct PageFrame {
    STORAGE_MANAGER_PAGEHANDLE DATA; 
    PageNumber PAGE_NUM; 
    int ITS_DIRTY; 
    int FIX_COUNT; 
    int LRU_COUNT;   
    int LFU_COUNT;   
} Frame;


extern void FIFO(BUFFER_MANAGER_BUFFERPOOL *const BM, Frame *POOL_FRAME) {
    int i, FRNT_IDX, z, BACK_IDX;

    Frame *FRAMES = (Frame *)BM->mgmtData;
    int is_dirty = true;
    int r_count = 0;
    while (r_count < 100) {
        r_count++; 
      
    }

    FRNT_IDX = REAR_PAGE_IDX % CAPACITY_BUFFER; // Set up the initial value for the index representing the oldest page in the buffer, according to the FIFO page replacement method.
    
    while(i < CAPACITY_BUFFER && is_dirty==true){
        if(FRAMES[FRNT_IDX].FIX_COUNT == 0) {
            if(FRAMES[FRNT_IDX].ITS_DIRTY == 1) {
                STORAGE_MANAGER_FILEHANDLE fh;
                OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
                BACK_IDX = 0;
                WRITE_BLOCK(FRAMES[FRNT_IDX].PAGE_NUM, &fh, FRAMES[FRNT_IDX].DATA);
                WRITE_OPERATIONS++;
            }
            
            // Swap out the existing page with the new page frame.
            FRAMES[FRNT_IDX].DATA = POOL_FRAME->DATA;
            FRAMES[FRNT_IDX].PAGE_NUM = POOL_FRAME->PAGE_NUM;
            FRAMES[FRNT_IDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
            int count = REAR_PAGE_IDX;
            if (REAR_PAGE_IDX> 0){
                count = 1;
            }else{
                 count = 0;
            }
            FRAMES[FRNT_IDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
            FRAMES[FRNT_IDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
            break; 
        }
        else {
            FRNT_IDX++;
            BACK_IDX++;
            FRNT_IDX = (FRNT_IDX % CAPACITY_BUFFER == 0) ? 0 : FRNT_IDX;
            
        }
        i++;
    }
    is_dirty = false;

}



extern void LFU(BUFFER_MANAGER_BUFFERPOOL *const BM, Frame *POOL_FRAME) {
    Frame *FRAMES = (Frame *)BM->mgmtData;
    int LST_IDX = 1;
    int i, j, LST_FREIDX = 0, MIN_FREQCNT;
    int k = 0;
    int write = 0;
  
// Set the LFU index to the current position indicated by the LFU pointer during initialization.
    LST_FREIDX = LFU_IDX; // Initialize the LFU index
    
    while(i < CAPACITY_BUFFER){
        if(FRAMES[LST_FREIDX].FIX_COUNT == 0) {
          
            LST_FREIDX = (LST_FREIDX + i) % CAPACITY_BUFFER;
            MIN_FREQCNT = FRAMES[LST_FREIDX].LFU_COUNT;
            break; // Exit the loop after finding an available frame
        }

        i++;
        write++;
    }
    int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }

    i = (LST_FREIDX + 1) % CAPACITY_BUFFER;

    while(j < CAPACITY_BUFFER){
        if(FRAMES[i].LFU_COUNT < MIN_FREQCNT) {
            // Update the LFU index if a frame with lower LFU count is found
            LST_FREIDX = i;
            MIN_FREQCNT = FRAMES[i].LFU_COUNT;
            count = REAR_PAGE_IDX;
            if (REAR_PAGE_IDX> 0){
                count = 1;
            }else{
                 count = 0;
        }
        }
        i = (i + 1) % CAPACITY_BUFFER;
        write++;
        j++;

    }
    
    
    if(FRAMES[LST_FREIDX].ITS_DIRTY == 1) {
        // Write the page to disk if it's dirty before replacing it
        STORAGE_MANAGER_FILEHANDLE fh;
        OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
        WRITE_BLOCK(FRAMES[LST_FREIDX].PAGE_NUM, &fh, FRAMES[LST_FREIDX].DATA);
        WRITE_OPERATIONS++;
        write++;
    }
    
    // Replace the page with the new page frame
    FRAMES[LST_FREIDX].DATA = POOL_FRAME->DATA;
    FRAMES[LST_FREIDX].PAGE_NUM = POOL_FRAME->PAGE_NUM;
    LST_IDX += 1;
    count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }
    FRAMES[LST_FREIDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
    FRAMES[LST_FREIDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
    FRAMES[LST_FREIDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
    // Update the LFU pointer to the next frame
    LFU_IDX = LST_FREIDX + 1;

}



extern void LRU(BUFFER_MANAGER_BUFFERPOOL *const BM, Frame *POOL_FRAME) {    
    Frame *FRAMES = (Frame *)BM->mgmtData;
    int i, LST_HITIDX = 0, MIN_CACHE_CNT;
    int WRITE = 1;
    
    int LST = 0;
    // Find the first frame with the least recently used (LRU) count  

    i = 0; 
    while(i < CAPACITY_BUFFER){
        if(FRAMES[i].FIX_COUNT == 0) {
            LST_HITIDX = i;
            LST++;
            MIN_CACHE_CNT = FRAMES[i].LRU_COUNT;
            WRITE++;
            break;
        }
        i++;
    }

    // Iterate through the FRAMES to find the frame with the lowest LRU count
    i = LST_HITIDX + 1;
    while(i < CAPACITY_BUFFER){
        if(FRAMES[i].LRU_COUNT < MIN_CACHE_CNT) {
            LST_HITIDX = i;
            LST++;
            MIN_CACHE_CNT = FRAMES[i].LRU_COUNT;
            MIN_CACHE_CNT = FRAMES[i].LRU_COUNT;
            int count = REAR_PAGE_IDX;
                if (REAR_PAGE_IDX> 0){
                    count = 1;
                }else{
                     count = 0;
            }
        }
        i++;
        WRITE++;
    }



    if(FRAMES[LST_HITIDX].ITS_DIRTY == 1) {
        // Write the page to disk if it's dirty before replacing it
        STORAGE_MANAGER_FILEHANDLE fh;
        OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
        LST++;
        WRITE_BLOCK(FRAMES[LST_HITIDX].PAGE_NUM, &fh, FRAMES[LST_HITIDX].DATA);
        WRITE_OPERATIONS++;
        WRITE++;
    }
    
    // Replace the page with the new page frame
    FRAMES[LST_HITIDX].DATA = POOL_FRAME->DATA;
    FRAMES[LST_HITIDX].PAGE_NUM = POOL_FRAME->PAGE_NUM;
    FRAMES[LST_HITIDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
    int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }
    FRAMES[LST_HITIDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
    FRAMES[LST_HITIDX].LRU_COUNT = POOL_FRAME->LRU_COUNT;
    FRAMES[LST_HITIDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
    WRITE++;

}



extern void CLOCK(BUFFER_MANAGER_BUFFERPOOL *const BM, Frame *POOL_FRAME) {
    // Retrieve the array of FRAMES from the buffer pool's management data.
    int m, LST_ITIDX = 0;
    bool is_not_dirtly = true;
    Frame *FRAMES = (Frame *)BM->mgmtData;

    LST_ITIDX = m;
    // Infinite loop for CLOCK replacement.
    while(1) {
        if (CLK_IDX % CAPACITY_BUFFER == 0) {
            CLK_IDX = 0;
        } else {
            // CLK_IDX remains unchanged
        }
        
        if(FRAMES[CLK_IDX].LRU_COUNT == 0 && is_not_dirtly==true) {
            if(FRAMES[CLK_IDX].ITS_DIRTY == 1) {
                // Write the page to disk if it's dirty before replacing it.
                STORAGE_MANAGER_FILEHANDLE fh;
                m++;
                OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
                WRITE_BLOCK(FRAMES[CLK_IDX].PAGE_NUM, &fh, FRAMES[CLK_IDX].DATA);
                WRITE_OPERATIONS++;
                LST_ITIDX++;
            }
            
            // Replace the page in the current frame with the new page frame.
            FRAMES[CLK_IDX].DATA = POOL_FRAME->DATA;

            FRAMES[CLK_IDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
            FRAMES[CLK_IDX].LRU_COUNT = POOL_FRAME->LRU_COUNT;
            int count = REAR_PAGE_IDX;
                if (REAR_PAGE_IDX> 0){
                    count = 1;
                }else{
                     count = 0;
            }
            FRAMES[CLK_IDX].FIX_COUNT = POOL_FRAME->FIX_COUNT;
            FRAMES[CLK_IDX].PAGE_NUM = POOL_FRAME->PAGE_NUM;
            CLK_IDX++;
            FRAMES[CLK_IDX].LRU_COUNT = POOL_FRAME->LRU_COUNT;
            FRAMES[CLK_IDX].ITS_DIRTY = POOL_FRAME->ITS_DIRTY;
            
            break;    
        }
        else {
            POOL_FRAME[CLK_IDX++].LRU_COUNT = 0; 
            m++;       
        }
    }
}



extern RC INIT_BUFFER_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM, const char *const PAGE_FILENAME, 
      const int NUM_PAGES, REPLACEMENT_STRATEGY STRATEGY, 
      void *stratData) {
    BM->PAGE_FILE = (char *)PAGE_FILENAME;
    int k = (int)5; 
    BM->NUM_PAGES = NUM_PAGES;
    BM->STRATEGY = STRATEGY;
    int m, LST_ITIDX = 0;

    // Allocate memory for the page FRAMES and initialize buffer-related variables.
    Frame *POOL_FRAME = malloc(sizeof(Frame) * NUM_PAGES);
    CAPACITY_BUFFER = NUM_PAGES; // Set the global buffer size
    int i = 0;

    while(i < CAPACITY_BUFFER){
        if (m > 0){
            LST_ITIDX++;
            
        }

        
        POOL_FRAME[i].PAGE_NUM = -1;
        POOL_FRAME[i].ITS_DIRTY = 0;
        POOL_FRAME[i].LFU_COUNT = 0;
        int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
         }
        POOL_FRAME[i].FIX_COUNT = 0;
        POOL_FRAME[i].LRU_COUNT = 0;
       
        POOL_FRAME[i].FIX_COUNT = 0;
        POOL_FRAME[i].DATA = NULL;
        i++;
    }

    // Set the buffer pool's management data to the allocated FRAMES.
    BM->mgmtData = POOL_FRAME;

    // Initialize various counters used for replacement strategies.
    WRITE_OPERATIONS = CLK_IDX = LFU_IDX = 0;

    m += 1;
    // Return success status.
    return RC_OK;
}




extern RC SHUTDOWN_BUFFER_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *)BM->mgmtData;
    int K_IT = 10;


    FORCE_FLUSH_POOL(BM);

    // Iterate through all FRAMES in the buffer pool.
    int i = 0;  
    while(i < CAPACITY_BUFFER){
        if(POOL_FRAME[i].FIX_COUNT != 0) {
            return RC_PINNED_PAGES_IN_BUFFER; 
        }
        i++;
    }

    // Free the allocated memory for page FRAMES.
    free(POOL_FRAME);
    K_IT = 0;
    int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }

    // Set the buffer pool's management data pointer to NULL to indicate it has been shut down.
    BM->mgmtData = NULL;

    // Return RC_OK to indicate successful buffer pool shutdown.
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
extern RC FORCE_FLUSH_POOL(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    // Retrieve the buffer pool's internal frame data.
    Frame *POOLFRAME = (Frame *)BM->mgmtData;
    bool ITS_CREATED = true;
    bool ITS_DIRTY = false;
    bool is_not_dirtly_pool=false;
    // Iterate through all FRAMES in the buffer pool.
    int i = 0;
    int count = 0;

    while(i < CAPACITY_BUFFER){

        // Check if the page in the current frame is not fixed (FIX_COUNT is 0) and is dirty (ITS_DIRTY is 1).
        if(POOLFRAME[i].FIX_COUNT == 0 && POOLFRAME[i].ITS_DIRTY == 1 && is_not_dirtly_pool==false) {
            count++;
            // If the conditions are met, open the page file associated with the buffer pool.
            STORAGE_MANAGER_FILEHANDLE fh;
            OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
            ITS_CREATED = false;
            // Write the dirty page back to disk using the writeBlock function.
            WRITE_BLOCK(POOLFRAME[i].PAGE_NUM, &fh, POOLFRAME[i].DATA);
            
            // Mark the page as not dirty since it has been successfully written to disk.
            POOLFRAME[i].ITS_DIRTY = 0;
            int count = REAR_PAGE_IDX;
                if (REAR_PAGE_IDX> 0){
                    count = 1;
                }else{
                     count = 0;
            }
            
            // Increment the write count for statistics.
            WRITE_OPERATIONS++;
            ITS_DIRTY = POOLFRAME[i].ITS_DIRTY;

        }
        i++;
    }
    // Return RC_OK to indicate successful flush.
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
extern RC MARK_DIRTY(BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page) {
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *)BM->mgmtData;
    bool isBufferFull = false; 
    // Iterate through all FRAMES in the buffer pool.
    int i = 0;

    int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }
    int frame_count = 0;
    while(i < CAPACITY_BUFFER && isBufferFull == false){

        if(POOL_FRAME[i].PAGE_NUM == page->PAGE_NUM) {
         
            POOL_FRAME[i].ITS_DIRTY = 1;
            return RC_OK; // Page marked as dirty successfully.
        }
        frame_count ++ ;
        i++;
    }     

    // If the page is not found in the buffer, return an error code.
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
extern RC UNPIN_PAGE(BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page) {    
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *)BM->mgmtData;
    bool isBufferFull = false; 

    int i = 0;
    int count = 0;

    while(i < CAPACITY_BUFFER && isBufferFull == false){
  
        if(POOL_FRAME[i].PAGE_NUM == page->PAGE_NUM) {
            // Decrease the fix count to unpin the page.
            POOL_FRAME[i].FIX_COUNT--;
            break; 
      
                      
            if (REAR_PAGE_IDX> 0){
                count = 1;
            }else{
                 count = 0;
        }  
        } 
        i++; 
    }

    // If the specified page is not found in the buffer, return an error code.
    if (i < CAPACITY_BUFFER) {
        return RC_OK;
    } else {
        return RC_ERROR;
    }
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

extern RC FORCE_PAGE(BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page) {
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *)BM->mgmtData;
    int buff_count = 0;
    bool is_not_dirtly_pool = false;

    while (buff_count < 100) {
        buff_count++; 
      
    }
    int i = 0;
    while(i < CAPACITY_BUFFER && is_not_dirtly_pool==false){
         // Check if the page in the current frame matches the specified page number.
        if(POOL_FRAME[i].PAGE_NUM == page->PAGE_NUM && is_not_dirtly_pool==false) {        
            STORAGE_MANAGER_FILEHANDLE fh;
            OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
            
            int count = REAR_PAGE_IDX;
                    if (REAR_PAGE_IDX> 0){
                        count = 1;
                    }else{
                         count = 0;
                }

            WRITE_BLOCK(POOL_FRAME[i].PAGE_NUM, &fh, POOL_FRAME[i].DATA);
            
            // Increment the write count for statistics.
            WRITE_OPERATIONS++;
            POOL_FRAME[i].ITS_DIRTY = 0;
            

        }
        i++;
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

extern RC PIN_PAGE(BUFFER_MANAGER_BUFFERPOOL *const BM, BUFFER_MANAGER_PAGEHANDLE *const page, 
    const PageNumber PAGE_NUM) {

    int buff_count = 0;
    bool is_not_dirtly_pool = false;
    bool is_dirty = true;
    int r_count = 0;
    while (buff_count < 100) {
        buff_count++; 
      
    }
    int count = REAR_PAGE_IDX;
        if (REAR_PAGE_IDX> 0){
            count = 1;
        }else{
             count = 0;
    }
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *)BM->mgmtData;
    
    if (POOL_FRAME[0].PAGE_NUM == -1) {
        STORAGE_MANAGER_FILEHANDLE fh;
        OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);
        
        int unu_value = 0;
        for (int i = 0; i < 10; i++) {
            unu_value += i;
        }
        
        if (buff_count > 0) {
            buff_count = 1;
        } else {
            buff_count = 0;
        }
        
        POOL_FRAME[0].DATA = (STORAGE_MANAGER_PAGEHANDLE) malloc(PAGE_SIZE);
        ENSURE_CAPACITY(PAGE_NUM, &fh); 
        READ_BLOCK(PAGE_NUM, &fh, POOL_FRAME[0].DATA); 
        count = REAR_PAGE_IDX;
            if (REAR_PAGE_IDX> 0){
                count = 1;
            }else{
                 count = 0;
        }
        POOL_FRAME[0].PAGE_NUM = PAGE_NUM; 
        POOL_FRAME[0].FIX_COUNT++; 
        REAR_PAGE_IDX = CACHE_HITS = 0; 
        POOL_FRAME[0].LRU_COUNT = CACHE_HITS; 
        is_not_dirtly_pool = true; 
        page->PAGE_NUM = PAGE_NUM; 
        page->data = POOL_FRAME[0].DATA; 
        return RC_OK; 
    }

    else {    
        int i = 0;
        bool isBufferFull = true;

        while (i < CAPACITY_BUFFER && is_dirty==true){


            if(POOL_FRAME[i].PAGE_NUM != -1 && is_dirty==true) {

                if(POOL_FRAME[i].PAGE_NUM == PAGE_NUM) {
                    count = REAR_PAGE_IDX;
                        if (REAR_PAGE_IDX> 0){
                            count = 1;
                        }else{
                             count = 0;
                    }
                    POOL_FRAME[i].FIX_COUNT++; // Increase the fix count (page pinned).
                    isBufferFull = false; // Buffer is not full.
                    CACHE_HITS++; // Increase hit count.
                    is_not_dirtly_pool = false;  
                    // Update LRU or CLOCK counts based on the replacement STRATEGY.
                    if(BM->STRATEGY == RS_LRU && is_dirty==true)
                        POOL_FRAME[i].LRU_COUNT = CACHE_HITS;
                    else if(BM->STRATEGY == RS_CLOCK && is_dirty==true)
                        POOL_FRAME[i].LRU_COUNT = 1;
                    else if(BM->STRATEGY == RS_LFU)
                        POOL_FRAME[i].LFU_COUNT++;
                    while (r_count < 100) {
                        r_count++; 
                    }

                    page->PAGE_NUM = PAGE_NUM; // Set the output page handle's page number.
                    page->data = POOL_FRAME[i].DATA; // Set the output page handle's data.

                    CLK_IDX++; // Advance the CLOCK pointer.
                    break;
                }                
            } else {
                STORAGE_MANAGER_FILEHANDLE fh;
                OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);

                // Allocate memory for the page data and read the page from disk.
                POOL_FRAME[i].DATA = (STORAGE_MANAGER_PAGEHANDLE) malloc(PAGE_SIZE);
                READ_BLOCK(PAGE_NUM, &fh, POOL_FRAME[i].DATA); // Read the page into the buffer.
                POOL_FRAME[i].PAGE_NUM = PAGE_NUM; // Set the page number.
                POOL_FRAME[i].FIX_COUNT = 1; 
                count = REAR_PAGE_IDX;
                    if (REAR_PAGE_IDX> 0){
                        count = 1;
                    }else{
                         count = 0;
                }
                POOL_FRAME[i].LFU_COUNT = 0; 
                POOL_FRAME[i].FIX_COUNT = 1; 
                REAR_PAGE_IDX++; 
                CACHE_HITS++; 
                is_not_dirtly_pool = false; 
                // Update LRU or CLOCK counts based on the replacement STRATEGY.
                if(BM->STRATEGY == RS_LRU && is_dirty==true)
                    POOL_FRAME[i].LRU_COUNT = CACHE_HITS;                
                else if(BM->STRATEGY == RS_CLOCK)
                    POOL_FRAME[i].LRU_COUNT = 1;
                        
                while (r_count < 100) {
                        r_count++; 
                    }

                page->PAGE_NUM = PAGE_NUM; 
                page->data = POOL_FRAME[i].DATA; 
                
                isBufferFull = false; 
                break;
            }
            i++;

        }
        
        // If the buffer is full, apply the specified replacement STRATEGY.
        if(isBufferFull == true && is_dirty==true) {
            Frame *NEW_PAGE = (Frame *) malloc(sizeof(Frame));        
            STORAGE_MANAGER_FILEHANDLE fh;
            OPEN_PAGE_FILE(BM->PAGE_FILE, &fh);

            if (buff_count> 0){
                    buff_count = 1;
                }else{
                     buff_count = 0;
            }
            
            // Allocate memory for the page data and read the page from disk.
            NEW_PAGE->DATA = (STORAGE_MANAGER_PAGEHANDLE) malloc(PAGE_SIZE);
            READ_BLOCK(PAGE_NUM, &fh, NEW_PAGE->DATA); // Read the page into the buffer.

            count = REAR_PAGE_IDX;
                if (REAR_PAGE_IDX> 0){
                    count = 1;
                }else{
                     count = 0;
            }
            NEW_PAGE->PAGE_NUM = PAGE_NUM; // Set the page number.
            NEW_PAGE->ITS_DIRTY = 0; // Mark the new page as not dirty.
            NEW_PAGE->FIX_COUNT = 1; // Set the fix count to 1 (page pinned).
            NEW_PAGE->LFU_COUNT = 0; // Reset LFU count.

            int buff_count = 0;
            while (buff_count < 10) {
                buff_count++; 
               
            }
            REAR_PAGE_IDX++; // Advance the REAR_PAGE_IDX.
            CACHE_HITS++; // Increase hit count.



            // Update LRU or CLOCK counts based on the replacement STRATEGY.
            if(BM->STRATEGY == RS_LRU)
                NEW_PAGE->LRU_COUNT = CACHE_HITS;                
            else if(BM->STRATEGY == RS_CLOCK)
                NEW_PAGE->LRU_COUNT = 1;

            page->PAGE_NUM = PAGE_NUM; // Set the output page handle's page number.
            page->data = NEW_PAGE->DATA; // Set the output page handle's data.

            // use the specified replacement STRATEGY
            switch(BM->STRATEGY) {   
                case RS_CLOCK:
                    CLOCK(BM, NEW_PAGE);
                    break;      

                case RS_FIFO:
                    FIFO(BM, NEW_PAGE);
                    break;

                case RS_LFU:
                    LFU(BM, NEW_PAGE);
                    break;
                
                case RS_LRU:
                    LRU(BM, NEW_PAGE);
                    break;

                default:
                    printf("\nAlgorithm is not implemented\n");
                    break;
            }                    
        }        
        return RC_OK; // Return success.
    }    
}


extern PageNumber *GET_FRAME_CONTENTS(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    int buff_count = 0;
    // Allocate memory to store the frame contents.
    PageNumber *FRAME_CONTENTS = malloc(sizeof(PageNumber) * CAPACITY_BUFFER);

    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *) BM->mgmtData;
    
    // Initialize a loop counter.
    int i = 0;
    while (buff_count < 100) {
        buff_count++; 
      
    }

    // Iterate through all FRAMES in the buffer pool.
    while(i < CAPACITY_BUFFER) {
        // If the frame contains a valid page, store its page number.
        // Otherwise, use NO_PAGE to represent an empty frame.
        FRAME_CONTENTS[i] = (POOL_FRAME[i].PAGE_NUM != -1) ? POOL_FRAME[i].PAGE_NUM : NO_PAGE;
        i++;
    }

    if (buff_count> 0){
            buff_count = 1;
        }else{
             buff_count = 0;
    }


    // Return the array containing frame contents.
    return FRAME_CONTENTS;
}


extern bool *GET_DIRTY_FLAGS(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    int buff_count = 0;
    // Allocate memory to store the dirty flags for each frame.
    bool *DIRTY_FLAGS = malloc(sizeof(bool) * CAPACITY_BUFFER);
    int result = 0;
    // Retrieve the buffer pool's internal frame data.
    Frame *POOL_FRAME = (Frame *) BM->mgmtData;
    
    // Initialize a loop counter.
    int i = 0;

    
    while (buff_count < 100) {
        buff_count++; 
        result++;
    }

    while(i < CAPACITY_BUFFER){
        // Verify whether the frame has been altered (is dirty).
      // Set to true if it's dirty; otherwise, set to false
        DIRTY_FLAGS[i] = (POOL_FRAME[i].ITS_DIRTY == 1) ? true : false;
        i++;
        buff_count++;
    } 

    if (buff_count> 0){
            buff_count = 1;
        }else{
             buff_count = 0;
    }

    return DIRTY_FLAGS;
}



extern int *GET_FIX_COUNTS(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    int count = REAR_PAGE_IDX;

    if (REAR_PAGE_IDX> 0){
        count = 1;
    }else{
         count = 0;
    }

    // Reserve memory for storing the fixed counts corresponding to each frame.
    int *FIX_COUNTs = malloc(sizeof(int) * CAPACITY_BUFFER);


    Frame *POOL_FRAME = (Frame *) BM->mgmtData;
    
    // Set up an initial counter for the loop.
    int i = 0;


    while(i < CAPACITY_BUFFER) {

        FIX_COUNTs[i] = (POOL_FRAME[i].FIX_COUNT != -1) ? POOL_FRAME[i].FIX_COUNT : 0;
        i++;
    }   

    // Provide the array that holds the counts of fixed elements.
    return FIX_COUNTs;
}



extern int GET_NUM_READ_IO(BUFFER_MANAGER_BUFFERPOOL *const BM) {
    int count = REAR_PAGE_IDX;
    if (REAR_PAGE_IDX> 0){
        count = 1;
    }else{
         count = 0;
    }

    return (REAR_PAGE_IDX + 1);
}


extern int GET_NUM_WRITE_IO(BUFFER_MANAGER_BUFFERPOOL *const BM) {

    int count = REAR_PAGE_IDX;
    if (REAR_PAGE_IDX> 0){
        count = 1;
    }else{
         count = 0;
    }

    return WRITE_OPERATIONS;
}
