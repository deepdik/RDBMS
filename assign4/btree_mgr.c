#include <stdlib.h>
#include "dberror.h"
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "tables.h"
#include "btree_operations.h"


BTreeMgr * treeMgr = NULL;


/**
 * @brief Initializes the Index Manager.
 *
 * This function initializes the Index Manager by initializing the underlying storage manager.
 *
 * @param mgmtData Management data (not used).
 * @return RC_OK on success, or an error code on failure.
 */
RC initIndexManager(void *mgmtData) {
    initStorageManager();
    return RC_OK;
}


/**
 * @brief Creates a new B+ Tree.
 *
 * This function creates a new B+ Tree with the specified parameters.
 *
 * @param[in] idxId   The name of the B+ Tree.
 * @param[in] keyType The datatype of the key.
 * @param[in] n       The order of the B+ Tree.
 *
 * @return RC_OK on success, or an error code on failure.
 */

RC createBtree(char *idxId, DataType keyType, int n) {
    int maxNodes = PAGE_SIZE / sizeof(Node);

    // Return error if we cannot accommodate a B++ Tree of that order.
    if (n > maxNodes) {
        printf("\n n = %d > Max. Nodes = %d \n", n, maxNodes);
        return RC_ORDER_TOO_HIGH_FOR_PAGE;
    }

    // Initialize the members of our B+ Tree metadata structure.
    treeMgr = (BTreeMgr *) malloc(sizeof(BTreeMgr));
    treeMgr->order = n + 2;     
    treeMgr->numNodes = 0;      
    treeMgr->numEntries = 0;    
    treeMgr->root = NULL;       
    treeMgr->queue = NULL;      
    treeMgr->keyType = keyType;

    // Initialize Buffer Manager and store in our structure.
    BM_BufferPool * bm = (BM_BufferPool *) malloc(sizeof(BM_BufferPool));
    treeMgr->bufferPool = *bm;

    SM_FileHandle fHandler;
    RC result;

    char data[PAGE_SIZE];

    // Create page file. Return error code if error occurs.
    if ((result = createPageFile(idxId)) != RC_OK)
        return result;

    // Open page file.  Return error code if error occurs.
    if ((result = openPageFile(idxId, &fHandler)) != RC_OK)
        return result;

    // Write empty content to page.  Return error code if error occurs.
    if ((result = writeBlock(0, &fHandler, data)) != RC_OK)
        return result;

    // Close page file.  Return error code if error occurs.
    if ((result = closePageFile(&fHandler)) != RC_OK)
        return result;

    return (RC_OK);
}


/**
 * @brief Shuts down the Index Manager.
 *
 * This function shuts down the Index Manager by setting the tree manager pointer to NULL.
 *
 * @return RC_OK on success, or an error code on failure.
 */
RC shutdownIndexManager() {
    treeMgr = NULL;
    return RC_OK;
}



/**
 * @brief Closes the B+ Tree.
 *
 * This function closes the B+ Tree, shuts down the buffer pool, and deallocates all utilized memory space.
 *
 * @param[in] tree The B+ Tree handle.
 *
 * @return RC_OK on success, or an error code on failure.
 */
RC closeBtree(BTreeHandle *tree) {
    BTreeMgr *treeMgr = (BTreeMgr*) tree->mgmtData;
    markDirty(&treeMgr->bufferPool, &treeMgr->pageHandler);
    shutdownBufferPool(&treeMgr->bufferPool);
    free(treeMgr);
    free(tree);
    return RC_OK;
}


/**
 * @brief Deletes the B+ Tree.
 *
 * This function deletes the B+ Tree by deleting the associated page with it.
 *
 * @param[in] idxId The identifier of the B+ Tree.
 *
 * @return RC_OK on success, or an error code on failure.
 */
RC deleteBtree(char *idxId) {
    RC result;
    if ((result = destroyPageFile(idxId)) != RC_OK)
        return result;
    return RC_OK;
}



/**
 * @brief Inserts a new entry/record with the specified key and RID into the B+ Tree.
 *
 * This function inserts a new entry/record with the specified key and RID into the B+ Tree.
 * If the key already exists in the tree, the insertion fails and returns an error code.
 *
 * @param[in] tree The handle to the B+ Tree.
 * @param[in] key The key to be inserted into the B+ Tree.
 * @param[in] rid The Record IDentifier (RID) associated with the key to be inserted.
 *
 * @return RC_OK if the insertion is successful; RC_IM_KEY_ALREADY_EXISTS if the key already exists in the B+ Tree.
 */
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    // Retrieve B+ Tree's metadata information.
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;
    NodeData *pointer;
    Node *leaf;

    int bTOrder = treeMgr->order;

    // Check if a record with the specified key already exists.
    if (find_Record(treeMgr->root, key) != NULL) {
        printf("\n insertKey :: KEY EXISTS");
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    // Create a new record (NodeData) for the value RID.
    pointer = make_Record(&rid);

    // If the tree doesn't exist yet, create a new tree.
    if (treeMgr->root == NULL) {
        treeMgr->root = create_New_Tree(treeMgr, key, pointer);
        //printTree(tree);
        return RC_OK;
    }

    // If the tree already exists, find a leaf where the key can be inserted.
    leaf = find_Leaf_Node(treeMgr->root, key);

    if (leaf->num_keys < bTOrder - 1) {
        // If the leaf has room for the new key, insert the new key into that leaf.
        leaf = insert_Into_Leaf(treeMgr, leaf, key, pointer);
    } else {
        // If the leaf does not have room for the new key, split the leaf and then insert the new key into that leaf.
        treeMgr->root = insert_Into_Leaf_After_Splitting(treeMgr, leaf, key, pointer);
    }

    // Print the B+ Tree for debugging purposes.
    return RC_OK;
}



/**
 * @brief Opens an existing B+ Tree.
 *
 * This function opens an existing B+ Tree from the specified page.
 *
 * @param[out] tree  The pointer to the B+ Tree handle.
 * @param[in]  idx The name of the B+ Tree.
 *
 * @return RC_OK on success, or an error code on failure.
 */
RC openBtree(BTreeHandle **tree, char *idx) {
    // Allocate memory for the B+ Tree handle
    *tree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
    
    // Assign the metadata structure to the B+ Tree handle
    (*tree)->mgmtData = treeMgr; // Assuming 'treeMgr' is a global variable or defined elsewhere

    // Initialize a Buffer Pool using the Buffer Manager
    RC result = initBufferPool(&treeMgr->bufferPool, idx, 1000, RS_FIFO, NULL);
    
    // Check if the Buffer Pool initialization was successful
    if (result == RC_OK) {
        // If successful, return OK status
        return RC_OK;
    }
    // Otherwise, return the result indicating the error
    return result;
}






/**
 * @brief Searches the B+ Tree for the specified key and retrieves the corresponding RID.
 *
 * This function searches the B+ Tree for the specified key. If the key is found, it retrieves
 * the corresponding RID (Record IDentifier) and stores it in the memory location pointed to by
 * the "result" parameter.
 *
 * @param[in] tree The handle to the B+ Tree.
 * @param[in] key The key to search for in the B+ Tree.
 * @param[out] result Pointer to the memory location where the RID will be stored if the key is found.
 *
 * @return RC_OK if the key is found; RC_IM_KEY_NOT_FOUND if the key is not found in the B+ Tree.
 */
// Function to find a key in the B+ tree
extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    // Retrieve B+ Tree's metadata information.
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;

    // Search the B+ Tree for the specified key.
    NodeData *record = find_Record(treeMgr->root, key);

    // If the returned record is NULL, then the key does not exist in the B+ Tree.
    if (record == NULL) {
        // Return an error code indicating that the key was not found.
        return RC_IM_KEY_NOT_FOUND;
    }

    // If the record is not NULL, store the value (RID) in the memory location pointed to by "result".
    *result = record->rid;
    
    // Return OK status, indicating that the key was found and the corresponding value (RID) was stored.
    return RC_OK;
}



/**
 * @brief Retrieves the number of nodes present in the B+ Tree.
 *
 * This function retrieves the number of nodes present in the B+ Tree and stores the result
 * in the memory location pointed to by the "result" parameter.
 *
 * @param[in] tree The B+ Tree handle.
 * @param[out] result Pointer to the memory location where the result will be stored.
 *
 * @return RC_OK on success.
 */
RC getNumNodes(BTreeHandle *tree, int *result) {
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;
    *result = treeMgr->numNodes;
    return RC_OK;
}


/**
 * @brief Retrieves the number of entries (i.e., keys) present in the B+ Tree.
 *
 * This function retrieves the number of entries present in the B+ Tree and stores the result
 * in the memory location pointed to by the "result" parameter.
 *
 * @param[in] tree The B+ Tree handle.
 * @param[out] result Pointer to the memory location where the result will be stored.
 *
 * @return RC_OK on success.
 */
RC getNumEntries(BTreeHandle *tree, int *result) {
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;
    *result = treeMgr->numEntries;
    return RC_OK;
}

/**
 * @brief Retrieves the datatype of the keys in the B+ Tree.
 *
 * This function retrieves the datatype of the keys in the B+ Tree and stores the result
 * in the memory location pointed to by the "result" parameter.
 *
 * @param[in] tree The B+ Tree handle.
 * @param[out] result Pointer to the memory location where the result will be stored.
 *
 * @return RC_OK on success.
 */
RC getKeyType(BTreeHandle *tree, DataType *result) {
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;
    *result = treeMgr->keyType;
    return RC_OK;
}

/**
 * @brief Deletes the entry/record with the specified key in the B+ Tree.
 *
 * This function deletes the entry/record with the specified "key" in the B+ Tree.
 *
 * @param[in] tree The B+ Tree handle.
 * @param[in] key The key of the entry to be deleted.
 *
 * @return RC_OK on success.
 */
RC deleteKey(BTreeHandle *tree, Value *key) {
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;
    treeMgr->root = delete(treeMgr, key);
    return RC_OK;
}


/**
 * @brief Prints the B+ Tree.
 *
 * This function prints the B+ Tree in a readable format.
 *
 * @param[in] tree The B+ Tree handle.
 *
 * @return NULL on success.
 */
extern char *printTree(BTreeHandle *tree) {
    // Retrieve B+ Tree's metadata information.
    BTreeMgr *treeMgr = (BTreeMgr *) tree->mgmtData;

    // Initialize variables
    Node *nd = NULL;
    int i = 0;
    int rnk = 0;
    int newRank = 0;

    // Check if the root of the tree is NULL
    if (treeMgr->root == NULL) {
        // If the tree is empty, return NULL
        return NULL;
    }

    // Initialize queue for level-order traversal
    treeMgr->queue = NULL;
    // Enqueue the root node to start traversal
    enqueue(treeMgr, treeMgr->root);

    // Level-order traversal of the B+ tree
    while (treeMgr->queue != NULL) {
        // Dequeue a node for processing
        nd = dequeue(treeMgr);

        // Check if the node's rank (level in the tree) has changed
        if (nd->parent != NULL && nd == nd->parent->ptr[0]) {
            newRank = pathToRoot(treeMgr->root, nd);
            if (newRank != rnk) {
                rnk = newRank;
                // Print a new line for clarity when changing levels
                printf("\n");
            }
        }

        // Print keys and associated RIDs
        for (i = 0; i < nd->num_keys; i++) {
            // Print key value based on the data type of the key
            switch (treeMgr->keyType) {
                case DT_INT:
                    printf("%d ", nd->keys[i]->v.intV);
                    break;
                case DT_FLOAT:
                    printf("%.02f ", nd->keys[i]->v.floatV);
                    break;
                case DT_STRING:
                    printf("%s ", nd->keys[i]->v.stringV);
                    break;
                case DT_BOOL:
                    printf("%d ", nd->keys[i]->v.boolV);
                    break;
            }
            // Print associated RID (page number and slot number)
            printf("(%d - %d) ", ((NodeData *) nd->ptr[i])->rid.page, ((NodeData *) nd->ptr[i])->rid.slot);
        }

        // Enqueue child ptr for further traversal (if not a leaf node)
        if (!nd->is_leaf)
            for (i = 0; i <= nd->num_keys; i++)
                enqueue(treeMgr, nd->ptr[i]);

        // Print a delimiter to separate nodes at the same level
        printf("| ");
    }
    // Print a new line after completing tree traversal
    printf("\n");

    // Return NULL indicating successful printing
    return NULL;
}




/**
 * @brief Initializes the scan mechanism to traverse the entries in the B+ Tree.
 *
 * This function initializes the scan to traverse the entries in the B+ Tree.
 *
 * @param[in] tree The B+ Tree handle.
 * @param[out] handle The handle to the initialized scan.
 *
 * @return RC_OK if successful, or RC_NO_RECORDS_TO_SCAN if the tree is empty.
 */
// Function to open a scan on the B+ tree
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **hndle) {
    // Retrieve B+ Tree's metadata information.
    BTreeMgr *treeMgr = (BTreeMgr *)tree->mgmtData;

    // Allocate memory for B+ Tree Scan's metadata.
    ScanMgr *scan_meta = malloc(sizeof(ScanMgr));

    // Allocate memory for the scan handle.
    *hndle = malloc(sizeof(BT_ScanHandle));

    // Start traversal from the root node.
    Node *node = treeMgr->root;

    // Check if the tree is empty.
    if (treeMgr->root == NULL) {
        // Return an error if the tree is empty.
        return RC_NO_RECORDS_TO_SCAN;
    } else {
        // Traverse to the leftmost leaf node.
        while (!node->is_leaf)
            node = node->ptr[0];

        // Initialize the scan's metadata information.
        scan_meta->keyIndex = 0; // Initialize key index to start from the beginning
        scan_meta->totalKeys = node->num_keys; // Total keys in the leaf node
        scan_meta->node = node; // Current leaf node being scanned
        scan_meta->order = treeMgr->order; // B+ tree order
        (*hndle)->mgmtData = scan_meta; // Assign scan metadata to the scan handle
    }
    // Return OK status, indicating successful scan initialization.
    return RC_OK;
}



/**
 * @brief Traverse to the next entry in the B+ Tree scan.
 *
 * This function moves the scan to the next entry in the B+ Tree and retrieves the record details (RID).
 *
 * @param[in] handle The scan handle.
 * @param[out] result Pointer to store the retrieved RID.
 *
 * @return RC_OK if successful, or RC_IM_NO_MORE_ENTRIES if there are no more entries to scan.
 */
// Function to retrieve the next entry in the B+ tree scan
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    // Retrieve B+ Tree Scan's metadata information.
    ScanMgr *scan_meta = (ScanMgr *)handle->mgmtData;

    // Retrieve all the necessary information.
    int keyIndex = scan_meta->keyIndex; // Current key index in the leaf node
    int totalKeys = scan_meta->totalKeys; // Total keys in the leaf node
    int bTOrder = scan_meta->order; // B+ tree order
    RID rid;

    Node *node = scan_meta->node; // Current leaf node being scanned

    // Return error if the current node is empty i.e., NULL.
    if (node == NULL) {
        return RC_IM_NO_MORE_ENTRIES;
    }

    if (keyIndex < totalKeys) {
        // If the current key entry is present on the same leaf node.
        rid = ((NodeData *)node->ptr[keyIndex])->rid; // Retrieve RID associated with the current key
        scan_meta->keyIndex++; // Move to the next key entry
    } else {
        // If all the entries on the leaf node have been scanned, move to the next node.
        if (node->ptr[bTOrder - 1] != NULL) {
            // Move to the next node in the leaf level
            node = node->ptr[bTOrder - 1];
            scan_meta->keyIndex = 1; // Reset key index to start from the beginning of the new node
            scan_meta->totalKeys = node->num_keys; // Update total keys for the new node
            scan_meta->node = node; // Update current node being scanned
            rid = ((NodeData *)node->ptr[0])->rid; // Retrieve RID associated with the first key in the new node
        } else {
            // If no next node, it means there are no more entries to be scanned.
            return RC_IM_NO_MORE_ENTRIES;
        }
    }

    // Store the record/RID.
    *result = rid;
    
    // Return OK status, indicating successful retrieval of the next entry.
    return RC_OK;
}





/**
 * @brief Closes the scan mechanism and frees up resources.
 *
 * This function closes the scan mechanism and releases any allocated resources.
 *
 * @param[in] handle The scan handle.
 *
 * @return RC_OK on success.
 */
extern RC closeTreeScan(BT_ScanHandle *handle) {
    handle->mgmtData = NULL;
    free(handle);
    return RC_OK;
}


