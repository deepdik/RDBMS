#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);
static void testGetBlockPos(void);
static void testAppendEmptyBlock(void);
static void testEnsureCapacity(void);



/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();

  printf("###################### TEST CASE 1 START ############################## \n");
  testCreateOpenClose();
  printf("###################### TEST CASE 1 END ############################## \n\n");


  printf("##################### TEST CASE 2 START ############################## \n");
  testSinglePageContent();
  printf("###################### TEST CASE 2 END ############################## \n\n");

  // self written test cases
  printf("##################### TEST CASE 3 START ############################## \n");
  testGetBlockPos();
  printf("##################### TEST CASE 3 END ############################## \n\n");
  

  printf("#################### TEST CASE 4 START ############################## \n");
  testAppendEmptyBlock();
  printf("##################### TEST CASE 4 END ############################### \n\n");


  printf("###################### TEST CASE 5 START ######################### \n");
  testEnsureCapacity();
  printf("####################### TEST CASE 5 END ########################## \n");

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));
  
  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");
  
  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");
    
  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // destroy new page file
  TEST_CHECK(destroyPageFile (TESTPF));  
  
  TEST_DONE();
}


/* Return the current page position in a file */
void testGetBlockPos(void) {
    SM_FileHandle fh;
    SM_PageHandle ph;

    testName = "test get Block Pos";

    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    if (ph == NULL) {
        printf("Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the file handle before using it
    TEST_CHECK(createPageFile(TESTPF));
    TEST_CHECK(openPageFile(TESTPF, &fh));

    // Perform the test for getBlockPos
    ASSERT_TRUE((getBlockPos(&fh) == 0), "initial page position should be 0");

    // Clean up
    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile(TESTPF));

    free(ph);

    TEST_DONE();
}

void testAppendEmptyBlock(void) {
    SM_FileHandle fh;
    SM_PageHandle ph;

    testName = "test append Empty Block";

    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    if (ph == NULL) {
        printf("Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    TEST_CHECK(createPageFile(TESTPF));
    TEST_CHECK(openPageFile(TESTPF, &fh));

    // Append an empty block
    TEST_CHECK(appendEmptyBlock(&fh));

    // Read the new last block
    TEST_CHECK(readLastBlock(&fh, ph));

    // Verify that the new last block is filled with zero bytes
    for (int i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "new last block should be filled with zero bytes");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile(TESTPF));

    free(ph);

    TEST_DONE();
}

void testEnsureCapacity(void) {
    SM_FileHandle fh;
    SM_PageHandle ph;

    testName = "test ensure Capacity";

    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    if (ph == NULL) {
        printf("Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    TEST_CHECK(createPageFile(TESTPF));
    TEST_CHECK(openPageFile(TESTPF, &fh));

    // Initially, the file should have 1 page
    ASSERT_TRUE((fh.totalNumPages == 1), "initial totalNumPages should be 1");

    // Ensure capacity for 5 pages
    TEST_CHECK(ensureCapacity(5, &fh));

    // Verify that totalNumPages is now 5
    ASSERT_TRUE((fh.totalNumPages == 5), "totalNumPages should be 5 after ensureCapacity");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile(TESTPF));

    free(ph);

    TEST_DONE();
}