#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"

#include <stdio.h>
#include <stdlib.h>

// Below are local functions
static void DISPLAY_STRATEGY (BUFFER_MANAGER_BUFFERPOOL *const BM);

// Below are external functions
void 
SHOW_BUFFER_POOL_CONTENT (BUFFER_MANAGER_BUFFERPOOL *const BM)
{
	PageNumber *PAGE_NUMBER;
	bool *DIRTY_FLAGS;
	int *PIN_COUNTS;
	int PAGE_INDEX;

	PAGE_NUMBER = GET_FRAME_CONTENTS(BM);
	DIRTY_FLAGS = GET_DIRTY_FLAGS(BM);
	PIN_COUNTS = GET_FIX_COUNTS(BM);

	printf("{");
	DISPLAY_STRATEGY(BM);
	printf(" %i}: ", BM->NUM_PAGES);

	for (PAGE_INDEX = 0; PAGE_INDEX < BM->NUM_PAGES; PAGE_INDEX++)
		printf("%s[%i%s%i]", ((PAGE_INDEX == 0) ? "" : ",") , PAGE_NUMBER[PAGE_INDEX], (DIRTY_FLAGS[PAGE_INDEX] ? "x": " "), PIN_COUNTS[PAGE_INDEX]);
	printf("\n");
}

char *STRINGIFY_BUFFER_POOL_CONTENT (BUFFER_MANAGER_BUFFERPOOL *const BM)
{
	PageNumber *PAGE_NUMBER;
	bool *DIRTY_FLAGS;
	int *PIN_COUNTS;
	int PAGE_INDEX;
	char *result;
	int POSITION = 0;

	result = (char *) malloc(256 + (22 * BM->NUM_PAGES));
	PAGE_NUMBER = GET_FRAME_CONTENTS(BM);
	DIRTY_FLAGS = GET_DIRTY_FLAGS(BM);
	PIN_COUNTS = GET_FIX_COUNTS(BM);

	for (PAGE_INDEX = 0; PAGE_INDEX < BM->NUM_PAGES; PAGE_INDEX++)
		POSITION += sprintf(result + POSITION, "%s[%i%s%i]", ((PAGE_INDEX == 0) ? "" : ",") , PAGE_NUMBER[PAGE_INDEX], (DIRTY_FLAGS[PAGE_INDEX] ? "x": " "), PIN_COUNTS[PAGE_INDEX]);

	return result;
}


void SHOW_PAGE_CONTENT (BUFFER_MANAGER_PAGEHANDLE *const PAGE)
{
	int BYTE_INDEX;

	printf("[Page %i]\n", PAGE->PAGE_NUM);

	for (BYTE_INDEX = 1; BYTE_INDEX <= PAGE_SIZE; BYTE_INDEX++)
		printf("%02X%s%s", PAGE->data[BYTE_INDEX], (BYTE_INDEX % 8) ? "" : " ", (BYTE_INDEX % 64) ? "" : "\n");
}

char * STRINGIFY_PAGE_CONTENT  (BUFFER_MANAGER_PAGEHANDLE *const PAGE)
{
	int BYTE_INDEX;
	char *result;
	int POSITION = 0;

	result = (char *) malloc(30 + (2 * PAGE_SIZE) + (PAGE_SIZE % 64) + (PAGE_SIZE % 8));
	POSITION += sprintf(result + POSITION, "[Page %i]\n", PAGE->PAGE_NUM);

	for (BYTE_INDEX = 1; BYTE_INDEX <= PAGE_SIZE; BYTE_INDEX++)
		POSITION += sprintf(result + POSITION, "%02X%s%s", PAGE->data[BYTE_INDEX], (BYTE_INDEX % 8) ? "" : " ", (BYTE_INDEX % 64) ? "" : "\n");

	return result;
}

void DISPLAY_STRATEGY (BUFFER_MANAGER_BUFFERPOOL *const BM)
{
    const char *STRATEGY_NAMES[] = {"FIFO", "LRU", "CLOCK", "LFU", "LRU-K"};
    if (BM->STRATEGY >= 0 && BM->STRATEGY < sizeof(STRATEGY_NAMES) / sizeof(STRATEGY_NAMES[0])) {
        printf("%s", STRATEGY_NAMES[BM->STRATEGY]);
    } else {
        printf("Unknown Strategy (%i)", BM->STRATEGY);
    }
}
    
