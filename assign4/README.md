## Contributions

| Team Member                            | Contributions                                       |
|----------------------------------------|-----------------------------------------------------|
| Deepak Kumar (A20547017)               | 1/3                                                 |
| Shubham Bajirao Dhanavade (A20541092)  | 1/3                                                 |
| Siddhant Sarnobat (A20543734)          | 1/3                                                 |

## Aim

The goal of this assignment is to implement a B+tree index. The index should be backed up by a page file and pages of the index should be accessed through your buffer manager. As discussed in the lecture, each node should occupy one page. However, for debugging purposes you should support trees with a smaller fan-out and still let each node occupy a full page. A B+tree stores pointer to records (the RID introduced in the last assignment) index by keys of a given datatype. The assignment only requires you to support DT INT (integer) keys (see optional extensions). Pointers to intermediate nodes should be represented by the page number of the page the node is stored in.

### Extra Test Cases
 

1. For test_assing4_1 added two new test cases -
   -  `testInsertAndFind_String()`
   -  `testDelete_String()`
   
2. Added Some new test cases for `test_expr` under -
   - `testExpressions()`  
   - `testValueSerialize()`


### Instructions for running the code

1. Execute "**make clean**" to remove previously generated objects and executable files.
2. Run "**make test_assign4**" to compile the test_assign4_1.c file.
3. Execute "**make run_test_assign4**" to run the test_assign4 executable.
4. Run "**make test_expr**" to compile the test_expr.c file.
5. Execute "**make run_test_expr**" to run the test_expr executable (for Linux or Mac users).


## Memory Management

- Ensured proper memory management while developing the B+ Tree.
- Freed any reserved space whenever possible.
- Minimized the use of variables to optimize memory usage.

## Custom B+ Tree Functions (btree_operations.h)

- **find_Leaf(...)**:
  - Finds the leaf node containing the entry with the specified key.
  - Utilized during insertion and retrieval operations.

- **find_Record(...)**:
  - Searches the B+ Tree for an entry with the given key.
  - Returns the record if the key is found; otherwise, returns null.

- **make_Record(...)**:
  - Creates a new record element encapsulating a RID.

- **insert_Into_Leaf(...)**:
  - Inserts a new pointer to the record and its corresponding key into a leaf node.
  - Returns the modified leaf node.

- **create_New_Tree(...)**:
  - Creates a new B+ Tree when the first element/entry is inserted.

- **create_Node(...)**:
  - Creates a new general node, adaptable as a leaf/internal/root node.

- **create_Leaf(...)**:
  - Creates a new leaf node.

- **insert_Into_Leaf_After_Splitting(...)**:
  - Inserts a new key and pointer into a leaf node, splitting it if necessary to maintain B+ Tree properties.

- **insert_Into_Node(...)**:
  - Inserts a new key and pointer into a non-leaf node without violating B+ Tree properties.

- **insert_Into_Node_After_Splitting(...)**:
  - Inserts a new key and pointer into a non-leaf node, splitting it if needed.

- **insert_Into_Parent(...)**:
  - Inserts a new node (leaf or internal) into the B+ Tree.
  - Returns the root of the tree after insertion.

- **insert_Into_New_Root(..)**:
  - Creates a new root for two subtrees and inserts the appropriate key.

- **get_Left_Index(...)**:
  - Finds the index of the parent's pointer to the node to the left of the key to be inserted.

- **adjust_Root(...)**:
  - Adjusts the root after a record deletion, ensuring B+ Tree properties are maintained.

- **merge_Nodes(...)**:
  - Combines (merges) a node that has become too small after deletion with a neighboring node.

- **redistribute_Nodes(...)**:
  - Redistributes entries between two nodes when one is too small after deletion, and its neighbor is too big.

- **delete_Entry(...)**:
  - Deletes an entry from the B+ Tree, preserving its properties.

- **delete(...)**:
  - Deletes an entry/record with the specified key.

- **remove_Entry_From_Node(...)**:
  - Removes a record with the specified key from the node.

- **getNeighborIndex(...)**:
  - Returns the index of a node's nearest neighbor (sibling) to the left if one exists.

## Initialize and Shutdown Index Manager

- **initIndexManager(...)**:
  - Initializes the index manager.
  - Calls initStorageManager(...) function of the Storage Manager.

- **shutdownIndexManager(...)**:
  - Shuts down the index manager, deallocating all allocated resources.
  - Frees up all resources/memory space used by the Index Manager.

## B+ Tree Index Related Functions

- **createBtree(...)**:
  - Creates a new B+ Tree.
  - Initializes the TreeManager structure.
  - Initializes the buffer manager and creates a buffer pool.
  - Creates a page with the specified page name "idxId" using Storage Manager.

- **openBtree(...)**:
  - Opens an existing B+ Tree stored on the specified file.
  - Retrieves the TreeManager and initializes the Buffer Pool.

- **closeBtree(...)**:
  - Closes the B+ Tree.
  - Marks all pages dirty for writing back to disk.
  - Shuts down the buffer pool and frees allocated resources.

- **deleteBtree(...)**:
  - Deletes the page file specified by "idxId" parameter using Storage Manager.

## Access Information about B+ Tree

- **getNumNodes(...)**:
  - Returns the number of nodes in the B+ Tree.
  - Data stored in the TreeManager structure.

- **getNumEntries(...)**:
  - Returns the number of entries/records/keys in the B+ Tree.
  - Data stored in the TreeManager structure.

- **getKeyType(...)**:
  - Returns the data type of the keys stored in the B+ Tree.
  - Data stored in the TreeManager structure.

## Accessing B+ Tree Functions

- **findKey(...)**:
  - Searches the B+ Tree for the specified key.
  - Stores the RID (value) for that key if found.
  - Utilizes findRecord(..) method.

- **insertKey(...)**:
  - Adds a new entry/record with the specified key and RID.
  - Checks for duplicate keys before insertion.
  - Adjusts the tree structure if needed.

- **deleteKey(...)**:
  - Deletes the entry/record with the specified key from the B+ Tree.
  - Adjusts the tree structure accordingly.

- **openTreeScan(...)**:
  - Initializes a scan to traverse entries in the B+ Tree in sorted order.
  - Initializes the ScanManager structure for the scan operation.

- **nextEntry(...)**:
  - Traverses the entries in the B+ Tree.
  - Stores record details (RID) for each entry.

- **closeTreeScan(...)**:
  - Closes the scan mechanism and frees resources.

## Debugging and Test Functions

- **printTree(...)**:
  - Prints the B+ Tree for debugging purposes.







