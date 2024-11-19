
#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void initStorageManager(void) {
    // we left this function undefined as of no use.
}


// Create a new page file fileName. The initial file size should be one page.
//  This method should fill this single page with ’\0’ bytes.
// Create a new page file with the specified file name (fName)
RC createPageFile(char *fName) {
    FILE *newFile = fopen(fName, "w+"); // Open the file in binary write mode

    if (newFile == NULL)
        return RC_WRITE_FAILED; // Return an error code if the file cannot be opened

    // Allocate memory for a single page filled with '\0' bytes
    SM_PageHandle newEmptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    if (newEmptyPage == NULL) {
        fclose(newFile); // Close the file if memory allocation fails
        return RC_WRITE_FAILED;
    }

    // Write the empty page to the file
    if (fwrite(newEmptyPage, sizeof(char), PAGE_SIZE, newFile) < PAGE_SIZE) {
        free(newEmptyPage); // Free allocated memory
        fclose(newFile);    // Close the file if the write operation fails
        return RC_WRITE_FAILED;
    }

    // Update metadata in the file handle
    fclose(newFile); 
    free(newEmptyPage);
    return RC_OK;
}




// Open the file with the given fileName in binary read/write mode
// – Opens an existing page file. Should return RC FILE NOT FOUND if the file does not exist.
// – The second parameter is an existing file handle.
// – If opening the file is successful, then the fields of this file handle should be initialized with the information
// about the opened file. For instance, you would have to read the total number of pages that are stored
// in the file from disk.
RC openPageFile(char *fName, SM_FileHandle *fileHandle) {
    FILE *file = fopen(fName, "rb+");

    if (file == NULL) {
        // Return FILE NOT FOUND if the specified file doesn't exist
        return RC_FILE_NOT_FOUND; 
    }

    // Calculate the total number of pages in the file
    // Move the file position to the end
    fseek(file, 0, SEEK_END); 

    fileHandle->totalNumPages = ftell(file) / PAGE_SIZE;
    printf("Debug: total number of pages - %d \n", fileHandle->totalNumPages);

    // Reset the file position to the beginning
    fseek(file, 0, SEEK_SET);

    // Initialize other fields of fileHandle
    fileHandle->fileName = fName;
    fileHandle->curPagePos = 0;
    fileHandle->mgmtInfo = file;
    return RC_OK;
}



// – Close an open page file or destroy (delete) a page file.
RC closePageFile(SM_FileHandle *fileHandle) {
    // Check if the file handle is already closed or uninitialized
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    
    FILE *file = (FILE *)fileHandle->mgmtInfo;
    fclose(file); // Close the file
    fileHandle->mgmtInfo = NULL; // Set the management info to NULL to indicate it's closed

    return RC_OK;
}


extern RC destroyPageFile(char *targetFileName) {
    FILE *fileStream;
    
    // Open the file stream in read mode ('r') to check if the file exists.
    fileStream = fopen(targetFileName, "r");
    
    if (fileStream == NULL)
        return RC_FILE_NOT_FOUND; // Return FILE NOT FOUND if the file doesn't exist
    
    // Close the file stream
    fclose(fileStream);
    
    // Delete the given file to make it inaccessible.
    if (remove(targetFileName) != 0)
        return RC_FILE_NOT_FOUND; // Failed to delete the file
    
    return RC_OK; // File successfully destroyed
}



/**
 * @brief Reads a specified block (page) from the file associated with the given file handle.
 *
 * This function reads the content of the specified page into the provided memory page buffer.
 * The file handle's current page position is updated accordingly.
 *
 * @param pageNumber Page number to be read.
 * @param fileHandle Pointer to the file handle structure.
 * @param memPage Pointer to the memory page where the data will be stored.
 *
 * @return
 *   - RC_OK: Successful read operation.
 *   - RC_READ_NON_EXISTING_PAGE: Invalid handle, page number, or failed read operation.
 */
RC readBlock(int pageNumber, SM_FileHandle *fileHandle, SM_PageHandle memPage) {
    
    // Check for a valid file handle, page number, and page buffer
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL || pageNumber < 0 || pageNumber >= fileHandle->totalNumPages || memPage == NULL)
        return RC_READ_NON_EXISTING_PAGE; // Invalid handle, page number, or page buffer

    // Retrieve the FILE pointer from the management information
    FILE *file = (FILE *)fileHandle->mgmtInfo;

    // Calculate the file position to read from
    long position = (long)pageNumber * PAGE_SIZE;

    // Seek to the desired file position
    if (fseek(file, position, SEEK_SET) != 0)
        return RC_READ_NON_EXISTING_PAGE; // Failed to move to the desired position

    // Read a page of data into memPage
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, file);

    // Check for a partial or failed read
    if (bytesRead != PAGE_SIZE)
        return RC_READ_NON_EXISTING_PAGE;

    // Update the current page position in the file handle
    fileHandle->curPagePos = pageNumber;

    return RC_OK; // Successful read operation
}

/**
 * @brief Retrieves the current block/page position in the file.
 *
 * This function returns the current block/page position associated with the provided file handle.
 * If the file handle is invalid or uninitialized, it returns an appropriate error code.
 *
 * @param fileHandle Pointer to the file handle structure.
 *
 * @return
 *   - The current block/page position if successful.
 *   - RC_FILE_HANDLE_NOT_INIT: Invalid or uninitialized file handle.
 */
int getBlockPos(SM_FileHandle *fileHandle) {
    
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT; 
    }

    // Return the current block/page position
    return fileHandle->curPagePos;
}

// – The method reads the block at position pageNum from a file and stores its content in the memory pointed
// to by the memoryPage page handle.
// – If the file has less than pageNum pages, the method should return RC READ NON EXISTING PAGE.
// Function to the read first page in a file

RC readFirstBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage) {
    // Use readBlock to read the first block (block 0)
    return readBlock(0, fileHandle, memoryPage);
}


/**
 * @brief Reads the content of the previous block in the file and stores it in the memory page.
 *
 * This function utilizes the existing readBlock() function, passing the page number as
 * the current page position minus one.
 *
 * @param fileHandle Pointer to the file handle structure.
 * @param memoryPage Pointer to the memory page where the data will be stored.
 *
 * @return
 *   - RC_OK: Successful read operation.
 *   - RC_READ_NON_EXISTING_PAGE: Invalid handle, page number, or failed read operation.
 */
RC readPreviousBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage) {
    return readBlock(fileHandle->curPagePos - 1, fileHandle, memoryPage);
}

/**
 * @brief Reads the content of the current block in the file and stores it in the memory page.
 *
 * This function utilizes the existing readBlock() function, passing the page number as
 * the current page position.
 *
 * @param fileHandle Pointer to the file handle structure.
 * @param memoryPage Pointer to the memory page where the data will be stored.
 *
 * @return
 *   - RC_OK: Successful read operation.
 *   - RC_READ_NON_EXISTING_PAGE: Invalid handle, page number, or failed read operation.
 */
RC readCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage) {
    return readBlock(fileHandle->curPagePos, fileHandle, memoryPage);
}

/**
 * @brief Reads the content of the next block in the file and stores it in the memory page.
 *
 * This function utilizes the existing readBlock() function, passing the page number as
 * the current page position plus one.
 *
 * @param fileHandle Pointer to the file handle structure.
 * @param memoryPage Pointer to the memory page where the data will be stored.
 *
 * @return
 *   - RC_OK: Successful read operation.
 *   - RC_READ_NON_EXISTING_PAGE: Invalid handle, page number, or failed read operation.
 */
RC readNextBlock(SM_FileHandle* fileHandle, SM_PageHandle memoryPage) {
    return readBlock(fileHandle->curPagePos+1, fileHandle, memoryPage);
}

/**
 * @brief Reads the content of the last block in the file and stores it in the memory page.
 *
 * This function utilizes the existing readBlock() function, passing the page number as
 * the total number of pages minus one.
 *
 * @param fileHandle Pointer to the file handle structure.
 * @param memoryPage Pointer to the memory page where the data will be stored.
 *
 * @return
 *   - RC_OK: Successful read operation.
 *   - RC_READ_NON_EXISTING_PAGE: Invalid handle, page number, or failed read operation.
 */
RC readLastBlock(SM_FileHandle* fileHandle, SM_PageHandle memoryPage) {
    return readBlock(fileHandle->totalNumPages - 1, fileHandle, memoryPage);
}


 
RC writeBlock(int targetPageNum, SM_FileHandle *fileHandle, SM_PageHandle sourceMemPage) {
    
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Check if the given targetPageNum is within the valid range
    if (targetPageNum < 0 || targetPageNum >= fileHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Calculate the byte offset to the desired page
    long byteOffset = (long)targetPageNum * PAGE_SIZE;

    // Move the file pointer to the calculated byte offset
    if (fseek(fileHandle->mgmtInfo, byteOffset, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }

    // Write the content of sourceMemPage to the file
    size_t bytesWritten = fwrite(sourceMemPage, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo);

    // Check if the write operation was successful
    if (bytesWritten != PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Update the current page position in the file handle
    fileHandle->curPagePos = targetPageNum;

    return RC_OK;
}



/**
 * @brief Writes the content of the current page to the file.
 *
 * This function writes the content of the current page in the file associated
 * with the given file handle. The file handle's current page position is updated.
 *
 * @param fileHandle Pointer to the file handle structure.
 * @param Page Pointer to the memory page containing the data to be written.
 *
 * @return
 *   - RC_OK: Successful operation.
 *   - RC_FILE_HANDLE_NOT_INIT: File handle not initialized or file not open for writing.
 *   - RC_WRITE_FAILED: Failed to move file pointer or encountered an issue during the write operation.
 */
RC writeCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle Page) {
   
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Calculate the byte offset to the current page
    long byteOffset = (long)fileHandle->curPagePos * PAGE_SIZE;

    // Move the file pointer to the calculated byte offset
    if (fseek(fileHandle->mgmtInfo, byteOffset, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }

    // Write one page (block) of data from Page to the file
    if (fwrite(Page, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo) != PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Update the current page position
    fileHandle->curPagePos++;

    return RC_OK;
}


//– Increase the number of pages in the file by one. The new last page should be filled with zero bytes.
/**
 * @brief Appends an empty block (page) to the end of the file.
 *
 * This function appends an empty block to the file associated with the given file handle.
 * The file handle's total number of pages and current page position are updated accordingly.
 *
 * @param fileHandle Pointer to the file handle structure.
 *
 * @return
 *   - RC_OK: Successful operation.
 *   - RC_FILE_HANDLE_NOT_INIT: File handle not initialized or file not open for writing.
 *   - RC_WRITE_FAILED: Failed to allocate memory or encountered an issue during the write operation.
 */
RC appendEmptyBlock(SM_FileHandle *fileHandle) {
    // Check if the file is open for writing
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Calculate the byte offset to the end of the file
    long byte_offset = (long)(fileHandle->totalNumPages) * PAGE_SIZE;

    // Move the file pointer to the calculated byte offset
    if (fseek(fileHandle->mgmtInfo, byte_offset, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }

    // Create an empty page filled with '\0' bytes
    SM_PageHandle empty_block = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if (empty_block == NULL) {
        return RC_WRITE_FAILED;
    }

    // Write the empty page to the file
    if (fwrite(empty_block, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo) != PAGE_SIZE) {
        free(empty_block);
        return RC_WRITE_FAILED;
    }

    // Update the total number of pages and current page position
    fileHandle->totalNumPages++;
    fileHandle->curPagePos = fileHandle->totalNumPages - 1;

    // Free allocated memory
    free(empty_block);

    return RC_OK;
}


// – If the file has less than numberOfPages pages then increase the size to numberOfPages.
/**
 * @brief Ensures the file has the required capacity in terms of pages.
 *
 * This function ensures that the file associated with the given file handle
 * has at least the specified number of pages. If the current capacity is sufficient,
 * no action is taken.
 *
 * @param requiredPages The target number of pages needed in the file.
 * @param fileHandle Pointer to the file handle structure.
 *
 * @return
 *   - RC_OK: Successful operation.
 *   - RC_FILE_HANDLE_NOT_INIT: File handle not initialized or file not open for writing.
 *   - RC_WRITE_FAILED: Failed to allocate memory or encountered an issue during the write operation.
 */
RC ensureCapacity(int requiredPages, SM_FileHandle *fileHandle) {
    // Check if the file is open for writing
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Determine the current number of pages in the file
    int currentNumOfPages = fileHandle->totalNumPages;

    if (requiredPages <= currentNumOfPages) {
        return RC_OK;
    }

    int addPages = requiredPages - currentNumOfPages;

    SM_PageHandle paddingPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if (paddingPage == NULL) {
        return RC_WRITE_FAILED; // Failed to allocate memory for an empty page
    }

    if (fseek(fileHandle->mgmtInfo, 0, SEEK_END) != 0) {
        free(paddingPage);
        return RC_WRITE_FAILED;
    }

    for (int pageIndex = 0; pageIndex < addPages; pageIndex++) {
        if (fwrite(paddingPage, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo) != PAGE_SIZE) {
            free(paddingPage);
            return RC_WRITE_FAILED;
        }
        currentNumOfPages++; 
    }

    // Update the total number of pages in the file handle
    fileHandle->totalNumPages = currentNumOfPages;

    free(paddingPage);
    return RC_OK;
}