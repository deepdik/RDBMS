#include <stdlib.h>
#include "btree_operations.h"
#include "dt.h"
#include "string.h"


/**
 * @brief Creates a new tree when the first element (NodeData) is inserted.
 *
 * This function creates a new tree with a single leaf node when the first element is inserted.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param key Pointer to the value representing the key of the inserted element.
 * @param pointer Pointer to the NodeData representing the inserted element.
 * @return Pointer to the root node of the newly created tree.
 */
Node *create_New_Tree(BTreeMgr *treeMgr, Value *key, NodeData *pointer) {
    // Create a new leaf node to serve as the root
    Node *root = create_Leaf(treeMgr);
    int bTOrder = treeMgr->order; // B+ tree order

    // Initialize the root node with the provided key and pointer
    root->keys[0] = key; // Assign the provided key to the root
    root->ptr[0] = pointer; // Assign the provided pointer to the root
    root->ptr[bTOrder - 1] = NULL; // Set the last pointer to NULL
    root->parent = NULL; // The root has no parent
    root->num_keys++; // Increment the number of keys in the root node

    // Update B+ tree statistics in treeMgr
    treeMgr->numEntries++; // Increment the total number of entries in the tree

    // Return the newly created root node
    return root;
}

/**
 * @brief Creates a new record (NodeData) to hold the value corresponding to a given key.
 *
 * This function allocates memory for a new record structure and initializes it with the provided RID.
 *
 * @param rid Pointer to the record identifier (RID) structure.
 * @return Pointer to the newly created record structure.
 */
NodeData *make_Record(RID *rid) {
    // Allocate memory for the new NodeData record
    NodeData *newRecord = (NodeData *)malloc(sizeof(NodeData));

    // Check if memory allocation was successful
    if (newRecord == NULL) {
        // If allocation failed, print an error message and exit with an error code
        perror("Error creating NodeData.");
        exit(RC_INSERT_ERROR);
    } else {
        // If allocation succeeded, initialize the record with the given RID
        newRecord->rid.page = rid->page; // Assign page number
        newRecord->rid.slot = rid->slot; // Assign slot number
    }

    // Return the newly created NodeData record
    return newRecord;
}


/**
 * @brief Inserts a new pointer to the record (NodeData) and its corresponding key into a leaf node.
 *
 * This function inserts a new pointer to the record (NodeData) and its corresponding key into a leaf node
 * and returns the altered leaf node.
 *
 * @param treeMgr Pointer to the B-tree manager structure.
 * @param leaf Pointer to the leaf node where the insertion will be performed.
 * @param key Pointer to the value representing the key to be inserted.
 * @param pointer Pointer to the NodeData representing the record to be inserted.
 * @return Pointer to the altered leaf node.
 */

Node *insert_Into_Leaf(BTreeMgr *treeMgr, Node *leaf, Value *key, NodeData *ptr) {
    int i, insertionPtr;

    // Increment the number of entries in the tree
    treeMgr->numEntries++;

    // Find the position to insert the new key and ptr
    insertionPtr = 0;
    while (insertionPtr < leaf->num_keys && is_Less(leaf->keys[insertionPtr], key))
        insertionPtr++;

    // Shift keys and ptr to make space for the new entry
    for (i = leaf->num_keys; i > insertionPtr; i--) {
        leaf->keys[i] = leaf->keys[i - 1]; // Shift keys to the right
        leaf->ptr[i] = leaf->ptr[i - 1]; // Shift ptr to the right
    }

    // Insert the new key and ptr at the appropriate position
    leaf->keys[insertionPtr] = key; // Insert the new key
    leaf->ptr[insertionPtr] = ptr; // Insert the new pointer
    leaf->num_keys++; // Increment the number of keys in the leaf node

    // Return the updated leaf node
    return leaf;
}




/**
 * @brief Inserts a new node (leaf or internal node) into the B+ tree's parent node.
 *
 * This function inserts a new node (either a leaf or an internal node) into the parent node of the B+ tree.
 * It checks if the parent node has enough space to accommodate the new node. If not, it splits the parent node.
 * Returns the root of the tree after insertion.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param left Pointer to the left child node.
 * @param key Pointer to the value representing the key to be inserted.
 * @param right Pointer to the right child node.
 * @return Pointer to the root of the tree after insertion.
 */
Node *insert_Into_Parent(BTreeMgr *treeMgr, Node *left, Value *key, Node *right) {
    int leftIndex; // Index of the left child in the parent node
    Node *parent = left->parent; // Parent node of the left child
    int bTreeOrder = treeMgr->order; // B+ tree order

    // Check if it is the new root.
    if (parent == NULL)
        return insert_Into_NewRoot(treeMgr, left, key, right); // Insert into new root

    // Find the index of the left child in the parent node.
    leftIndex = get_Left_Index(parent, left);

    // If the new key can be accommodated in the node.
    if (parent->num_keys < bTreeOrder - 1) {
        return insert_Into_Node(treeMgr, parent, leftIndex, key, right); // Insert into the parent node
    }

    // If the node cannot accommodate the new key, split the node while preserving B+ tree properties.
    return insert_Into_Node_After_Splitting(treeMgr, parent, leftIndex, key, right); // Split and insert into parent node
}



/**
 * @brief Inserts a new key and pointer to a new record (NodeData) into a leaf node, causing it to split if necessary.
 *
 * This function inserts a new key and pointer to a new record (NodeData) into a leaf node. If the insertion causes
 * the leaf node to exceed the tree's order, it splits the leaf node in half.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param leaf Pointer to the leaf node where the insertion will be performed.
 * @param key Pointer to the value representing the key to be inserted.
 * @param pointer Pointer to the NodeData representing the record to be inserted.
 * @return Pointer to the new leaf node after splitting.
 */

Node *insert_Into_Leaf_After_Splitting(BTreeMgr *treeMgr, Node *leaf, Value *key, NodeData *ptr) {
    Node *newLeaf; // New leaf node to store split keys and ptr
    Value **tempKeys; // Temporary array to hold keys during splitting
    void **tmpPtrs; // Temporary array to hold ptr during splitting
    int insertionIndex, split, i, j;

    // Create a new leaf node to store split keys and ptr
    newLeaf = create_Leaf(treeMgr);
    int bTOrder = treeMgr->order; // B+ tree order

    // Allocate memory for temporary arrays
    tempKeys = malloc(bTOrder * sizeof(Value));
    if (tempKeys == NULL) {
        perror("Temporary keys array.");
        exit(RC_INSERT_ERROR);
    }

    tmpPtrs = malloc(bTOrder * sizeof(void *));
    if (tmpPtrs == NULL) {
        perror("Temporary ptr array.");
        exit(RC_INSERT_ERROR);
    }

    // Find the position to insert the new key and ptr
    insertionIndex = 0;
    while (insertionIndex < bTOrder - 1 && is_Less(leaf->keys[insertionIndex], key))
        insertionIndex++;

    // Populate temporary arrays with keys and ptr
    for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
        if (j == insertionIndex)
            j++;
        tempKeys[j] = leaf->keys[i];
        tmpPtrs[j] = leaf->ptr[i];
    }

    // Insert the new key and ptr into temporary arrays
    tempKeys[insertionIndex] = key;
    tmpPtrs[insertionIndex] = ptr;

    // Reset the number of keys in the original leaf node
    leaf->num_keys = 0;

    // Calculate the split point
    if ((bTOrder - 1) % 2 == 0)
        split = (bTOrder - 1) / 2;
    else
        split = (bTOrder - 1) / 2 + 1;

    // Copy keys and ptr to the original leaf node
    for (i = 0; i < split; i++) {
        leaf->ptr[i] = tmpPtrs[i];
        leaf->keys[i] = tempKeys[i];
        leaf->num_keys++;
    }

    // Copy keys and ptr to the new leaf node
    for (i = split, j = 0; i < bTOrder; i++, j++) {
        newLeaf->ptr[j] = tmpPtrs[i];
        newLeaf->keys[j] = tempKeys[i];
        newLeaf->num_keys++;
    }

    // Free temporary arrays
    free(tmpPtrs);
    free(tempKeys);

    // Adjust ptr between leaf nodes
    newLeaf->ptr[bTOrder - 1] = leaf->ptr[bTOrder - 1];
    leaf->ptr[bTOrder - 1] = newLeaf;

    // Set remaining ptr to NULL
    for (i = leaf->num_keys; i < bTOrder - 1; i++)
        leaf->ptr[i] = NULL;
    for (i = newLeaf->num_keys; i < bTOrder - 1; i++)
        newLeaf->ptr[i] = NULL;

    // Update parent-child relationship for the new leaf node
    newLeaf->parent = leaf->parent;

    // Get the first key from the new leaf node
    Value *newKey = newLeaf->keys[0];

    // Increment the total number of entries in the tree
    treeMgr->numEntries++;

    // Insert the new key into the parent node
    return insert_Into_Parent(treeMgr, leaf, newKey, newLeaf);
}


/**
 * @brief Inserts a new key and pointer into a node, causing the node's size to exceed the order and splitting it into two.
 *
 * This function inserts a new key and pointer into a node, causing the node's size to exceed the order.
 * It then splits the node into two and returns the resulting new node.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param oldNode Pointer to the node where the insertion will be performed.
 * @param leftIndex Index of the left pointer of the insertion point.
 * @param key Pointer to the value representing the key to be inserted.
 * @param right Pointer to the right child node to be inserted.
 * @return Pointer to the new node after splitting.
 */
// Function to insert a new key and pointer into a node after splitting it
Node *insert_Into_Node_After_Splitting(BTreeMgr *treeMgr, Node *oldNode, int leftIndex, Value *key, Node *right) {
    int i, j, split;
    Node *newNode, *child;
    Value **tempKeys;
    Node **tmpptr;

    int bTreeOrder = treeMgr->order; // B+ tree order

    // Allocate memory for temporary arrays
    tmpptr = malloc((bTreeOrder + 1) * sizeof(Node *));
    if (tmpptr == NULL) {
        perror("Temporary ptr array for splitting nodes.");
        exit(RC_INSERT_ERROR);
    }
    tempKeys = malloc(bTreeOrder * sizeof(Value *));
    if (tempKeys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(RC_INSERT_ERROR);
    }

    // Populate temporary arrays
    for (i = 0, j = 0; i < oldNode->num_keys + 1; i++, j++) {
        if (j == leftIndex + 1)
            j++;
        tmpptr[j] = oldNode->ptr[i];
    }

    for (i = 0, j = 0; i < oldNode->num_keys; i++, j++) {
        if (j == leftIndex)
            j++;
        tempKeys[j] = oldNode->keys[i];
    }

    tmpptr[leftIndex + 1] = right;
    tempKeys[leftIndex] = key;

    // Determine the split index
    if ((bTreeOrder - 1) % 2 == 0)
        split = (bTreeOrder - 1) / 2;
    else
        split = (bTreeOrder - 1) / 2 + 1;

    // Create a new node for the right half of keys and ptr
    newNode = create_Node(treeMgr);
    oldNode->num_keys = 0;

    // Copy keys and ptr to the old node for the left half
    for (i = 0; i < split - 1; i++) {
        oldNode->ptr[i] = tmpptr[i];
        oldNode->keys[i] = tempKeys[i];
        oldNode->num_keys++;
    }
    oldNode->ptr[i] = tmpptr[i];
    Value *kPrime = tempKeys[split - 1];

    // Copy keys and ptr to the new node for the right half
    for (++i, j = 0; i < bTreeOrder; i++, j++) {
        newNode->ptr[j] = tmpptr[i];
        newNode->keys[j] = tempKeys[i];
        newNode->num_keys++;
    }
    newNode->ptr[j] = tmpptr[i];

    // Free temporary arrays
    free(tmpptr);
    free(tempKeys);

    // Update parent ptr for children
    newNode->parent = oldNode->parent;
    for (i = 0; i <= newNode->num_keys; i++) {
        child = newNode->ptr[i];
        child->parent = newNode;
    }

    // Insert a new key into the parent node
    treeMgr->numEntries++;
    return insert_Into_Parent(treeMgr, oldNode, kPrime, newNode);
}


/**
 * @brief Inserts a new key and pointer into a node where they can fit without violating the B+ tree properties.
 *
 * This function inserts a new key and pointer into a node into which they can fit without violating the B+ tree properties.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param parent Pointer to the parent node where the insertion will be performed.
 * @param leftIndex Index of the left pointer of the insertion point.
 * @param key Pointer to the value representing the key to be inserted.
 * @param right Pointer to the right child node to be inserted.
 * @return Pointer to the root of the tree after insertion.
 */

Node *insert_Into_Node(BTreeMgr *treeMgr, Node *parent, int leftIndex, Value *key, Node *right) {
    int i;

    // Shift ptr and keys to the right to make space for the new key and pointer
    for (i = parent->num_keys; i > leftIndex; i--) {
        parent->ptr[i + 1] = parent->ptr[i]; // Shift ptr to the right
        parent->keys[i] = parent->keys[i - 1]; // Shift keys to the right
    }

    // Insert the new key and pointer
    parent->ptr[leftIndex + 1] = right; // Insert pointer
    parent->keys[leftIndex] = key; // Insert key
    parent->num_keys++; // Increment the number of keys in the node

    // Return the root of the tree
    return treeMgr->root;
}



/**
 * @brief Finds the index of the parent's pointer to the node to the left of the key to be inserted.
 *
 * This function is used in `insert_Into_Parent` to find the index of the parent's pointer to the node
 * to the left of the key to be inserted.
 *
 * @param parent Pointer to the parent node.
 * @param left Pointer to the left child node.
 * @return The index of the parent's pointer to the left child node.
 */
int get_Left_Index(Node *parent, Node *left) {
    int leftIndex = 0;
    while (leftIndex <= parent->num_keys && parent->ptr[leftIndex] != left)
        leftIndex++;
    return leftIndex;
}




/**
 * @brief Creates a new root for two subtrees and inserts the appropriate key into the new root.
 *
 * This function creates a new root node for two subtrees and inserts the appropriate key into the new root.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param left Pointer to the left child node.
 * @param key Pointer to the value representing the key to be inserted.
 * @param right Pointer to the right child node.
 * @return Pointer to the root of the tree after insertion.
 */
Node *insert_Into_NewRoot(BTreeMgr *treeMgr, Node *left, Value *key, Node *right) {
    // Create a new root node
    Node *root = create_Node(treeMgr);

    // Insert key and ptr into the new root
    root->keys[0] = key; // Insert key
    root->ptr[0] = left; // Insert left pointer
    root->ptr[1] = right; // Insert right pointer
    root->num_keys++; // Increment the number of keys in the root node
    root->parent = NULL; // Set parent of root to NULL
    left->parent = root; // Update parent of left child
    right->parent = root; // Update parent of right child

    // Return the new root node
    return root;
}


/**
 * @brief Creates a new general node, adaptable to serve as either a leaf or an internal node.
 *
 * This function creates a new general node, which can be adapted to serve as either a leaf or an internal node in the B+ tree.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @return Pointer to the newly created node.
 */
// Function to create a new node in the B+ tree
Node *create_Node(BTreeMgr *treeMgr) {
    // Increment the count of nodes in the tree
    treeMgr->numNodes++;

    // Obtain the order of the B+ tree
    int bTreeOrder = treeMgr->order;

    // Allocate memory for the new node
    Node *newNode = malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Error creating node.");
        exit(RC_INSERT_ERROR);
    }

    // Allocate memory for the keys array of the new node
    newNode->keys = malloc((bTreeOrder - 1) * sizeof(Value *));
    if (newNode->keys == NULL) {
        perror("Error creating node keys array.");
        exit(RC_INSERT_ERROR);
    }

    // Allocate memory for the ptr array of the new node
    newNode->ptr = malloc(bTreeOrder * sizeof(void *));
    if (newNode->ptr == NULL) {
        perror("Error creating node ptr array.");
        exit(RC_INSERT_ERROR);
    }

    // Initialize the properties of the new node
    newNode->is_leaf = false;
    newNode->num_keys = 0;
    newNode->parent = NULL;
    newNode->next = NULL;

    return newNode;
}


/**
 * @brief Creates a new leaf node.
 *
 * This function creates a new leaf node by calling the create_Node function and sets its is_leaf attribute to true.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @return Pointer to the newly created leaf node.
 */
Node *create_Leaf(BTreeMgr *treeMgr) {
    // Create a new node using the create_Node function
    Node *leaf = create_Node(treeMgr);

    // Set the node as a leaf node
    leaf->is_leaf = true;

    return leaf;
}



/**
 * @brief Searches for the key from the root to the leaf.
 *
 * This function searches for the key from the root to the leaf in the B+ tree.
 * It returns the leaf node containing the given key.
 *
 * @param root Pointer to the root node of the tree.
 * @param key Pointer to the value representing the key to be searched.
 * @return Pointer to the leaf node containing the given key.
 */
// Function to find the leaf node containing the given key in the B+ tree
Node *find_Leaf_Node(Node *root, Value *key) {
    int i = 0;
    Node *c = root;

    // If the tree is empty, return NULL
    if (c == NULL) {
        return c;
    }

    // Traverse from root to leaf
    while (!c->is_leaf) {
        i = 0;
        // Find the appropriate child index for the key
        while (i < c->num_keys) {
            // Compare the key with the keys in the current node
            if (is_Greater(key, c->keys[i]) || is_Equal(key, c->keys[i])) {
                i++;
            } else {
                break;
            }
        }
        // Move to the appropriate child node
        c = (Node *)c->ptr[i];
    }

    // Return the leaf node containing the given key
    return c;
}



/**
 * @brief Finds and returns the record (NodeData) to which a key refers.
 *
 * This function finds and returns the record (NodeData) to which a key refers in the B+ tree.
 * It searches for the key from the root to the leaf and then within the leaf node to find the corresponding record.
 *
 * @param root Pointer to the root node of the tree.
 * @param key Pointer to the value representing the key to be searched.
 * @return Pointer to the record (NodeData) corresponding to the given key, or NULL if not found.
 */
NodeData *find_Record(Node *root, Value *key) {
    int i = 0;
    
    // Find the leaf node containing the key
    Node *c = find_Leaf_Node(root, key);

    // If the leaf node is not found or the tree is empty, return NULL
    if (c == NULL)
        return NULL;

    // Search for the key within the leaf node
    for (i = 0; i < c->num_keys; i++) {
        // If the key is found, break out of the loop
        if (is_Equal(c->keys[i], key))
            break;
    }

    // If the key is found, return the corresponding record (NodeData)
    if (i == c->num_keys)
        return NULL; // Key not found
    else
        return (NodeData *)c->ptr[i];
}




/**
 * @brief Removes a record with the specified key from the specified node.
 *
 * This function removes a record with the specified key from the specified node in the B+ tree.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param n Pointer to the node from which the record is to be removed.
 * @param key Pointer to the value representing the key to be removed.
 * @param pointer Pointer to the record (NodeData) associated with the key.
 * @return Pointer to the updated node after removing the entry.
 */
// Function to remove an entry (key-pointer pair) from a node in the B+ tree
Node *remove_Entry_From_Node(BTreeMgr *treeMgr, Node *n, Value *key, Node *pointer) {
    int i, num_ptr;
    int bTreeOrder = treeMgr->order;

    // Find the index of the key to be removed and shift other keys accordingly
    i = 0;
    while (!is_Equal(n->keys[i], key))
        i++;

    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Determine the number of ptr
    num_ptr = n->is_leaf ? n->num_keys : n->num_keys + 1;

    // Find the index of the pointer to be removed and shift other ptr accordingly
    i = 0;
    while (n->ptr[i] != pointer)
        i++;

    for (++i; i < num_ptr; i++)
        n->ptr[i - 1] = n->ptr[i];

    // Decrement the number of keys
    n->num_keys--;

    // Decrement the total number of entries in the tree
    treeMgr->numEntries--;

    // Set the unused ptr to NULL for tidiness
    if (n->is_leaf) {
        for (i = n->num_keys; i < bTreeOrder - 1; i++)
            n->ptr[i] = NULL;
    } else {
        for (i = n->num_keys + 1; i < bTreeOrder; i++)
            n->ptr[i] = NULL;
    }

    return n;
}



/**
 * @brief Returns the index of a node's nearest neighbor (sibling) to the left if one exists.
 *
 * This function returns the index of a node's nearest neighbor (sibling) to the left if one exists.
 * If the node is the leftmost child, it returns -1 to signify this special case.
 *
 * @param n Pointer to the node for which the left neighbor index is to be determined.
 * @return The index of the node's nearest neighbor to the left, or -1 if the node is the leftmost child.
 */
// Function to find the index of the given node within its parent's ptr array
int getNeighborIndex(Node *n) {
    int i;

    // Iterate over the ptr array of the parent node to find the given node
    for (i = 0; i <= n->parent->num_keys; i++) {
        // If the current pointer matches the given node, return its index
        if (n->parent->ptr[i] == n)
            return i - 1;
    }

    // Error state: No matching pointer found in the parent node
    printf("Error: Nonexistent pointer to the node in its parent.\n");
    printf("Node Address: %#lx\n", (unsigned long)n);
    exit(RC_ERROR); // Exit the program with an error code
}

/**
 * @brief Adjusts the root after a record has been deleted from the B+ Tree.
 *
 * This function adjusts the root after a record has been deleted from the B+ Tree.
 * If the root node becomes empty after the deletion, this function either promotes the first (only) child
 * as the new root (if the root is an internal node), or sets the new root to NULL (if the root is a leaf).
 *
 * @param root Pointer to the root node of the tree.
 * @return Pointer to the adjusted root node after deletion.
 */
Node *adjustRoot(Node *root) {
    Node *newRoot;

    // If the root is not empty, do nothing and return it
    if (root->num_keys > 0)
        return root;

    // If the root is not empty and it has a child, promote the first (only) child as the new root
    if (!root->is_leaf) {
        newRoot = root->ptr[0];
        newRoot->parent = NULL; // Update the parent pointer of the new root to NULL
    } else {
        // If the root is not empty and it is a leaf (has no children), then the whole tree is empty
        newRoot = NULL;
    }

    // Deallocate memory space for the old root node
    free(root->keys);
    free(root->ptr);
    free(root);

    // Return the new root
    return newRoot;
}



/**
 * @brief Combines a node that has become too small after deletion with a neighboring node that can accept the additional entries without exceeding the maximum.
 *
 * This function combines a node that has become too small after deletion with a neighboring node that can accept the additional entries without exceeding the maximum.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param n Pointer to the node that has become too small after deletion.
 * @param neighbor Pointer to the neighboring node that can accept additional entries.
 * @param neighbor_index The index of the neighbor node relative to the current node.
 * @param k_prime Pointer to the key that separates the ptr to n and neighbor in the parent node.
 * @return Pointer to the root node after merging and adjusting the tree.
 */
// Function to merge a node with its neighbor
Node *merge_Nodes(BTreeMgr *treeMgr, Node *n, Node *neighbour, int neighbor_index, Value *k_prime) {
    int i, j, neighbor_insertion_index, n_end;
    Node *tmp;
    int bTreeOrder = treeMgr->order; // B+ tree order

    // Swap neighbor with our node if our node is on the extreme left and the neighbor is to its right.
    if (neighbor_index == -1) {
        tmp = n;
        n = neighbour;
        neighbour = tmp;
    }

    // Our starting point in the neighbor for copying keys and ptr from our node.
    // Our node and the neighbor have swapped places in the special case of our node being a leftmost child.
    neighbor_insertion_index = neighbour->num_keys;

    // If our node is a non-leaf node, we append k_prime and the following pointer.
    // Also, we append all ptr and keys from the neighbor.
    if (!n->is_leaf) {
        neighbour->keys[neighbor_insertion_index] = k_prime;
        neighbour->num_keys++;

        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbour->keys[i] = n->keys[j];
            neighbour->ptr[i] = n->ptr[j];
            neighbour->num_keys++;
            n->num_keys--;
        }

        neighbour->ptr[i] = n->ptr[j];

        // We point all children to the same parent.
        for (i = 0; i < neighbour->num_keys + 1; i++) {
            tmp = (Node *)neighbour->ptr[i];
            tmp->parent = neighbour;
        }
    } else {
        // In a leaf, we append the keys and ptr of our node to the neighbor.
        // We set the neighbor's last pointer to point to what had been our node's right neighbor.
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbour->keys[i] = n->keys[j];
            neighbour->ptr[i] = n->ptr[j];
            neighbour->num_keys++;
        }
        neighbour->ptr[bTreeOrder - 1] = n->ptr[bTreeOrder - 1];
    }

    // We update the root after deleting the entry from the parent node.
    treeMgr->root = delete_Entry(treeMgr, n->parent, k_prime, n);

    // We deallocate memory space for the merged node.
    free(n->keys);
    free(n->ptr);
    free(n);

    // We return the updated root
    return treeMgr->root;
}





/**
 * @brief Deletes an entry from the B+ tree.
 *
 * This function removes the record associated with the specified key and pointer from the leaf node,
 * and then adjusts the tree structure as necessary to maintain B+ tree properties.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param node Pointer to the node containing the entry to be deleted.
 * @param key Pointer to the key of the entry to be deleted.
 * @param pointer Pointer to the entry to be deleted.
 * @return Pointer to the root node after deletion.
 */
// Function to delete a key entry from a node in the B+ tree
Node *delete_Entry(BTreeMgr *treeMgr, Node *node, Value *key, void *pointer) {
    int minKeys;
    Node *neighbour;
    int neighborIndex;
    int kPrimeIndex;
    int capacity;
    int bTreeOrder = treeMgr->order;

    // Remove the key and pointer from the node
    node = remove_Entry_From_Node(treeMgr, node, key, pointer);

    // If the node is the root, adjust the root as necessary
    if (node == treeMgr->root)
        return adjustRoot(treeMgr->root);

    // Determine the minimum allowable size of a node after deletion based on B+ tree properties
    if (node->is_leaf) {
        minKeys = (bTreeOrder - 1) % 2 == 0 ? (bTreeOrder - 1) / 2 : (bTreeOrder - 1) / 2 + 1;
    } else {
        minKeys = (bTreeOrder) % 2 == 0 ? (bTreeOrder) / 2 : (bTreeOrder) / 2 + 1;
        minKeys--;
    }

    // Check if the node has enough keys to maintain B+ tree properties
    if (node->num_keys >= minKeys)
        return treeMgr->root;

    // If the node falls below the minimum size, decide whether to merge or redistribute
    // Find the appropriate neighbor node with which to merge, and locate the key (kPrime)
    // in the parent node between the ptr to the node and its neighbor
    neighborIndex = getNeighborIndex(node);
    kPrimeIndex = neighborIndex == -1 ? 0 : neighborIndex;
    Value *kPrime = node->parent->keys[kPrimeIndex];
    neighbour = (neighborIndex == -1) ? node->parent->ptr[1] : node->parent->ptr[neighborIndex];

    capacity = node->is_leaf ? bTreeOrder : bTreeOrder - 1;

    if (neighbour->num_keys + node->num_keys < capacity)
        // Merge nodes if combining them does not exceed capacity
        return merge_Nodes(treeMgr, node, neighbour, neighborIndex, kPrime);
    else
        // Otherwise, redistribute entries between the nodes
        return redistribute_Nodes(treeMgr->root, node, neighbour, neighborIndex, kPrimeIndex, kPrime);
}




/**
 * @brief Deletes the entry/record with the specified key from the B+ tree.
 *
 * This function searches for the record with the specified key in the B+ tree, removes it,
 * and adjusts the tree structure as necessary to maintain B+ tree properties.
 *
 * @param treeManager Pointer to the B-tree manager structure.
 * @param key Pointer to the key of the entry to be deleted.
 * @return Pointer to the root node after deletion.
 */
Node *delete(BTreeMgr *treeMgr, Value *key) {
    // Find the record and the leaf node containing the key
    NodeData *record = find_Record(treeMgr->root, key);
    Node *key_Leaf = find_Leaf_Node(treeMgr->root, key);

    // If the record and leaf node are found, delete the entry and adjust the tree
    if (record != NULL && key_Leaf != NULL) {
        treeMgr->root = delete_Entry(treeMgr, key_Leaf, key, record);
        free(record); // Free memory allocated for the record
    }

    // Return the root of the tree
    return treeMgr->root;
}




/**
 * @brief Enqueues a node in the queue used for printing the B+ tree.
 *
 * This function adds a node to the end of the queue used for printing the B+ tree level by level.
 *
 * @param treeManager Pointer to the B+ tree manager.
 * @param newNode Pointer to the new node to be enqueued.
 */
// Function to add a node to the end of the queue
void enqueue(BTreeMgr *treeMgr, Node *newNode) {
    Node *current;
    
    // If the queue is empty, set the new node as the first node in the queue.
    if (treeMgr->queue == NULL) {
        treeMgr->queue = newNode;
        treeMgr->queue->next = NULL; // Ensure the next pointer of the new node points to NULL.
    } else {
        // Traverse the queue to find the last node.
        current = treeMgr->queue;
        while (current->next != NULL) {
            current = current->next;
        }
        // Add the new node to the end of the queue.
        current->next = newNode;
        newNode->next = NULL; // Ensure the next pointer of the new node points to NULL, indicating the end of the queue.
    }
}



/**
 * @brief Redistributes entries between two nodes when one node becomes too small after deletion,
 *        but its neighbor is too big to append the small node's entries without exceeding the maximum.
 *
 * This function redistributes keys and ptr between the node and its neighbor to balance the
 * number of entries between them, ensuring that neither node falls below the minimum allowable size
 * and preserving the B+ tree properties.
 *
 * @param root Pointer to the root node of the B+ tree.
 * @param node Pointer to the node requiring redistribution.
 * @param neighbor Pointer to the neighbor node for redistribution.
 * @param neighborIndex Index of the neighbor node.
 * @param kPrimeIndex Index of the key between the node and its neighbor in the parent node.
 * @param kPrime Pointer to the key between the node and its neighbor in the parent node.
 * @return Pointer to the root node after redistribution.
 */
// Function to redistribute keys and ptr between a node and its neighbor
Node *redistribute_Nodes(Node *root, Node *node, Node *neighbor, int neighborIndex, int kPrimeIndex, Value *kPrime) {
    int i;
    Node *temp;

    // If the node has a neighbor to the left
    if (neighborIndex != -1) {
        // Pull the last key-pointer pair from the neighbor to the node
        if (!node->is_leaf)
            node->ptr[node->num_keys + 1] = node->ptr[node->num_keys];

        // Shift keys and ptr in the node to make space for the new key-pointer pair
        for (i = node->num_keys; i > 0; i--) {
            node->keys[i] = node->keys[i - 1];
            node->ptr[i] = node->ptr[i - 1];
        }

        // Handle non-leaf nodes differently
        if (!node->is_leaf) {
            // Adjust ptr and keys for non-leaf nodes
            node->ptr[0] = neighbor->ptr[neighbor->num_keys];
            temp = (Node *) node->ptr[0];
            temp->parent = node;
            neighbor->ptr[neighbor->num_keys] = NULL;
            node->keys[0] = kPrime;
            node->parent->keys[kPrimeIndex] = neighbor->keys[neighbor->num_keys - 1];
        } else {
            // Adjust ptr and keys for leaf nodes
            node->ptr[0] = neighbor->ptr[neighbor->num_keys - 1];
            neighbor->ptr[neighbor->num_keys - 1] = NULL;
            node->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            node->parent->keys[kPrimeIndex] = node->keys[0];
        }
    } else {
        // Handle the case when the node is to the right of its neighbor
        if (node->is_leaf) {
            // Adjust keys and ptr for leaf nodes
            node->keys[node->num_keys] = neighbor->keys[0];
            node->ptr[node->num_keys] = neighbor->ptr[0];
            node->parent->keys[kPrimeIndex] = neighbor->keys[1];
        } else {
            // Adjust keys and ptr for non-leaf nodes
            node->keys[node->num_keys] = kPrime;
            node->ptr[node->num_keys + 1] = neighbor->ptr[0];
            temp = (Node *) node->ptr[node->num_keys + 1];
            temp->parent = node;
            node->parent->keys[kPrimeIndex] = neighbor->keys[0];
        }

        // Shift keys and ptr in the neighbor to fill the gap
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->ptr[i] = neighbor->ptr[i + 1];
        }

        // Adjust pointer in the neighbor for non-leaf nodes
        if (!node->is_leaf)
            neighbor->ptr[i] = neighbor->ptr[i + 1];
    }

    // Update the number of keys in the node and its neighbor
    node->num_keys++;
    neighbor->num_keys--;

    // Return the root of the tree
    return root;
}







/**
 * @brief Dequeues a node from the queue used for printing the B+ tree.
 *
 * This function removes and returns the first node from the queue used for printing the B+ tree level by level.
 *
 * @param treeManager Pointer to the B+ tree manager.
 * @return Pointer to the dequeued node.
 */
Node *dequeue(BTreeMgr *treeMgr) {
    Node *dequeuedNode = treeMgr->queue;
    treeMgr->queue = treeMgr->queue->next;
    dequeuedNode->next = NULL;
    return dequeuedNode;
}


/**
 * @brief Calculates the length in edges of the path from a child node to the root node.
 *
 * This function computes the number of edges traversed from a child node to the root node.
 *
 * @param root The root node of the B+ tree.
 * @param child The child node whose path to the root is being calculated.
 * @return The length in edges of the path from the child node to the root node.
 */
int pathToRoot(Node *root, Node *child) {
    int length = 0;
    Node *currentNode = child;
    while (currentNode != root) {
        currentNode = currentNode->parent;
        length++;
    }
    return length;
}


/**
 * @brief Compares two keys and returns true if the first key is less than the second key.
 *
 * This function compares two keys based on their data types and returns true if the first key
 * is less than the second key. It supports comparison for integer, float, and string data types.
 *
 * @param key1 The first key.
 * @param key2 The second key.
 * @return true if key1 is less than key2, otherwise false.
 */
bool is_Less(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            return key1->v.intV < key2->v.intV;
        case DT_FLOAT:
            return key1->v.floatV < key2->v.floatV;
        case DT_STRING:
            return strcmp(key1->v.stringV, key2->v.stringV) < 0;
        case DT_BOOL:
            // Boolean datatype can only be Equal or Not Equal To
            return false;
    }
}


/**
 * @brief Compares two keys and returns true if the first key is greater than the second key.
 *
 * This function compares two keys based on their data types and returns true if the first key
 * is greater than the second key. It supports comparison for integer, float, and string data types.
 *
 * @param key1 The first key.
 * @param key2 The second key.
 * @return true if key1 is greater than key2, otherwise false.
 */
bool is_Greater(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            return key1->v.intV > key2->v.intV;
        case DT_FLOAT:
            return key1->v.floatV > key2->v.floatV;
        case DT_STRING:
            return strcmp(key1->v.stringV, key2->v.stringV) > 0;
        case DT_BOOL:
            // Boolean datatype can only be Equal or Not Equal To
            return false;
    }
}


/**
 * @brief Compares two keys and returns true if the first key is equal to the second key, else returns false.
 *
 * This function compares two keys based on their data types and returns true if the first key
 * is equal to the second key. It supports comparison for integer, float, string, and boolean data types.
 *
 * @param key1 The first key.
 * @param key2 The second key.
 * @return true if key1 is equal to key2, otherwise false.
 */
bool is_Equal(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            return key1->v.intV == key2->v.intV;
        case DT_FLOAT:
            return key1->v.floatV == key2->v.floatV;
        case DT_STRING:
            return strcmp(key1->v.stringV, key2->v.stringV) == 0;
        case DT_BOOL:
            return key1->v.boolV == key2->v.boolV;
    }
}

