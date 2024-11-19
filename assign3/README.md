## Contributions

| Team Member                            | Contributions                                       |
|----------------------------------------|-----------------------------------------------------|
| Deepak Kumar (A20547017)               | 1/3                                                 |
| Shubham Bajirao Dhanavade (A20541092)  | 1/3                                                 |
| Siddhant Sarnobat (A20543734)          | 1/3                                                 |

## Aim

The primary objective of this assignment is to implement a fundamental record management system. The record manager handles tables with fixed schemas, allowing clients to perform operations such as adding, removing, updating, and browsing records within a table. Additionally, it supports scans based on specific search conditions, returning only the records that meet the specified criteria. Each table is stored in a separate page file, and the record manager is designed to access the file's pages using the buffer manager implemented in the previous assignment.


### Extra Test Cases
 
1. Added Some new test cases for `test_expr` under -
   - `testExpressions()`  
   - `testValueSerialize()`
2. For test_assing3_1 added two new test cases -
   -  `test_insertAdditionalTuples()`
   -  `test_insertMoreTuples()`


### Instructions for running the code

1. Execute "**make clean**" to remove previously generated objects and executable files.
2. Run "**make test_assign3**" to compile the test_assign3_1.c file.
3. Execute "**make run_test_assign3**" to run the test_assign3 executable.
4. Run "**make test_expr**" to compile the test_expr.c file.
5. Execute "**make run_test_expr**" to run the test_expr executable (for Linux or Mac users).


### Description of functions used-

#### Table and Record Manager Functions

The record manager is initialized and terminated using functions related to the record manager. Table-related functions are used to create, open, close, and delete tables. Access to pages of the file is facilitated through the buffer manager's page replacement strategy. Additionally, file-related operations are indirectly performed through the storage manager.

1. `initRecordManager ()`

   - This function initializes the record manager.
   - It calls the initStorageManager function of the Storage Manager to initialize the storage manager.

2. `shutdownRecordManager()`

   - This function deallocates all resources allocated to the record manager and shuts it down.
   - All memory and resources used by the Record Manager are released.
   - The recordManager data structure pointer is set to NULL, and memory is deallocated using the C function free().

3. `createTable()`

   - This function creates a table with the specified name.
   - It calls initBufferPool() to initialize the Buffer Pool with a policy of replacing pages using LRUs.
   - The function initializes the properties of the table (name, data type, and size) and initializes all table values. After that, it creates a page file, opens it, writes the block containing the table to the page file, and then closes the page file.
   

4. `openTable()`

   - This function is used to access table data.
   - It pins the first page to retrieve metadata after initializing the buffer pool.
   - The schema is read from the file, and then the page is unpinned.

5. `closeTable()`

   - This function closes the table specified by the parameter 'rel'.
   - It invokes the shutdownBufferPool() function of the Buffer Manager.
   - The Buffer Manager writes the modifications made to the table to the page file before shutting down.

6. `deleteTable()`

   - This function deletes the table with the specified name.
   - It invokes the destroyPageFile() function of the Storage Manager.
   - The destroyPageFile() function removes the page from the hard drive and frees up the memory space allocated for that file.

7. `getNumTuples()`

   - This function returns the number of tuples in the table specified by the parameter 'rel'.
   - It returns the tuplesCount variable, which stores metadata for the tables.

#### Record Functions

These functions are used to insert a new record, update an existing record, delete a record, and retrieve a record.

1. `insertRecord()`

   - This function inserts a new record into the table.
   - The inserted record is assigned a Record ID.
   - It pins the page containing an empty slot, adds a '+', indicating a newly inserted record, marks the page as dirty to write its contents back to disk, and then unpins the page.
   - The record's data is copied using the memcpy() function.

2. `deleteRecord()`

   - This function deletes a record from the table specified by 'rel' using the Record ID 'id'.
   - It updates the table's metadata to free up the space occupied by the deleted record, marks the record as '-', indicating deletion, pins the page containing the record's data, marks it as dirty, and then unpins the page.

3. `updateRecord()`

   - This function updates an existing record in the table specified by 'rel' using the provided 'record'.
   - It locates the record's page using the table's metadata, pins the page, updates the Record ID, copies the record's data, marks the page as dirty, and then unpins the page.

4. `getRecord()`

   - This function retrieves a record from the table specified by 'rel' using the Record ID 'id' and stores it in the 'record' parameter.
   - It locates the record's page using the table's metadata, pins the page, transfers the data to the 'record' parameter, sets the Record ID, and then unpins the page.

#### Scan Functions

Scan functions are used to retrieve tuples from a table that satisfy a specific condition.

1. `startScan()`

   - This function initiates a scan on the table specified by 'rel' using the given condition.
   - It initializes variables related to the scan and returns an error code if the condition is NULL.

2. `next()`

   - This function retrieves the next tuple that satisfies the scan condition.
   - It iterates over each tuple in the table, evaluates the condition, and returns the next tuple that satisfies the condition.
   - If no tuples satisfy the condition, it returns an error code indicating no more tuples.

3. `closeScan()`

   - This function ends the scanning process.
   - It checks if the scan was complete and releases memory allocated for the scan.


#### Schema Functions

These functions facilitate the creation of a new schema and provide the size of records for a given schema in bytes.

1. `getRecordSize(schema)`

   - The `getRecordSize()` function calculates the size of a record within the provided schema.
   - Parameters:
     - `schema`: The schema for which record size is calculated. (Type: schema object)
   - It iterates through the attributes of the schema, adding the size of each attribute (in bytes) to the variable "size."

2. `freeSchema(schema)`

   - This function deallocates the memory allocated for the stored schema.
   - Parameters:
     - `schema`: The schema object to be deallocated. (Type: schema object)
   - It utilizes the `refNum` field in each page frame to track the page frames accessed by the client.
   - Memory occupied by the schema is released using the C function `free()`.

3. `createSchema(numAttr, attrNames, dataTypes, typeLength)`

   - The `createSchema()` function creates a new schema in memory based on the provided parameters.
   - Parameters:
     - `numAttr`: Number of attributes in the schema. (Type: integer)
     - `attrNames`: Names of the attributes. (Type: list of strings)
     - `dataTypes`: Data types of the attributes. (Type: list of data types)
     - `typeLength`: Length of data types (e.g., length of a STRING). (Type: list of integers)
   - The function initializes a schema object and allocates memory space for it. Then, it sets the schema parameters based on the supplied values.

#### Attribute Functions

These functions handle the creation of new records, retrieval, and modification of attribute values within existing records for a given schema. Memory allocation for the data field when creating a new record must accommodate the binary representations of all attributes as specified by the schema.

1. `createRecord(schema)`

   - The `createRecord()` function generates a new record based on the provided schema and assigns it to the `record` parameter.
   - Parameters:
     - `schema`: The schema based on which the record is created. (Type: schema object)
   - It allocates the appropriate memory for the new record and its data field, ensuring it accommodates the entire record size.
   - Additionally, it appends '\0' (NULL in C) to the first position and adds a '-' to indicate a new blank record.

2. `attrOffset(schema, attrNum)`

   - The `attrOffset()` function calculates the offset (in bytes) from the initial position to the specified attribute within the record.
   - Parameters:
     - `schema`: The schema containing the attribute. (Type: schema object)
     - `attrNum`: The attribute number for which offset is calculated. (Type: integer)
   - It iterates through the schema's attributes until the designated attribute number, summing up the space (in bytes) needed for each property.

3. `freeRecord(record)`

   - This function releases the memory allocated for the provided record.
   - Parameters:
     - `record`: The record to be deallocated. (Type: record object)
   - Memory used by the record is deallocated using the C function `free()`.

4. `getAttr(schema, record, attrNum, value)`

   - The `getAttr()` function retrieves an attribute from the given schema using the supplied record.
   - Parameters:
     - `schema`: The schema containing the attribute. (Type: schema object)
     - `record`: The record from which the attribute is retrieved. (Type: record object)
     - `attrNum`: The attribute number to retrieve. (Type: integer)
     - `value`: The variable to store the retrieved attribute value. (Type: data type)
   - It fetches the record, schema, and attribute number to extract the attribute's data.
   - Using the `attrOffset()` function, it locates the attribute's position, copies the attribute's data type and value to the `value` parameter based on the attribute's data type.

5. `setAttr(schema, record, attrNum, value)`

   - `setAttr()` function updates the attribute value within the record using the provided schema.
   - Parameters:
     - `schema`: The schema containing the attribute. (Type: schema object)
     - `record`: The record in which the attribute is updated. (Type: record object)
     - `attrNum`: The attribute number to update. (Type: integer)
     - `value`: The new value for the attribute. (Type: data type)
   - The attribute value is passed via the `value` parameter.
   - By utilizing the `attrOffset()` function, it navigates to the attribute's position, copying the data from the `value` parameter to the attribute's data type and value, based on the attribute's data type.