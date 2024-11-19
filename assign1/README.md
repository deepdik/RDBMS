# CS525 Advanced Database Organization Assignment 1: Storage Manager

## Contributions

| Team Member                            | Contributions                                       |
|----------------------------------------|-----------------------------------------------------|
| Deepak Kumar (A20547017)               | 1/3                                                 |
| Shubham Bajirao Dhanavade (A20541092)  | 1/3                                                 |
| Siddhant Sarnobat (A20543734)          | 1/3                                                 |

## Project Modules
### C source files
- storage_mgr.c
- dberror.c
- test_assign1_1.c

### Header files
- storage_mgr.h
- dberror.h
- test_helper.h

## Aim
The goal of this assignment is to implement a simple storage manager - a module that is
capable of reading blocks from a file on disk into memory and writing blocks from memory to
a file on disk. The storage manager deals with pages (blocks) of fixed size (PAGE SIZE).
In addition to reading and writing pages from a file, it provides methods for creating, opening,
and closing files. The storage manager has to maintain several types of information for an open file:
The number of total pages in the file, the current page position (for reading and writing),
the file name, and a POSIX file descriptor or FILE pointer.

## Contents

1. **Instructions to run the code**
   1. For executing mandatory test cases:
      - In the terminal, navigate to the assignment directory.
      - Type: `make`
      - Run: `make run_storage_manager`
      - Clean: `make clean`

2. **Description of functions used**

    1. `initStorageManager(void)`
        - **Description**:
          1. This function acts as a placeholder and currently lacks any functionality.
          2. It serves as an entry point to the program but does not perform any tasks in this implementation.

    2. `createPageFile(char *newFileName)`

        - **Description**:
          1. Creates a new page file with the specified `fName`.
          2. Opens the file in binary write mode ("wb") using `fopen()` and stores the file pointer in the `file` variable.
          3. Allocates an empty page of size 4KB for `SM_PageHandle` memory page, filled with null bytes. If memory allocation fails, it returns `RC_WRITE_FAILED`.
          4. If memory allocation is successful, writes the empty page to the file using `fwrite()`. Closes the file, frees memory allocated for the empty page, and returns `RC_OK` if the page file is successfully created.

    3. `openPageFile(char *fName, SM_FileHandle *fileHandle)`

        - **Description**:
          1. Opens an existing page file specified by `fName` in binary read/write mode ("rb+").
          2. Calculates the total number of pages, resets the file position to the beginning, and initializes fields of the file handle (`SM_FileHandle`). Returns an error code (`RC_FILE_NOT_FOUND`) if the file doesn't exist.
          
    4. `closePageFile(SM_FileHandle *fileHandle)`

        - **Description**:
          1. Closes an open page file.
          2. Returns an error code (`RC_FILE_HANDLE_NOT_INIT`) if the file handle is already closed or uninitialized. 

    5. `destroyPageFile(char *targetFileName)`

        - **Description**:
          1. Checks whether the file exists.
          2. Attempts to delete the specified file using the `remove` function. Returns an `RC_FILE_NOT_FOUND` error code if the file is not found or if deletion fails.
          
    6. `readBlock(int pageNumber, SM_FileHandle *fileHandle, SM_PageHandle memPage)`

        - **Description**:
          1. Reads a block (page) from the specified `pageNumber` into a memory page (`memPage`).
          2. Performs error checks to ensure a valid file handle, page number, and the existence of the page. Retrieves the file pointer from the management information (`mgmtInfo`).
          3. Seeks to the desired file position using `fseek()`, reads the page data into `memPage` using `fread()`, and stores the data in the `bytesRead` variable.

    7. `getBlockPos(SM_FileHandle *fileHandle)`

        - **Description**:
          1. Checks for a valid file handle and management information, then gets the current Page Position from `SM_FileHandle` metadata. Retrieves the current block (page) position in the file handle.

    8. `readFirstBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage)`

        - **Description**:
          1. Reads the first block (page 0) in the file by calling `readBlock` with `pageNum` set to 0 and stores its content in `memoryPage`. Directly calls `readBlock()` method and passes `pageNum` as 0.

    9. `readPreviousBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage)`

        - **Description**:
          1. Reads the block preceding the current block in the file by calling `readBlock` with `pageNum` set to `curPagePos – 1` and stores its content in `memoryPage`.

    10. `readCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage)`

        - **Description**:
          1. Reads the content of the current block using `readBlock` with `pageNum` set to `curPagePos` and stores its content in `memoryPage`.

    11. `readNextBlock(SM_FileHandle* fileHandle, SM_PageHandle memoryPage)`

        - **Description**:
          1. Reads the block following the current block in the file by calling `readBlock` with `pageNum` set to `curPagePos + 1` and stores its content in `memoryPage`.

    12. `writeBlock(int targetPageNum, SM_FileHandle *fileHandle, SM_PageHandle sourceMemPage)`

        - **Description**:
          1. Writes the content of the memory page to the specified block. Checks if the file is open for writing, if the file exists, and if the given `targetPageNum` is valid.
          2. Calculates the byte offset to the desired page and moves the file pointer to the calculated byte offset using `fseek()`. Writes the content of `sourceMemPage` to the file using `fwrite()`. Updates the current page position in the file handle. Returns 

    13. `writeCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle Page)`

        - **Description**:
          1. Writes the content of the current page to the file by calling `writeBlock` with `pageNum` set to `curPagePos`.
          2. Follows steps similar to `writeBlock` method by calling `fseek()` and `fwrite()` methods to write one page of data from `Page` to the file. Updates the current page position by incrementing the `curPagePos`.
          
    15. `appendEmptyBlock(SM_FileHandle *fileHandle)`

        - **Description**:
          1. Appends an empty block to the end of the file.
          2. Checks if the file is open for writing; if not, returns `RC_FILE_NOT_FOUND` error code. Calculates the offset to the end of the file by passing `SEEK_END` to `fseek()`.
          4. Creates an empty page filled with '\0' bytes, writes the empty page to the file using `fwrite()`. Updates the total number of pages and the current page position in the file handle. Releases memory allocated for the empty page.

    16. `ensureCapacity(int requiredPages, SM_FileHandle *fileHandle)`

        - **Description**:
          1. Ensures that the file has at least the specified number of pages by appending empty blocks if necessary. Checks if the file is open for writing and determines the current number of pages in the file from `totalNumPages`. Compares `numberOfPages` with `totalNumPages` to check if the file already has enough pages.
          4. If more pages are needed, calculates the number of additional pages required. Appends empty pages to the file by seeking to the end of the file using `SEEK_END` and iterating over the additional pages.
          
3. **Testcases**

    - **Create, Open, and Close Methods**
        - Description: Tests the functionality of creating, opening, and closing a page file.
        - Test Function: `testCreateOpenClose`

    - **Single Page Content**
        - Description: Tests reading, writing, and verifying content of a single page in a newly     created page file.
        - Test Function: `testSinglePageContent`

    - **Get Block Position**
        - Description: Tests the accuracy of obtaining the current block position in a file.
        - Test Function: `testGetBlockPos`

    - **Append Empty Block**
        - Description: Tests the functionality of appending an empty block to the end of a page file.
        - Test Function: `testAppendEmptyBlock`

    - **Ensure Capacity**
        - Description: Tests ensuring the capacity of a file by appending empty blocks if needed.
        - Test Function: `testEnsureCapacity`
