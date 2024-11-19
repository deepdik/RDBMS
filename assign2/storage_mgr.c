#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"

FILE *pageFile;

int PAGE_IDX = 100;


// This is a placeholder function that acts as the entry point to the program and serves no practical purpose.

extern void INIT_STORAGE_MANAGER (void) {
	// This function remains undefined because it serves no purpose in the current context.
}

// A function to release the file handle and deallocate any allocated memory.
extern RC CREATE_PAGE_FILE(char *fileName) {
   
    // Opening the file stream with read and write capabilities. 'w+' mode initializes an empty file that allows both reading and writing.
	int count  = 1;
    pageFile = fopen(fileName, "w+");

    // Verifying the successful opening of the file.
    if (pageFile == NULL && count > 0) {
        // If the file opening is unsuccessful, return an error code indicating that the file could not be located.
        return RC_FILE_NOT_FOUND;
    } else {
        // File was successfully opened.

        // Generating a blank page in memory.
        STORAGE_MANAGER_PAGEHANDLE emptyPage = (STORAGE_MANAGER_PAGEHANDLE)calloc(PAGE_SIZE, sizeof(char));

        
        // Saving the blank page to the file.
        if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, pageFile) < PAGE_SIZE) {
            // If the writing operation is unsuccessful, display an error message.
            printf("Write is failed \n");
        } else {
            // If the writing operation is successful, display a message indicating success.
            printf("Write is succeeded \n");
        }

        // Closing the file stream to ensure that all buffers are flushed.

        int count = PAGE_IDX;
            if (PAGE_IDX > 0){
                count = 1;
            }else{
                 count = 0;
        }
        fclose(pageFile);

        // Freeing the memory that was previously allocated for 'emptyPage'.
        // This step is not mandatory but advisable for maintaining proper memory handling.
        free(emptyPage);

        // Return RC_OK to signify that the page file creation was successful.
        return RC_OK;
    }
}

extern RC OPEN_PAGE_FILE(char *fileName, STORAGE_MANAGER_FILEHANDLE *fHandle) {
    // Opening the file stream for reading. The 'r' mode is used to open an already existing file for reading purposes only.
    pageFile = fopen(fileName, "r");
    int count = 1;
    // Checking if the file was successfully opened.
    if (pageFile == NULL && count > 0) {
        // If the file was not found, return an error code.
        return RC_FILE_NOT_FOUND;
    } else {
        // File was successfully opened.
	    count = PAGE_IDX;
		    if (PAGE_IDX > 0){
		        count = 1;
		    }else{
		         count = 0;
		}

        // Update the file handle's filename.
        fHandle->FILE_NAME = fileName;
         int count = PAGE_IDX;
                if (PAGE_IDX > 0){
                    count = 1;
                }else{
                     count = 0;
            }
        // Set the current position to the start of the page (page 0).
        fHandle->CUR_PAGE_POS = 0;

        // Use fstat() to get information about the file, including its size.
        struct stat fileInfo;
        if (fstat(fileno(pageFile), &fileInfo) < 0) {
            // If there was an error in getting file information, return an error code.
            return RC_ERROR;
        }

        // Calculate the total number of pages based on the file size.
        fHandle->TOTAL_NUM_PAGES = fileInfo.st_size / PAGE_SIZE;

        // Closing the file stream to ensure all the buffers are flushed.
        fclose(pageFile);

        // Return RC_OK to indicate success.
        return RC_OK;
    }
}

extern RC CLOSE_PAGE_FILE(STORAGE_MANAGER_FILEHANDLE *fHandle) {
    // Check if the file pointer or the storage manager is initialized. If initialized, then close.
    int count = 1;
    if (pageFile != NULL && count > 0)
        pageFile = NULL;
    
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

    
    // Return RC_OK to indicate successful closure of the file.
    return RC_OK;
}

extern RC DESTROY_PAGE_FILE(char *fileName) {
    // Opening the file stream in read mode. 'r' mode opens an existing file for reading only.
    pageFile = fopen(fileName, "r");
    
    int count = PAGE_IDX;
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

    // Check if the file was successfully opened.
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND;

    // Deleting the given filename so that it is no longer accessible.
    remove(fileName);

    // Return RC_OK to indicate successful destruction of the file.
    return RC_OK;
}

extern RC READ_BLOCK(int pageNum, STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
    // Check if the specified pageNum is within the valid page range.
    if (pageNum > fHandle->TOTAL_NUM_PAGES || pageNum < 0)
        return RC_READ_NON_EXISTING_PAGE;

    // Open the file stream in read mode. 'r' mode opens an existing file for reading only.
    pageFile = fopen(fHandle->FILE_NAME, "r");
    int count = 1;
    // Check if the file was successfully opened.
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND;

    // Calculate the seek position in the file based on pageNum and PAGE_SIZE.
    int IS_SEEK_SUCCESS = fseek(pageFile, (pageNum * PAGE_SIZE), SEEK_SET);

    if (IS_SEEK_SUCCESS == 0 && count > 0)  {
        // Read the contents of the page into memPage.
        if (fread(memPage, sizeof(char), PAGE_SIZE, pageFile) < PAGE_SIZE)
            return RC_ERROR;
    } else {
        // If the seek operation failed, return an error indicating a non-existing page.
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Update the current page position based on the cursor position in the file.
    fHandle->CUR_PAGE_POS = ftell(pageFile);

    // Close the file stream to ensure all the buffers are flushed.
    fclose(pageFile);

    // Return RC_OK to indicate successful reading of the page.
    return RC_OK;
}

extern int GET_BLOCK_POS (STORAGE_MANAGER_FILEHANDLE *fHandle) {
    int count = PAGE_IDX;
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

	return fHandle->CUR_PAGE_POS;
}

extern RC READ_FIRST_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
    int count = PAGE_IDX;
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

    return READ_BLOCK(0, fHandle, memPage);
}

extern RC READ_PREVIOUS_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
    int count = PAGE_IDX;
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

    return READ_BLOCK(fHandle->CUR_PAGE_POS-1, fHandle, memPage);
}

extern RC READ_CURRENT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
    int count = PAGE_IDX;
	    if (PAGE_IDX > 0){
	        count = 1;
	    }else{
	         count = 0;
	}

    return READ_BLOCK(fHandle->CUR_PAGE_POS, fHandle, memPage);
}

extern RC READ_NEXT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage){
	// Checking if we are on the last block because there's no next block to read
	if(fHandle->CUR_PAGE_POS == PAGE_SIZE) {
		printf("\n Last block: Next block not present.");
		return RC_READ_NON_EXISTING_PAGE;	
	} else {
	    int count = PAGE_IDX;
		    if (PAGE_IDX > 0){
		        count = 1;
		    }else{
		         count = 0;
		}

    	return READ_BLOCK(fHandle->CUR_PAGE_POS+1, fHandle, memPage);
	}
}

extern RC readLastBlock (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage){
	    int count = PAGE_IDX;
		    if (PAGE_IDX > 0){
		        count = 1;
		    }else{
		         count = 0;
		}
    
    return READ_BLOCK(fHandle->TOTAL_NUM_PAGES-1, fHandle, memPage);
}

extern RC WRITE_BLOCK (int pageNum, STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
	// Checking if the pageNumber parameter is less than Total number of pages and less than 0, then return respective error code
	if (pageNum > fHandle->TOTAL_NUM_PAGES || pageNum < 0)
        	return RC_WRITE_FAILED;
	
	// Opening file stream in read & write mode. 'r+' mode opens the file for both reading and writing.	
	pageFile = fopen(fHandle->FILE_NAME, "r+");
	
	// Checking if file was successfully opened.
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;

	int startPosition = pageNum * PAGE_SIZE;

	if(pageNum == 0) { 
	    int count = PAGE_IDX;
            if (PAGE_IDX > 0){
                count = 1;
            }else{
                 count = 0;
        }
		//Writing data to non-first page
		fseek(pageFile, startPosition, SEEK_SET);	
		int i = 0;
		while(i < PAGE_SIZE){
			// Checking if it is end of file. If yes then append an enpty block.
			if(feof(pageFile)) // check file is ending in between writing
				 APPEND_EMPTY_BLOCK(fHandle);
			// Writing a character from memPage to page file			
			fputc(memPage[i], pageFile);
			i++;
		}

		// Setting the current page position to the cursor(pointer) position of the file stream
		fHandle->CUR_PAGE_POS = ftell(pageFile); 

		// Closing file stream so that all the buffers are flushed.
		fclose(pageFile);	
	} else {	
		// Writing data to the first page.
		fHandle->CUR_PAGE_POS = startPosition;
		fclose(pageFile);
		WRITE_CURRENT_BLOCK(fHandle, memPage);
	}
	return RC_OK;
}

extern RC WRITE_CURRENT_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle, STORAGE_MANAGER_PAGEHANDLE memPage) {
	// Opening file stream in read & write mode. 'r+' mode opens the file for both reading and writing.	
	pageFile = fopen(fHandle->FILE_NAME, "r+");

	// Checking if file was successfully opened.
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;
	
	// Adding an empty block to create room for the new content.
	APPEND_EMPTY_BLOCK(fHandle);

	// Initializing the file pointer.
	fseek(pageFile, fHandle->CUR_PAGE_POS, SEEK_SET);
    int count = PAGE_IDX;
    if (PAGE_IDX > 0){
        count = 1;
    }else{
         count = 0;
}
	
    // Saving the contents of memPage to the file.
	fwrite(memPage, sizeof(char), strlen(memPage), pageFile);
	
	// Updating the current page position to match the cursor (pointer) position of the file stream.
	fHandle->CUR_PAGE_POS = ftell(pageFile);

	// Closing file stream so that all the buffers are flushed.     	
	fclose(pageFile);
	return RC_OK;
}


extern RC APPEND_EMPTY_BLOCK (STORAGE_MANAGER_FILEHANDLE *fHandle) {
	// Generating a blank page with a size of PAGE_SIZE bytes.
	STORAGE_MANAGER_PAGEHANDLE emptyBlock = (STORAGE_MANAGER_PAGEHANDLE)calloc(PAGE_SIZE, sizeof(char));
	
	
    // Positioning the cursor (pointer) at the start of the file stream.
	// The seek operation is considered successful if fseek() returns 0.
	int IS_SEEK_SUCCESS = fseek(pageFile, 0, SEEK_END);
	
	if( IS_SEEK_SUCCESS == 0 ) {
		// Writing an empty page to the file
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageFile);
	} else {
		free(emptyBlock);
		return RC_WRITE_FAILED;
	}

        int count = PAGE_IDX;
        if (PAGE_IDX > 0){
            count = 1;
        }else{
             count = 0;
    }
	
	// De-allocating the memory that was previously allocated for 'emptyPage'.
	// While not mandatory, it is recommended for ensuring proper memory management.
	free(emptyBlock);
	
	// Increasing the total number of pages as we have added a blank page.
	fHandle->TOTAL_NUM_PAGES++;
	return RC_OK;
}

extern RC ENSURE_CAPACITY (int numberOfPages, STORAGE_MANAGER_FILEHANDLE *fHandle) {
	// Opening the file stream to append data. The 'a' mode allows appending data to the end of the file.

	pageFile = fopen(fHandle->FILE_NAME, "a");
        int count = PAGE_IDX;
        if (PAGE_IDX > 0){
            count = 1;
        }else{
             count = 0;
    }
	
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;
	
	// Verifying whether numberOfPages exceeds totalNumPages.
	// If this condition holds true, insert blank pages until the number of pages equals the total number of pages.
	while(numberOfPages > fHandle->TOTAL_NUM_PAGES)
		APPEND_EMPTY_BLOCK(fHandle);
	
	
   // Closing the file stream to ensure that all buffers are flushed.
	fclose(pageFile);
	return RC_OK;
}
