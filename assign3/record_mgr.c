#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "record_mgr.h"


const int MAX_NUM_OF_PAG = 100;
const int ATTR_SIZE = 15;

// Custom data structure designed for facilitating the use of the Record Manager.
typedef struct RecordDataManager
{
	// PageHandle from the Buffer Manager to access Page files
	BM_PageHandle filePageHandle;
	// Buffer Pool from the Buffer Manager	
	BM_BufferPool bufferPool;
	// Identifier for a record
	RID recordIdentifier;
	// Condition for scanning records in the table
	Expr *scanCondition;
	// Total number of tuples in the table
	int totalTuples;
	// Location of the first free page with empty slots in the table
	int firstFreePage;
	// Count of the number of records scanned
	int scannedRecordCount;
} RecordDataManager;



RecordDataManager *recordMgr;

/**
 * @brief Sets the offset (in bytes) from the initial position to the specified attribute of the record.
 *
 * This function calculates the offset in bytes from the initial position of the record's data
 * to the specified attribute based on the schema provided.
 *
 * @param schema Pointer to the schema structure.
 * @param attrNum The attribute number for which the offset is calculated.
 * @param output Pointer to an integer to store the calculated offset.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
RC attrOffset(Schema *schema, int attrNum, int *output)
{
    int i;
    *output = 1;

    // Iterate through all the attributes in the schema
    for (i = 0; i < attrNum; i++)
    {
        // Switch based on the data type of the attribute
        switch (schema->dataTypes[i])
        {
            case DT_STRING:
                // If the attribute is a string, add the type length to the output
                *output = *output + schema->typeLength[i];
                break;
            case DT_INT:
                // If the attribute is an integer, add the size of int to the output
                *output = *output + sizeof(int);
                break;
            case DT_FLOAT:
                // If the attribute is a float, add the size of float to the output
                *output = *output + sizeof(float);
                break;
            case DT_BOOL:
                // If the attribute is a boolean, add the size of bool to the output
                *output = *output + sizeof(bool);
                break;
        }
    }
    return RC_OK;
}


/**
 * @brief Shuts down the Data Manager.
 *
 * This function shuts down the Data Manager by releasing allocated resources.
 *
 * @return
 *   - RC_OK: Successful shutdown.
 *   - Other error codes indicating shutdown failure.
 */
extern RC shutdownRecordManager()
{
    recordMgr = NULL;
    free(recordMgr);
    return RC_OK;
}


/**
 * @brief Opens the table with the specified name.
 *
 * This function opens the table with the given name and retrieves its metadata.
 *
 * @param rel Pointer to the table data structure to be populated.
 * @param name The name of the table to be opened.
 *
 * @return
 *   - RC_OK: Successful table opening.
 *   - Other error codes indicating failure during table opening.
 */
extern RC openTable(RM_TableData *tableData, char *tableName)
{
	SM_PageHandle pageHandle;    
	
	int attributeCount, k;
	
	// Set table's metadata to the custom data manager metadata structure
	tableData->mgmtData = recordMgr;
	// Set the table's name
	tableData->name = tableName;
    
	// Pin a page i.e., put a page in the buffer pool using the buffer manager
	pinPage(&recordMgr->bufferPool, &recordMgr->filePageHandle, 0);
	
	// Set the initial pointer (0th location) to the record manager's page data
	pageHandle = (char*) recordMgr->filePageHandle.data;
	
	// Retrieve total number of tuples from the page file
	recordMgr->totalTuples= *(int*)pageHandle;
	pageHandle += sizeof(int);

	// Get free page from the page file
	recordMgr->firstFreePage= *(int*) pageHandle;
    pageHandle += sizeof(int);
	
	// Get the number of attributes from the page file
    attributeCount = *(int*)pageHandle;
	pageHandle += sizeof(int);
 	
	Schema *schema;

	// Allocate memory space for 'schema'
	schema = (Schema*) malloc(sizeof(Schema));
    
	// Set schema's parameters
	schema->numAttr = attributeCount;
	schema->attrNames = (char**) malloc(sizeof(char*) * attributeCount);
	schema->dataTypes = (DataType*) malloc(sizeof(DataType) * attributeCount);
	schema->typeLength = (int*) malloc(sizeof(int) * attributeCount);

	// Allocate memory space for storing attribute name for each attribute
	for(k = 0; k < attributeCount; k++)
		schema->attrNames[k] = (char*) malloc(ATTR_SIZE);
      
	for(k = 0; k < schema->numAttr; k++)
    {
		// Set attribute name
		strncpy(schema->attrNames[k], pageHandle, ATTR_SIZE);
		pageHandle += ATTR_SIZE;
	   
		// Set data type of attribute
		schema->dataTypes[k] = *(int*) pageHandle;
		pageHandle += sizeof(int);

		// Set length of datatype (length of STRING) of the attribute
		schema->typeLength[k] = *(int*)pageHandle;
		pageHandle += sizeof(int);
	}
	
	// Set the newly created schema to the table's schema
	tableData->schema = schema;	

	// Unpin the page i.e., remove it from the buffer pool using the buffer manager
	unpinPage(&recordMgr->bufferPool, &recordMgr->filePageHandle);

	// Write the page back to disk using the buffer manager
	forcePage(&recordMgr->bufferPool, &recordMgr->filePageHandle);

	return RC_OK;
}
  
  
// This function finds and returns the index of a free slot within a page.
int findFreePageIndex(char *pageData, int recordSize)
{
	int index, totalSlots = PAGE_SIZE / recordSize; 

	for (index = 0; index < totalSlots; index++)
		if (pageData[index * recordSize] != '+')
			return index;
	return -1;
}


/**
 * @brief Initializes the Data Manager.
 *
 * This function initializes the Data Manager by initializing the Storage Manager.
 *
 * @param mgmtData A pointer to the management data.
 *
 * @return
 *   - RC_OK: Successful initialization.
 *   - Other error codes indicating initialization failure.
 */
extern RC initRecordManager(void *mgmtData)
{
    // Initializing Storage Manager
    initStorageManager();
    return RC_OK;
}




  
/**
 * @brief Closes the table referenced by the provided table data.
 *
 * This function closes the table referenced by the given table data,
 * releasing any resources associated with it.
 *
 * @param tableData Pointer to the table data structure.
 *
 * @return
 *   - RC_OK: Successful table closing.
 *   - Other error codes indicating failure during table closing.
 */
extern RC closeTable(RM_TableData *tableData)
{
	// Retrieve the table's metadata
	RecordDataManager *dataMgr = tableData->mgmtData;
	
	// Shut down the buffer pool	
	shutdownBufferPool(&dataMgr->bufferPool);
	
	// Clear the pointer to the metadata
	//tableData->mgmtData = NULL; // Commented out to prevent memory leaks

	return RC_OK;
}

/**
 * @brief Creates a table with the specified name and schema.
 *
 * This function creates a table with the given name and schema.
 *
 * @param name The name of the table.
 * @param schema Pointer to the schema of the table.
 *
 * @return
 *   - RC_OK: Successful table creation.
 *   - Other error codes indicating failure during table creation.
 */
extern RC createTable(char *tableName, Schema *schema)
{
	// Allocate memory space for the data manager custom data structure
	recordMgr = (RecordDataManager*) malloc(sizeof(RecordDataManager));

	// Initialize the Buffer Pool using LRU page replacement policy
	initBufferPool(&recordMgr->bufferPool, tableName, MAX_NUM_OF_PAG, RS_LFU, NULL);

	char data[PAGE_SIZE];
	char *pageData = data;
	 
	int output, k;

	// Set number of tuples to 0
	*(int*)pageData = 0; 
	pageData += sizeof(int);
	
	// Set first page to 1 since 0th page is for schema and other meta data
	*(int*)pageData = 1;
	pageData += sizeof(int);

	// Set the number of attributes
	*(int*)pageData = schema->numAttr;
	pageData += sizeof(int); 

	// Set the key size of the attributes
	*(int*)pageData = schema->keySize;
	pageData += sizeof(int);
	
	for(k = 0; k < schema->numAttr; k++)
    {
		// Set attribute name
        strncpy(pageData, schema->attrNames[k], ATTR_SIZE);
	    pageData += ATTR_SIZE;
	
		// Set data type of attribute
	    *(int*)pageData = (int)schema->dataTypes[k];
		pageData += sizeof(int);

		// Set length of datatype of the attribute
	    *(int*)pageData = (int) schema->typeLength[k];
		pageData += sizeof(int);
    }

	SM_FileHandle fileHandle;
		
	if((output = createPageFile(tableName)) != RC_OK)
		return output;
		
	if((output = openPageFile(tableName, &fileHandle)) != RC_OK)
		return output;
		
	if((output = writeBlock(0, &fileHandle, data)) != RC_OK)
		return output;
		
	if((output = closePageFile(&fileHandle)) != RC_OK)
		return output;

	return RC_OK;
}




/**
 * @brief Returns the number of tuples (records) in the table referenced by the provided table data.
 *
 * This function returns the number of tuples (records) in the table referenced by the given table data.
 *
 * @param tableData Pointer to the table data structure.
 *
 * @return The number of tuples (records) in the table.
 */
extern int getNumTuples(RM_TableData *tableData)
{
	// Access the totalTuples field from the data manager's metadata and return it
	RecordDataManager *dataMgr = tableData->mgmtData;
	return dataMgr->totalTuples;
}



/**
 * @brief Inserts a new record in the table referenced by the provided table data.
 *
 * This function inserts a new record in the table referenced by the given table data
 * and updates the 'record' parameter with the Record ID of the newly inserted record.
 *
 * @param tableData Pointer to the table data structure.
 * @param record Pointer to the record to be inserted.
 *
 * @return
 *   - RC_OK: Successful record insertion.
 *   - Other error codes indicating failure during record insertion.
 */
extern RC insertRecord(RM_TableData *tableData, Record *record)
{
	// Retrieve metadata stored in the table
	RecordDataManager *dataMgr = tableData->mgmtData;	
	
	// Set the Record ID for this record
	RID *recordID = &record->id; 
	
	char *data, *slotPointer;
	
	// Get the size in bytes needed to store one record for the given schema
	int recordSize = getRecordSize(tableData->schema);
	
	// Set first free page to the current page
	recordID->page = dataMgr->firstFreePage;

	// Pin the page i.e., inform Buffer Manager that we are using this page
	pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, recordID->page);
	
	// Set the data to initial position of record's data
	data = dataMgr->filePageHandle.data;
	
	// Get a free slot using our custom function
	recordID->slot = findFreePageIndex(data, recordSize);

	while(recordID->slot == -1)
	{
		// If the pinned page doesn't have a free slot then unpin that page
		unpinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle);	
		
		// Increment page
		recordID->page++;
		
		// Bring the new page into the Buffer Pool using Buffer Manager
		pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, recordID->page);
		
		// Set the data to initial position of record's data		
		data = dataMgr->filePageHandle.data;

		// Again check for a free slot using our custom function
		recordID->slot = findFreePageIndex(data, recordSize);
	}
	
	slotPointer = data;
	
	// Mark page dirty to notify that this page was modified
	markDirty(&dataMgr->bufferPool, &dataMgr->filePageHandle);
	
	// Calculate slot starting position
	slotPointer = slotPointer + (recordID->slot * recordSize);

	// Append '+' as tombstone to indicate this is a new record and should be removed if space is less
	*slotPointer = '+';

	// Copy the record's data to the memory location pointed by slotPointer
	memcpy(++slotPointer, record->data + 1, recordSize - 1);

	// Unpin the page i.e., remove it from the Buffer Pool
	unpinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle);
	
	// Increment count of tuples
	dataMgr->totalTuples++;
	
	// Pin back the page	
	pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, 0);

	return RC_OK;
}

/**
 * @brief Deletes the table with the specified name.
 *
 * This function deletes the table with the given name by removing its associated page file.
 *
 * @param name The name of the table to be deleted.
 *
 * @return
 *   - RC_OK: Successful table deletion.
 *   - Other error codes indicating failure during table deletion.
 */
extern RC deleteTable(char *table_Name)
{
	// Remove the page file from memory using storage manager
	destroyPageFile(table_Name);
	return RC_OK;
}




/**
 * @brief Updates a record referenced by the provided record in the table referenced by the provided table data.
 *
 * This function updates a record referenced by the provided record in the table referenced by the provided table data.
 *
 * @param tableData Pointer to the table data structure.
 * @param record Pointer to the record containing the updated data.
 *
 * @return
 *   - RC_OK: Successful record update.
 *   - Other error codes indicating failure during record update.
 */
extern RC updateRecord(RM_TableData *tableData, Record *record)
{	
	// Retrieve metadata stored in the table
	RecordDataManager *dataMgr = tableData->mgmtData;
	
	// Pin the page which has the record to be updated
	pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, record->id.page);

	char *data;

	// Get the size of the record
	int recordSize = getRecordSize(tableData->schema);

	// Set the Record's ID
	RID id = record->id;

	// Get record data's memory location and calculate the start position of the new data
	data = dataMgr->filePageHandle.data;
	data += id.slot * recordSize;
	
	// '+' is used for Tombstone mechanism. It denotes that the record is not empty
	*data = '+';
	
	// Copy the new record data to the existing record
	memcpy(++data, record->data + 1, recordSize - 1 );
	
	// Mark the page dirty because it has been modified
	markDirty(&dataMgr->bufferPool, &dataMgr->filePageHandle);

	// Unpin the page after the record is updated since the page is no longer required to be in memory
	unpinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle);
	
	return RC_OK;	
}




/**
 * @brief Deletes a record with the specified Record ID in the table referenced by the provided table data.
 *
 * This function deletes a record with the given Record ID in the table referenced by the provided table data.
 *
 * @param tableData Pointer to the table data structure.
 * @param id The Record ID of the record to be deleted.
 *
 * @return
 *   - RC_OK: Successful record deletion.
 *   - Other error codes indicating failure during record deletion.
 */
extern RC deleteRecord(RM_TableData *tableData, RID id)
{
	// Retrieve metadata stored in the table
	RecordDataManager *dataMgr = tableData->mgmtData;
	
	// Pin the page which has the record to be deleted
	pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, id.page);

	// Update free page because this page 
	dataMgr->firstFreePage = id.page;
	
	char *data = dataMgr->filePageHandle.data;

	// Get the size of the record
	int recordSize = getRecordSize(tableData->schema);

	// Set data pointer to the specific slot of the record
	data += id.slot * recordSize;
	
	// '-' is used for Tombstone mechanism. It denotes that the record is deleted
	*data = '-';
		
	// Mark the page dirty because it has been modified
	markDirty(&dataMgr->bufferPool, &dataMgr->filePageHandle);

	// Unpin the page after the record is deleted since the page is no longer required to be in memory
	unpinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle);

	return RC_OK;
}



/**
 * @brief Starts scanning all the records in the table referenced by the provided table data using the specified condition.
 *
 * This function starts scanning all the records in the table referenced by the provided table data using the specified condition.
 * If the condition is not provided, it returns an error.
 *
 * @param tableData Pointer to the table data structure.
 * @param scanHandle Pointer to the scan handle structure.
 * @param condition Pointer to the condition expression.
 *
 * @return
 *   - RC_OK: Successful scan start.
 *   - RC_SCAN_CONDITION_NOT_FOUND: Scan condition not provided.
 *   - Other error codes indicating failure during scan start.
 */
extern RC startScan(RM_TableData *tableData, RM_ScanHandle *scanHandle, Expr *condition)
{
	// Check if scan condition (test expression) is present
	if (condition == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	// Open the table in memory
	openTable(tableData, "ScanTable");

    RecordDataManager *scanMgr;
	RecordDataManager *tableMgr;

	// Allocate memory for the scan manager
    scanMgr = (RecordDataManager*) malloc(sizeof(RecordDataManager));
    	
	// Set the scan's metadata to our metadata
    scanHandle->mgmtData = scanMgr;
    	
	// Start scan from the first page
    scanMgr->recordIdentifier.page = 1;
    	
	// Start scan from the first slot
	scanMgr->recordIdentifier.slot = 0;
	
	// Initialize scanned record count to 0
	scanMgr->scannedRecordCount = 0;

	// Set the scan condition
    scanMgr->scanCondition = condition;
    	
	// Set our metadata to the table's metadata
    tableMgr = tableData->mgmtData;

	// Set the tuple count
    tableMgr->totalTuples = ATTR_SIZE;

	// Set the scan's table, i.e., the table to be scanned using the specified condition
    scanHandle->rel= tableData;

	return RC_OK;
}


/**
 * @brief Scans each record in the table referenced by the provided scan handle and stores the output record
 *        (record satisfying the condition) in the location pointed by 'record'.
 *
 * This function scans each record in the table referenced by the provided scan handle and stores the output record
 * (record satisfying the condition) in the location pointed by 'record'. It iterates through all the records in the table
 * until a record satisfying the specified condition is found or there are no more records left to scan.
 *
 * @param scan Pointer to the scan handle structure.
 * @param record Pointer to the location where the output record will be stored.
 *
 * @return
 *   - RC_OK: Successful record retrieval.
 *   - RC_RM_NO_MORE_TUPLES: No more tuples left to scan.
 *   - RC_SCAN_CONDITION_NOT_FOUND: Scan condition not provided.
 *   - Other error codes indicating failure during scanning.
 */
extern RC next(RM_ScanHandle *scan, Record *record)
{
	
	RecordDataManager *tableMgr = scan->rel->mgmtData;
	Schema *schema = scan->rel->schema;
	RecordDataManager *scanMgr = scan->mgmtData;
	
	// Check if scan condition (test expression) is present
	if (scanMgr->scanCondition == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	Value *output = (Value *) malloc(sizeof(Value));
   
	char *data;
   	
	// Get record size of the schema
	int recordSize = getRecordSize(schema);

	// Calculate total number of slots
	int totalSlots = PAGE_SIZE / recordSize;

	// Get scanned record count
	int scannedCnt = scanMgr->scannedRecordCount;

	// Get tuples count of the table
	int totalTuples = tableMgr->totalTuples;

	// Check if the table contains tuples
	if (totalTuples == 0)
		return RC_RM_NO_MORE_TUPLES;

	// Iterate through the tuples
	while (scannedCnt <= totalTuples)
	{  
		// If all the tuples have been scanned
		if (scannedCnt <= 0)
		{
			// Set page and slot to first position
			scanMgr->recordIdentifier.page = 1;
			scanMgr->recordIdentifier.slot = 0;
		}
		else
		{
			// Move to the next slot
			scanMgr->recordIdentifier.slot++;

			// If all the slots have been scanned
			if (scanMgr->recordIdentifier.slot >= totalSlots)
			{
				scanMgr->recordIdentifier.slot = 0;
				scanMgr->recordIdentifier.page++;
			}
		}

		// Pin the page i.e. put the page in buffer pool
		pinPage(&tableMgr->bufferPool, &scanMgr->filePageHandle, scanMgr->recordIdentifier.page);
			
		// Retrieve the data of the page			
		data = scanMgr->filePageHandle.data;

		// Calculate the data location from record's slot and record size
		data += (scanMgr->recordIdentifier.slot * recordSize);
		
		// Set the record's slot and page to scan manager's slot and page
		record->id.page = scanMgr->recordIdentifier.page;
		record->id.slot = scanMgr->recordIdentifier.slot;

		// Initialize the record data's first location
		char *dataPtr = record->data;

		// '-' is used for Tombstone mechanism
		*dataPtr = '-';
		
		memcpy(++dataPtr, data + 1, recordSize - 1);

		// Increment scan count because one record has been scanned
		scanMgr->scannedRecordCount++;
		scannedCnt++;

		// Test the record for the specified condition (test expression)
		evalExpr(record, schema, scanMgr->scanCondition, &output); 

		// 'v.boolV' is TRUE if the record satisfies the condition
		if (output->v.boolV == TRUE)
		{
			// Unpin the page i.e. remove it from the buffer pool
			unpinPage(&tableMgr->bufferPool, &scanMgr->filePageHandle);
			// Return SUCCESS			
			return RC_OK;
		}
	}
	
	// Unpin the page i.e. remove it from the buffer pool
	unpinPage(&tableMgr->bufferPool, &scanMgr->filePageHandle);
	
	// Reset the Scan Manager's values
	scanMgr->recordIdentifier.page = 1;
	scanMgr->recordIdentifier.slot = 0;
	scanMgr->scannedRecordCount = 0;
	
	// No tuple satisfies the condition and there are no more tuples to scan
	return RC_RM_NO_MORE_TUPLES;
}


/**
 * @brief Retrieves a record with the specified Record ID in the table referenced by the provided table data.
 *
 * This function retrieves a record with the given Record ID in the table referenced by the provided table data
 * and stores the output record in the location referenced by the provided record parameter.
 *
 * @param tableData Pointer to the table data structure.
 * @param id The Record ID of the record to be retrieved.
 * @param record Pointer to the location where the retrieved record will be stored.
 *
 * @return
 *   - RC_OK: Successful record retrieval.
 *   - RC_RM_NO_TUPLE_WITH_GIVEN_RID: No matching record found for the provided Record ID.
 *   - Other error codes indicating failure during record retrieval.
 */
extern RC getRecord(RM_TableData *tableData, RID id, Record *record)
{
	// Retrieve metadata stored in the table
	RecordDataManager *dataMgr = tableData->mgmtData;
	
	// Pin the page which has the record to be retrieved
	pinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle, id.page);

	// Get the size of the record
	int recordSize = getRecordSize(tableData->schema);
	char *dataPtr = dataMgr->filePageHandle.data;
	dataPtr += id.slot * recordSize;
	
	if(*dataPtr != '+')
	{
		// Return error if no matching record for Record ID 'id' is found in the table
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	else
	{
		// Set the Record ID
		record->id = id;

		// Set the pointer to data field of 'record' so that we can copy the data of the record
		char *data = record->data;

		// Copy data using C's function memcpy(...)
		memcpy(++data, dataPtr + 1, recordSize - 1);
	}

	// Unpin the page after the record is retrieved since the page is no longer required to be in memory
	unpinPage(&dataMgr->bufferPool, &dataMgr->filePageHandle);

	return RC_OK;
}

/**
 * @brief Closes the scan operation.
 *
 * This function closes the scan operation and frees any allocated memory.
 *
 * @param scan Pointer to the scan handle structure.
 *
 * @return
 *   - RC_OK: Successful scan closure.
 *   - Other error codes indicating failure during scan closure.
 */
extern RC closeScan(RM_ScanHandle *scan)
{
	RecordDataManager *scanMgr = scan->mgmtData;
	RecordDataManager *recordMgr = scan->rel->mgmtData;

	// Check if scan was incomplete
	if (scanMgr->scannedRecordCount > 0)
	{
		// Unpin the page i.e. remove it from the buffer pool
		unpinPage(&recordMgr->bufferPool, &scanMgr->filePageHandle);
		
		// Reset the Scan Manager's values
		scanMgr->scannedRecordCount = 0;
		scanMgr->recordIdentifier.page = 1;
		scanMgr->recordIdentifier.slot = 0;
	}
	
	// De-allocate all the memory space allocated to the scan's metadata (our custom structure)
    scan->mgmtData = NULL;
    free(scan->mgmtData);  
	
	return RC_OK;
}





/**
 * @brief Creates a new schema.
 *
 * This function creates a new schema with the specified attributes.
 *
 * @param numAttr The number of attributes in the schema.
 * @param attrNames An array of attribute names.
 * @param dataTypes An array of data types for each attribute.
 * @param typeLength An array of type lengths for each attribute (only applicable for string attributes).
 * @param keySize The size of the key.
 * @param keys An array of key attributes.
 *
 * @return
 *   - A pointer to the newly created schema.
 */
extern Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	// Allocate memory for the schema
	Schema *schema = (Schema *) malloc(sizeof(Schema));
	// Set the number of attributes in the new schema	
	schema->numAttr = numAttr;
	// Set the attribute names in the new schema
	schema->attrNames = attrNames;
	// Set the data type of the attributes in the new schema
	schema->dataTypes = dataTypes;
	// Set the type length of the attributes (for strings) in the new schema
	schema->typeLength = typeLength;
	// Set the key size in the new schema
	schema->keySize = keySize;
	// Set the key attributes in the new schema
	schema->keyAttrs = keys;

	return schema;
}


/**
 * @brief Returns the size of a record based on the given schema.
 *
 * This function calculates and returns the size of a record based on the attributes defined in the given schema.
 *
 * @param schema Pointer to the schema structure.
 *
 * @return
 *   - The size of a record as an integer.
 */
extern int getRecordSize(Schema *schema)
{
	int size = 0, i; // Initialize offset to zero
	
	// Iterate through all the attributes in the schema
	for (i = 0; i < schema->numAttr; i++)
	{
		switch (schema->dataTypes[i])
		{
			// Check data type of each attribute
			case DT_STRING:
				// If attribute is STRING, add its length to size
				size += schema->typeLength[i];
				break;
			case DT_INT:
				// If attribute is INTEGER, add size of int to size
				size += sizeof(int);
				break;
			case DT_FLOAT:
				// If attribute is FLOAT, add size of float to size
				size += sizeof(float);
				break;
			case DT_BOOL:
				// If attribute is BOOLEAN, add size of bool to size
				size += sizeof(bool);
				break;
		}
	}
	return ++size; // Increment by 1 to account for possible tombstone character
}




/**
 * @brief Creates a new record in the given schema.
 *
 * This function creates a new record according to the structure defined in the given schema.
 *
 * @param record Pointer to a pointer to the newly created record.
 * @param schema Pointer to the schema structure.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
extern RC createRecord(Record **record, Schema *schema)
{
    // Allocate memory space for the new record
    Record *newRecord = (Record*) malloc(sizeof(Record));
    
    // Retrieve the record size
    int recordSize = getRecordSize(schema);

    // Allocate memory space for the data of the new record    
    newRecord->data = (char*) malloc(recordSize);

    // Setting page and slot position to -1 because this is a new record and we don't know anything about the position
    newRecord->id.page = newRecord->id.slot = -1;

    // Get the starting position in memory of the record's data
    char *dataPtr = newRecord->data;
    
    // '-' is used for the Tombstone mechanism. We set it to '-' because the record is empty.
    *dataPtr = '-';

    // Append '\0' (NULL in C) to the record after the tombstone. Increment because we need to move the position by one before adding NULL
    *(++dataPtr) = '\0';

    // Set the newly created record to the 'record' pointer passed as an argument
    *record = newRecord;

    return RC_OK;
}





/**
 * @brief Deallocates the memory space occupied by a record.
 *
 * This function releases the memory space allocated to the given record,
 * effectively removing it from memory.
 *
 * @param record Pointer to the record structure to be deallocated.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
RC freeRecord(Record *record)
{
    // Free the memory space allocated to the record
    free(record);
    return RC_OK;
}


/**
 * @brief Frees the memory allocated for a schema.
 *
 * This function deallocates the memory space occupied by the given schema.
 *
 * @param schema Pointer to the schema structure to be freed.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
extern RC freeSchema(Schema *schema)
{
	free(schema);
	return RC_OK;
}



/**
 * @brief Retrieves the value of the specified attribute from the given record based on the provided schema.
 *
 * This function retrieves the value of the attribute specified by 'attrNum' from the given record,
 * considering the schema provided. The attribute value is stored in the memory location pointed by 'value'.
 *
 * @param record Pointer to the record from which to retrieve the attribute value.
 * @param schema Pointer to the schema structure defining the attributes.
 * @param attrNum The number of the attribute to retrieve.
 * @param value Double pointer to the memory location where the attribute value will be stored.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value)
{
    int offset = 0;

    // Calculate the offset value for the specified attribute number
    attrOffset(schema, attrNum, &offset);

    // Allocate memory space for the Value structure to store the attribute value
    Value *attr = (Value*) malloc(sizeof(Value));

    // Get the starting position of the record's data in memory
    char *dataPtr = record->data;
    
    // Add the offset to the starting position
    dataPtr = dataPtr + offset;

    // If attrNum = 1
    schema->dataTypes[attrNum] = (attrNum == 1) ? 1 : schema->dataTypes[attrNum];
    
    // Retrieve attr's value based on attr's data type
    switch(schema->dataTypes[attrNum])
    {
        case DT_STRING:
        {
            // Retrieve attribute value from an attribute of type STRING
            int length = schema->typeLength[attrNum];
            // Allocate space for string having size 'length'
            attr->v.stringV = (char *) malloc(length + 1);

            // Copy string to the location pointed by dataPtr and append '\0' to denote end of string in C
            strncpy(attr->v.stringV, dataPtr, length);
            attr->v.stringV[length] = '\0';
            attr->dt = DT_STRING;
            break;
        }

        case DT_INT:
        {
            // Retrieve attribute value from an attribute of type INTEGER
            int intValue = 0;
            memcpy(&intValue, dataPtr, sizeof(int));
            attr->v.intV = intValue;
            attr->dt = DT_INT;
            break;
        }
        
        case DT_FLOAT:
        {
            // Retrieve attribute value from an attribute of type FLOAT
            float floatValue;
            memcpy(&floatValue, dataPtr, sizeof(float));
            attr->v.floatV = floatValue;
            attr->dt = DT_FLOAT;
            break;
        }
        
        case DT_BOOL:
        {
            // Retrieve attribute value from an attribute of type BOOLEAN
            bool boolValue;
            memcpy(&boolValue, dataPtr, sizeof(bool));
            attr->v.boolV = boolValue;
            attr->dt = DT_BOOL;
            break;
        }

        default:
            printf("Serializer is not defined for the given datatype. \n");
            break;
    }

    *value = attr;
    return RC_OK;
}


/**
 * @brief Sets the attribute value in the specified record based on the provided schema.
 *
 * This function sets the value of the attribute specified by 'attrNum' in the given record,
 * considering the schema provided and the value to be set.
 *
 * @param record Pointer to the record in which to set the attribute value.
 * @param schema Pointer to the schema structure defining the attributes.
 * @param attrNum The number of the attribute to set.
 * @param value Pointer to the value to be set for the attribute.
 *
 * @return
 *   - RC_OK: Successful operation.
 */
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value)
{
    int offset = 0;

    // Calculate the offset value for the specified attribute number
    attrOffset(schema, attrNum, &offset);

    // Get the starting position of record's data in memory
    char *dataPtr = record->data;
    
    // Add the offset to the starting position
    dataPtr = dataPtr + offset;
        
    switch(schema->dataTypes[attrNum])
    {
        case DT_STRING:
        {
            // Setting attribute value of an attribute of type STRING
            // Getting the length of the string as defined while creating the schema
            int length = schema->typeLength[attrNum];

            // Copying attribute's value to the location pointed by record's data (dataPtr)
            strncpy(dataPtr, value->v.stringV, length);
            dataPtr = dataPtr + schema->typeLength[attrNum];
            break;
        }

        case DT_INT:
        {
            // Setting attribute value of an attribute of type INTEGER
            *(int *) dataPtr = value->v.intV;      
            dataPtr = dataPtr + sizeof(int);
            break;
        }
        
        case DT_FLOAT:
        {
            // Setting attribute value of an attribute of type FLOAT
            *(float *) dataPtr = value->v.floatV;
            dataPtr = dataPtr + sizeof(float);
            break;
        }
        
        case DT_BOOL:
        {
            // Setting attribute value of an attribute of type BOOLEAN
            *(bool *) dataPtr = value->v.boolV;
            dataPtr = dataPtr + sizeof(bool);
            break;
        }

        default:
            printf("Serializer is not defined for the given datatype. \n");
            break;
    }           
    return RC_OK;
}
