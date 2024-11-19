#ifndef BTREE_OPERATIONS_H
#define BTREE_OPERATIONS_H

#include "buffer_mgr.h"
#include "btree_mgr.h"

// Structure representing a node in the B+ Tree
typedef struct Node {
	void ** ptr; // ptrs to child nodes or records
	Value ** keys; // Keys stored in the node
	struct Node * parent; // ptr to the parent node
	bool is_leaf; // Flag indicating whether the node is a leaf
	int num_keys; // Number of keys currently in the node
	struct Node * next; // Used for queue operations
} Node;

// Structure facilitating scan operations on the B+ Tree
typedef struct ScanMgr {
	int keyIndex; // Index of the current key in the scan
	int totalKeys; // Total number of keys in the B+ Tree
	int order; // Order of the B+ Tree
	Node * node; // Current node in the scan
} ScanMgr;

// Structure storing additional information of the B+ Tree
typedef struct BTreeMgr {
	BM_BufferPool bufferPool; // Buffer pool for managing pages
	BM_PageHandle pageHandler; // Page handler for page operations
	int order; // Order of the B+ Tree
	int numNodes; // Total number of nodes in the B+ Tree
	int numEntries; // Total number of entries (records) in the B+ Tree
	Node * root; // ptr to the root node of the B+ Tree
	Node * queue; // Queue for level-order traversal
	DataType keyType; // Data type of the keys in the B+ Tree
} BTreeMgr;


// Structure to hold the actual data of an entry
typedef struct NodeData {
	RID rid; // Record ID
} NodeData;

// Functions to find an element (record) in the B+ Tree
Node * find_Leaf_Node(Node * root, Value * key);
NodeData * find_Record(Node * root, Value * key);

// Functions to support printing of the B+ Tree
void enqueue(BTreeMgr * treeMgr, Node * node);
Node * dequeue(BTreeMgr * treeMgr);
int pathToRoot(Node * root, Node * child);

// Functions to support deletion of an element (record) in the B+ Tree
Node * adjust_Root(Node * root);
Node * merge_Nodes(BTreeMgr * treeMgr, Node * n, Node * neighbour, int neighbour_index, Value * k_prime);
Node * redistribute_Nodes(Node * root, Node * n, Node * neighbour, int neighbour_index, int k_prime_index, Value * k_prime);
Node * delete_Entry(BTreeMgr * treeMgr, Node * n, Value * key, void * ptr);
Node * delete(BTreeMgr * treeMgr, Value * key);
Node * remove_Entry_From_Node(BTreeMgr * treeMgr, Node * n, Value * key, Node * ptr);
int getNeighborIndex(Node * n);

// Functions to support keys of multiple data types
bool is_Less(Value * first_key, Value * sec_key);
bool is_Greater(Value * first_key, Value * sec_key);
bool is_Equal(Value * first_key, Value * sec_key);

// Functions to support addition of an element (record) in the B+ Tree
NodeData * make_Record(RID * rid);
Node * insert_Into_Leaf(BTreeMgr * treeMgr, Node * leaf, Value * key, NodeData * ptr);
Node * create_New_Tree(BTreeMgr * treeMgr, Value * key, NodeData * ptr);
Node * create_Node(BTreeMgr * treeMgr);
Node * create_Leaf(BTreeMgr * treeMgr);
Node * insert_Into_Leaf_After_Splitting(BTreeMgr * treeMgr, Node * leaf, Value * key, NodeData * ptr);
Node * insert_Into_Node(BTreeMgr * treeMgr, Node * parent, int leftIndex, Value * key, Node * right);
Node * insert_Into_Node_After_Splitting(BTreeMgr * treeMgr, Node * parent, int leftIndex, Value * key, Node * right);
Node * insert_Into_Parent(BTreeMgr * treeMgr, Node * left, Value * key, Node * right);
Node * insert_Into_NewRoot(BTreeMgr * treeMgr, Node * left, Value * key, Node * right);
int get_Left_Index(Node * parent, Node * left);

#endif
