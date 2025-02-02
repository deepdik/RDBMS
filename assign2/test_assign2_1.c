#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variable to hold the name of the current test.
char *TEST_NAME;

// Verify if the contents of a buffer pool match the EXPECTED content.
// (provided in the format generated by STRINGIFY_BUFFER_POOL_CONTENT)
#define ASSERTEQUALSPOOL(EXPECTED,BM,OUTPUT)			        \
  do {									\
    char *REAL;								\
    char *_exp = (char *) (EXPECTED);                                   \
    REAL = STRINGIFY_BUFFER_POOL_CONTENT(BM);					\
    if (strcmp((_exp),REAL) != 0)					\
      {									\
	printf("[%s-%s-L%i-%s] FAILED: EXPECTED <%s> but was <%s>: %s\n",TESTINFO, _exp, REAL, OUTPUT); \
	free(REAL);							\
	exit(1);							\
      }									\
    printf("[%s-%s-L%i-%s] OK: EXPECTED <%s> and was <%s>: %s\n",TESTINFO, _exp, REAL, OUTPUT); \
    free(REAL);								\
  } while(0)

// test and helper methods
static void TEST_CREATING_AND_READING_DUMMY_PAGES (void);
static void CREATE_DUMMY_PAGES(BUFFER_MANAGER_BUFFERPOOL *BM, int num);
static void CHECK_DUMMY_PAGES(BUFFER_MANAGER_BUFFERPOOL *BM, int num);

static void TEST_READ_PAGE (void);
static void TEST_LFU (void);
static void TEST_FIFO (void);
static void TEST_LRU (void);

// main method
int 
main (void) 
{
  INIT_STORAGE_MANAGER();
  TEST_NAME = "";

  TEST_CREATING_AND_READING_DUMMY_PAGES();
  TEST_READ_PAGE();
  TEST_FIFO();
  TEST_LRU();
  TEST_LFU();
}

// Generate n pages with the text "Page X" and then read them to verify the correctness of the content.

void
TEST_CREATING_AND_READING_DUMMY_PAGES (void)
{
  BUFFER_MANAGER_BUFFERPOOL *BM = MAKE_POOL();
  TEST_NAME = "Creating and Reading Back Dummy Pages";

  CHECK(CREATE_PAGE_FILE("testbuffer.bin"));

  CREATE_DUMMY_PAGES(BM, 22);
  CHECK_DUMMY_PAGES(BM, 20);

  CREATE_DUMMY_PAGES(BM, 10000);
  CHECK_DUMMY_PAGES(BM, 10000);

  CHECK(DESTROY_PAGE_FILE("testbuffer.bin"));

  free(BM);
  TESTDONE();
}


void 
CREATE_DUMMY_PAGES(BUFFER_MANAGER_BUFFERPOOL *BM, int num)
{
  int i;
  BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();

  CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 3, RS_FIFO, NULL));
  
  for (i = 0; i < num; i++)
    {
      CHECK(PIN_PAGE(BM, h, i));
      sprintf(h->data, "%s-%i", "Page", h->PAGE_NUM);
      CHECK(MARK_DIRTY(BM, h));
      CHECK(UNPIN_PAGE(BM,h));
    }

  CHECK(SHUTDOWN_BUFFER_POOL(BM));

  free(h);
}

void 
CHECK_DUMMY_PAGES(BUFFER_MANAGER_BUFFERPOOL *BM, int num)
{
  int i;
  BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();
  char *EXPECTED = malloc(sizeof(char) * 512);

  CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 3, RS_FIFO, NULL));

  for (i = 0; i < num; i++)
    {
      CHECK(PIN_PAGE(BM, h, i));

      sprintf(EXPECTED, "%s-%i", "Page", h->PAGE_NUM);
      ASSERTEQUALSSTRING(EXPECTED, h->data, "reading back dummy page content");

      CHECK(UNPIN_PAGE(BM,h));
    }

  CHECK(SHUTDOWN_BUFFER_POOL(BM));

  free(EXPECTED);
  free(h);
}

void
TEST_READ_PAGE ()
{
  BUFFER_MANAGER_BUFFERPOOL *BM = MAKE_POOL();
  BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();
  TEST_NAME = "Reading a page";

  CHECK(CREATE_PAGE_FILE("testbuffer.bin"));
  CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 3, RS_FIFO, NULL));
  
  CHECK(PIN_PAGE(BM, h, 0));
  CHECK(PIN_PAGE(BM, h, 0));

  CHECK(MARK_DIRTY(BM, h));

  CHECK(UNPIN_PAGE(BM,h));
  CHECK(UNPIN_PAGE(BM,h));

  CHECK(FORCE_PAGE(BM, h));

  CHECK(SHUTDOWN_BUFFER_POOL(BM));
  CHECK(DESTROY_PAGE_FILE("testbuffer.bin"));

  free(BM);
  free(h);

  TESTDONE();
}

void
TEST_FIFO ()
{
  // EXPECTED results
  const char *poolContents[] = { 
    "[0 0],[-1 0],[-1 0]" , 
    "[0 0],[1 0],[-1 0]", 
    "[0 0],[1 0],[2 0]", 
    "[3 0],[1 0],[2 0]", 
    "[3 0],[4 0],[2 0]",
    "[3 0],[4 1],[2 0]",
    "[3 0],[4 1],[5x0]",
    "[6x0],[4 1],[5x0]",
    "[6x0],[4 1],[0x0]",
    "[6x0],[4 0],[0x0]",
    "[6 0],[4 0],[0 0]"
  };
  const int requests[] = {0,1,2,3,4,4,5,6,0};
  const int numLinRequests = 5;
  const int numChangeRequests = 3;

  int i;
  BUFFER_MANAGER_BUFFERPOOL *BM = MAKE_POOL();
  BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();
  TEST_NAME = "Testing FIFO page replacement";

  CHECK(CREATE_PAGE_FILE("testbuffer.bin"));

  CREATE_DUMMY_PAGES(BM, 100);

  CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 3, RS_FIFO, NULL));

  // Sequentially reading several pages with direct unpinning and no alterations.
  for(i = 0; i < numLinRequests; i++)
    {
      PIN_PAGE(BM, h, requests[i]);
      UNPIN_PAGE(BM, h);
      ASSERTEQUALSPOOL(poolContents[i], BM, "check pool content");
    }

  // Pin a single page and test the remaining pages.
  i = numLinRequests;
  PIN_PAGE(BM, h, requests[i]);
  ASSERTEQUALSPOOL(poolContents[i],BM,"pool content after pin page");

  
// Read pages and flag them as modified.
  for(i = numLinRequests + 1; i < numLinRequests + numChangeRequests + 1; i++)
    {
      PIN_PAGE(BM, h, requests[i]);
      MARK_DIRTY(BM, h);
      UNPIN_PAGE(BM, h);
      ASSERTEQUALSPOOL(poolContents[i], BM, "check pool content");
    }

  // Write the buffer pool contents to disk.
  i = numLinRequests + numChangeRequests + 1;
  h->PAGE_NUM = 4;
  UNPIN_PAGE(BM, h);
  ASSERTEQUALSPOOL(poolContents[i],BM,"unpin last page");
  
  i++;
  FORCE_FLUSH_POOL(BM);
  ASSERTEQUALSPOOL(poolContents[i],BM,"pool content after flush");

  
// Verify the count of write input/output operations.

  ASSERTEQUALSINT(3, GET_NUM_WRITE_IO(BM), "check number of write I/Os");
  ASSERTEQUALSINT(8, GET_NUM_READ_IO(BM), "check number of read I/Os");

  CHECK(SHUTDOWN_BUFFER_POOL(BM));
  CHECK(DESTROY_PAGE_FILE("testbuffer.bin"));

  free(BM);
  free(h);
  TESTDONE();
}

void
TEST_LFU(void)
{
    // EXPECTED results
    const char *poolContents[]= {
        
   "[3 0],[-1 0],[-1 0]",
   "[3 0],[7 0],[-1 0]",
   "[3 0],[7 0],[6 0]",
   "[4 0],[7 0],[6 0]",
   "[4 0],[7 0],[6 0]",
   "[4 0],[2 0],[6 0]",
   "[1 0],[2 0],[6 0]",
   "[1 0],[9 0],[6 0]",
   "[2 0],[9 0],[6 0]",
   "[2 0],[8 0],[6 0]"
    };

    const int orderRequests[]= {3,7,6,4,6,2,1,9,2,8};
        
    int i;
    int snapshot = 0;
    BUFFER_MANAGER_BUFFERPOOL *BM = MAKE_POOL();
    BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();
    TEST_NAME = "Testing LFU page replacement";

    CHECK(CREATE_PAGE_FILE("testbuffer.bin"));
    CREATE_DUMMY_PAGES(BM, 100);
    CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 3, RS_LFU, NULL));
    
    for (i=0;i<10;i++)
    {
        PIN_PAGE(BM,h,orderRequests[i]);
	//if(orderRequests[i] == 3)
	//	MARK_DIRTY(BM,h);
        UNPIN_PAGE(BM,h);
        ASSERTEQUALSPOOL(poolContents[snapshot++], BM, "check pool content using pages");
    }
    
    FORCE_FLUSH_POOL(BM);
    // check number of write IOs
    ASSERTEQUALSINT(0, GET_NUM_WRITE_IO(BM), "check number of write I/Os");
    ASSERTEQUALSINT(9, GET_NUM_READ_IO(BM), "check number of read I/Os");
    
    CHECK(SHUTDOWN_BUFFER_POOL(BM));
    CHECK(DESTROY_PAGE_FILE("testbuffer.bin"));
    
    free(BM);
    free(h);
    TESTDONE();
}

// test the LRU page replacement strategy
void
TEST_LRU (void)
{
  // EXPECTED results
  const char *poolContents[] = { 
    // read first five pages and directly unpin them
    "[0 0],[-1 0],[-1 0],[-1 0],[-1 0]" , 
    "[0 0],[1 0],[-1 0],[-1 0],[-1 0]", 
    "[0 0],[1 0],[2 0],[-1 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // use some of the page to create a fixed LRU order without changing pool content
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // check that pages get evicted in LRU order
    "[0 0],[1 0],[2 0],[5 0],[4 0]",
    "[0 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[8 0],[5 0],[6 0]",
    "[7 0],[9 0],[8 0],[5 0],[6 0]"
  };
  const int orderRequests[] = {3,4,0,2,1};
  const int numLRUOrderChange = 5;

  int i;
  int snapshot = 0;
  BUFFER_MANAGER_BUFFERPOOL *BM = MAKE_POOL();
  BUFFER_MANAGER_PAGEHANDLE *h = MAKE_PAGE_HANDLE();
  TEST_NAME = "Testing LRU page replacement";

  CHECK(CREATE_PAGE_FILE("testbuffer.bin"));
  CREATE_DUMMY_PAGES(BM, 100);
  CHECK(INIT_BUFFER_POOL(BM, "testbuffer.bin", 5, RS_LRU, NULL));

 // Sequentially read the initial five pages with direct unpinning and no changes.
  for(i = 0; i < 5; i++)
  {
      PIN_PAGE(BM, h, i);
      UNPIN_PAGE(BM, h);
      ASSERTEQUALSPOOL(poolContents[snapshot], BM, "check pool content reading in pages");
      snapshot++;
  }

// Access pages to alter the Least Recently Used (LRU) order.
  for(i = 0; i < numLRUOrderChange; i++)
  {
      PIN_PAGE(BM, h, orderRequests[i]);
      UNPIN_PAGE(BM, h);
      ASSERTEQUALSPOOL(poolContents[snapshot], BM, "check pool content using pages");
      snapshot++;
  }

// Substitute pages and confirm that the replacement occurs according to the Least Recently Used (LRU) order.
  for(i = 0; i < 5; i++)
  {
      PIN_PAGE(BM, h, 5 + i);
      UNPIN_PAGE(BM, h);
      ASSERTEQUALSPOOL(poolContents[snapshot], BM, "check pool content using pages");
      snapshot++;
  }

// Verify the count of write input/output operations.
  ASSERTEQUALSINT(0, GET_NUM_WRITE_IO(BM), "check number of write I/Os");
  ASSERTEQUALSINT(10, GET_NUM_READ_IO(BM), "check number of read I/Os");

  CHECK(SHUTDOWN_BUFFER_POOL(BM));
  CHECK(DESTROY_PAGE_FILE("testbuffer.bin"));

  free(BM);
  free(h);
  TESTDONE();
}
