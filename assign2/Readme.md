## Contributions

| Team Member                            | Contributions                                       |
|----------------------------------------|-----------------------------------------------------|
| Deepak Kumar (A20547017)               | 1/3                                                 |
| Shubham Bajirao Dhanavade (A20541092)  | 1/3                                                 |
| Siddhant Sarnobat (A20543734)          | 1/3                                                 |

## Aim

The goal of this assignment is to implement a simple buffer manager. The buffer manager manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in assignment 1. The memory pages managed by the buffer manager are called page frames or frames for short. We call the combination of a page file and the page frames storing pages from that file a Buffer Pool .
The buffer manager should be able to handle more than one open buffer pool at the same time. However, there can only be one buffer pool for each page file. Each buffer pool uses one page replacement strategy that is determined when the buffer pool is initialized. You should at least implement two replacement strategies FIFO and LRU. Your solution should implement all the methods defined in the buffer mgr.h header explained below. Make use of existing debugging and memory checking tools. At some point you will have to debug an error. See the main assignment page (Programming Assignment: Organization) for information about debugging. Memory leaks are errors!


### Additional Test Cases

1. **Extended Testing for `test_assign2_2` Module**:
   - In the `test_assign2_2` module, we introduced two new test cases to further validate system functionality:
     - `testCreatingAndReadingDummyPages()`
     - `testReadPage()`

1. **Extended Testing for `test_assign2_1` Module**:
   - In the `test_assign2_1` module, we introduced two new test cases to further validate system functionality:
     - `TEST_LFU()`



### Instructions for Running the Code

1. Compile all project files, including "**test_assign2_1.c**", by running "**make test1**".
2. Execute the command "**make run_test1**" to run the tests specified in "**test_assign2_1.c**".
3. Compile the custom test file "**test_assign2_2.c**" with "**make test2**".
4. Run the tests in "**test_assign2_2.c**" using "**make run_test2**".



### Buffer Pool Functions

These functions facilitate the creation and management of the buffer pool, which caches pages from the page file on disk.

- **initBufferPool(...)**:
  - Initializes a new buffer pool in memory.
  - Parameters:
    - `numPages`: Specifies the size of the buffer (number of page frames).
    - `pageFileName`: Name of the page file whose pages are cached in memory.
    - `strategy`: Determines the page replacement strategy (FIFO, LRU, LFU, CLOCK).
    - `stratData`: Additional parameters passed to the page replacement strategy.

- **shutdownBufferPool(...)**:
  - Shuts down the buffer pool, freeing up all associated resources.
  - Calls `forceFlushPool(...)` to write dirty pages to disk before shutdown.

- **forceFlushPool(...)**:
  - Writes all dirty pages (modified pages) to the disk.

### Page Management Functions

These functions handle loading pages from disk into the buffer pool (pinning), removing pages from the buffer pool (unpinning), marking pages as dirty, and forcing pages to be written to disk.

- **PIN_PAGE(...)**:
  - Pins the specified page, reading it from the page file into the buffer pool.
  - Employs page replacement strategies if the buffer pool is full.

- **UNPIN_PAGE(...)**:
  - Unpins the specified page, decrementing its fix count.

- **makeDirty(...)**:
  - Sets the dirty bit of the specified page.

- **forcePage(...)**:
  - Writes the content of the specified page frame to the page file on disk.

**Statistics Functions**
1. **getFrameContents(...)**  
   - Returns an array of PageNumbers, where the array size equals the buffer size (numPages).
   - Iterates over all page frames in the buffer pool to retrieve the pageNum value of each page frame.
   - Each element of the array corresponds to the page number of the page stored in the respective page frame.

2. **getDirtyFlags(...)**  
   - Returns an array of booleans, with the array size equal to the buffer size (numPages).
   - Iterates over all page frames in the buffer pool to obtain the dirtyBit value of each page frame.
   - Each element of the array is set to TRUE if the page stored in the respective page frame is dirty.

3. **getFixCounts(...)**
   - Returns an array of integers, where the array size equals the buffer size (numPages).
   - Iterates over all page frames in the buffer pool to fetch the fixCount value of each page frame.
   - Each element of the array represents the fixCount of the page stored in the respective page frame.

4. **getNumReadIO(...)**
   - Returns the total count of IO reads performed by the buffer pool, indicating the number of pages read from the disk.
   - This data is tracked and maintained using the rearIndex variable.

5. **getNumWriteIO(...)**
   - Returns the total count of IO writes executed by the buffer pool, reflecting the number of pages written to the disk.
   - The writeCount variable is employed to maintain this information, initialized to 0 during buffer pool initialization, and incremented whenever a page frame is written to disk.

### Page Replacement Algorithm Functions

These functions implement FIFO, LRU, LFU, and CLOCK page replacement algorithms.

Certainly, here's a rewritten version:

1. **FIFO (First In First Out):** FIFO operates by evicting the page that has been in the buffer pool the longest. It adheres to a queue-like structure, where the oldest page is replaced when the buffer reaches its capacity.

2. **LFU (Least Frequently Used):** LFU selects the page with the fewest accesses for eviction. It maintains a counter (refNum) to track the number of times each page has been accessed, opting to replace the page with the lowest count.

3. **LRU (Least Recently Used):** LRU targets the page that has remained untouched for the longest period. It utilizes a counter (hitNum) to record the time since each page was last accessed, preferring to replace the page with the lowest hitNum.

4. **CLOCK:** The CLOCK algorithm manages a circular list of pages alongside a clock hand that indicates the next page for potential replacement. It works by checking if the page pointed to by the clock hand has been accessed since its entry into memory. If not, that page is replaced; otherwise, the clock hand moves to the next page, and the process repeats until a suitable replacement candidate is identified.

