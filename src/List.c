/* Nic Pucci
 * LIST IMPLEMENTATION
*/

#include <stdio.h>
#include "List.h"

const int SUCCESS_OP_CODE = 0;
const int FAILURE_OP_CODE = -1;

/* NUM OF ALLOCATIONS (Only for defining size of static arrays at compile-time) */
#define MAX_NUM_NODES_ALLOC 500
#define MAX_NUM_LISTS_ALLOC 500

/* NUM OF ALLOCATIONS AS INTS (For functions and usage) */
const int MAX_NUM_NODES = MAX_NUM_NODES_ALLOC;
const int MAX_NUM_LISTS = MAX_NUM_LISTS_ALLOC;

/* INITIALIZE MEMORY STATE FLAGS */
const int INITIALIZED_FREE_MEM_ALLOC = 1;
int initializedFreeMemAllocFlag = 0;

/* ALLOCATED MEMORY */
NODE allocNodesArr [ MAX_NUM_NODES_ALLOC ];
int *freeNodeIndexesPtrArr [ MAX_NUM_NODES_ALLOC ];

int topFreeNodeIndex = -1; 

LIST allocListsArr [ MAX_NUM_LISTS_ALLOC ];
int *freeListIndexesPtrArr [ MAX_NUM_LISTS_ALLOC ];
int topFreeListIndex = -1;

void DEBUG_PRINT_FREE_ALLOC_INFO () {
	printf ( "\n-------------- DEBUG_PRINT_FREE_ALLOC_INFO\n" );

	int totalFreeNodes = topFreeNodeIndex + 1;
	int totalFreeListHeads = topFreeListIndex + 1;

	printf ( "Total Free Nodes: %d\n", totalFreeNodes );
	printf ( "Total Free List Heads: %d\n\n", totalFreeListHeads );

	printf ( "topFreeNodeIndex: %d\n", topFreeNodeIndex );
	printf ( "topFreeListIndex: %d\n\n", topFreeListIndex );
}

void ClearNode ( NODE *node ) 
{
	if ( !node ) {
		return;
	}

	node -> valuePtr = NULL;
	node -> prevNodePtr = NULL;
	node -> nextNodePtr = NULL;
}

int SetList ( LIST *list , NODE *initHeadNodePtr ) 
{
	if ( !list ) {
		return FAILURE_OP_CODE;
	}

	if ( !initHeadNodePtr ) {
		list -> currentNodePtr = NULL;
		list -> headNodePtr = NULL;
		list -> tailNodePtr = NULL;
		list -> currentCapacity = 0;
		list -> currentNodeState = BEFORE_HEAD;
	}
	else {
		list -> currentNodePtr = initHeadNodePtr;
		list -> headNodePtr = initHeadNodePtr;
		list -> tailNodePtr = initHeadNodePtr;
		list -> currentCapacity = 1;
		list -> currentNodeState = WITHIN_LIST;
	}

	return SUCCESS_OP_CODE;
}

NODE *PopNextFreeNode () 
{
	int stackEmpty = topFreeNodeIndex < 0;
	if ( stackEmpty ) 
	{
		return NULL;
	}

	int freeIndex = *freeNodeIndexesPtrArr [ topFreeNodeIndex ];
	NODE* freeNode = &allocNodesArr [ freeIndex ];

	topFreeNodeIndex -= 1;

	return freeNode;
}

LIST *PopNextFreeList () 
{
	int stackEmpty = topFreeListIndex < 0;
	if ( stackEmpty ) 
	{
		return NULL;
	}

	int freeIndex = *freeListIndexesPtrArr [ topFreeListIndex ];
	LIST *freeList = &allocListsArr [ freeIndex ];

	topFreeListIndex -= 1;

	return freeList;
}

void PushFreedNode ( NODE *node ) 
{
	if ( !node ) 
	{
		return;
	}

	int* nodeIndex = &node -> allocID;
	if ( *nodeIndex >= MAX_NUM_NODES ) 
	{
		return;
	}

	int stackFull = topFreeNodeIndex >= MAX_NUM_NODES - 1;
	if ( stackFull ) 
	{
		return;
	}

	ClearNode ( node );

	topFreeNodeIndex += 1;
	freeNodeIndexesPtrArr [ topFreeNodeIndex ] = nodeIndex;

}

void PushFreedList ( LIST *list ) 
{
	if ( !list ) 
	{
		return;
	}

	int* listIndex = &list -> allocID;
	if ( *listIndex >= MAX_NUM_LISTS ) 
	{
		return;
	}

	int stackFull = topFreeListIndex >= MAX_NUM_LISTS - 1;
	if ( stackFull ) 
	{
		return;
	}

	SetList ( list , NULL );

	topFreeListIndex += 1;
	freeListIndexesPtrArr [ topFreeListIndex ] = listIndex;

}

void FreeAllocNode ( NODE *node ) 
{
	if ( !node ) 
	{
		return;
	}

	node -> prevNodePtr = NULL;
	node -> nextNodePtr = NULL;
	node -> valuePtr = NULL;

	PushFreedNode ( node );
}

void FreeAllocList ( LIST *list ) 
{
	if ( !list ) 
	{
		return;
	}

	SetList ( list , NULL );

	PushFreedList ( list );
}

NODE *GetNewNode ( void *item ) {
	NODE *node = PopNextFreeNode ();
	if ( node ) 
	{
		node -> valuePtr = item;
	}

	return node;
}

LIST *GetNewList () {
	LIST *list = PopNextFreeList ();
	return list;
}

void InitAllFreeNodes () 
{
	for ( int i = 0 ; i < MAX_NUM_NODES ; i++ ) 
	{
		NODE *node = &allocNodesArr [ i ];
		node -> allocID = i;
		PushFreedNode ( node );
	}
}

void InitAllFreeLists () 
{
	for ( int i = 0 ; i < MAX_NUM_LISTS ; i++ ) 
	{
		LIST *list = &allocListsArr [ i ];
		list -> currentNodeState = BEFORE_HEAD;
		list -> currentCapacity = 0;
		list -> allocID = i;
		PushFreedList ( list );
	}
}

void InitFreeAllocMemory () 
{
	InitAllFreeNodes (); 
	InitAllFreeLists ();
}

LIST *ListCreate () {
	if ( initializedFreeMemAllocFlag != INITIALIZED_FREE_MEM_ALLOC ) 
	{
		InitFreeAllocMemory ();
		initializedFreeMemAllocFlag = INITIALIZED_FREE_MEM_ALLOC;
	}

	LIST* list = GetNewList ();
	return list;
}

void *ListFirst ( LIST *list )
{
	if ( !list ) {
		return NULL;
	}

	NODE *headNodePtr = list -> headNodePtr;
	if ( !headNodePtr ) {
		return NULL;
	}

	list -> currentNodePtr = headNodePtr;

	void *value = headNodePtr -> valuePtr;
	return value;
}

void *ListLast ( LIST *list )
{
	if ( !list ) {
		return NULL;
	}

	NODE *tailNodePtr = list -> tailNodePtr;
	if ( !tailNodePtr ) 
	{
		return NULL;
	}

	list -> currentNodePtr = tailNodePtr;
	void *value = tailNodePtr -> valuePtr;
	return value;
}

void *ListCurr ( LIST *list )
{
	if ( !list ) {
		return NULL;
	}

	if ( list -> currentNodeState != WITHIN_LIST ) 
	{
		return NULL;
	}

	NODE* currentNode = list -> currentNodePtr;
	void *value = currentNode -> valuePtr;
	return value;
}

void *ListNext ( LIST *list ) 
{
	if ( !list ) {
		return NULL;
	}

	int listEmpty = list -> currentCapacity <= 0;
	if ( listEmpty ) 
	{
		return NULL;
	}

	if ( list -> currentNodeState == AFTER_TAIL ) 
	{
		return NULL;
	}

	if ( list -> currentNodeState == BEFORE_HEAD ) 
	{
		list -> currentNodePtr = list -> headNodePtr;
		list -> currentNodeState = WITHIN_LIST;
		return list -> currentNodePtr -> valuePtr;
	}

	list -> currentNodePtr = list -> currentNodePtr -> nextNodePtr;
	if ( !list -> currentNodePtr ) 
	{
		list -> currentNodeState = AFTER_TAIL;
		return NULL;
	}

	void *value = list -> currentNodePtr -> valuePtr;
	return value;
}

void *ListPrev ( LIST *list ) 
{
	if ( !list ) {
		return NULL;
	}

	int listEmpty = list -> currentCapacity <= 0;
	if ( listEmpty ) 
	{
		return NULL;
	}

	if ( list -> currentNodeState == BEFORE_HEAD ) 
	{
		return NULL;
	}

	if ( list -> currentNodeState == AFTER_TAIL ) 
	{
		list -> currentNodePtr = list -> tailNodePtr;
		list -> currentNodeState = WITHIN_LIST;
		return list -> currentNodePtr -> valuePtr;
	}

	list -> currentNodePtr = list -> currentNodePtr -> prevNodePtr;
	if ( !list -> currentNodePtr ) 
	{
		list -> currentNodeState = BEFORE_HEAD;
		return NULL;
	}

	return list -> currentNodePtr -> valuePtr;
}

int InsertNode ( NODE* firstNode , NODE* lastNode , NODE* insertNode ) {
	if ( !insertNode ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( firstNode ) 
	{
		firstNode -> nextNodePtr = insertNode;
		insertNode -> prevNodePtr = firstNode;
	}
	else 
	{
		insertNode -> prevNodePtr = NULL;
	}

	if ( lastNode ) 
	{
		lastNode -> prevNodePtr = insertNode;
		insertNode -> nextNodePtr = lastNode;
	}
	else 
	{
		insertNode -> nextNodePtr = NULL;
	}

	return SUCCESS_OP_CODE;
}

int ListAppend ( LIST *list , void *item ) 
{
	if ( !list ) 
	{
		return FAILURE_OP_CODE;
	}

	NODE* newItemNode = GetNewNode ( item );
	int noFreeNodesLeft = !newItemNode;
	if ( noFreeNodesLeft ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( list -> currentCapacity <= 0 ) 
	{
		return SetList ( list , newItemNode );
	}

	newItemNode -> prevNodePtr = list -> tailNodePtr;
	list -> tailNodePtr -> nextNodePtr = newItemNode;

	list -> tailNodePtr = newItemNode;
	list -> currentNodePtr = newItemNode;
	list -> currentNodeState = WITHIN_LIST;	
	list -> currentCapacity += 1;

	if ( !newItemNode -> prevNodePtr ) 
	{
		list -> headNodePtr = newItemNode;
	}

	return SUCCESS_OP_CODE;
}

int ListPrepend ( LIST *list , void *item ) 
{
	if ( !list ) 
	{
		return FAILURE_OP_CODE;
	}

	NODE* newItemNode = GetNewNode ( item );
	int noFreeNodesLeft = !newItemNode;
	if ( noFreeNodesLeft ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( list -> currentCapacity <= 0 ) 
	{
		return SetList ( list , newItemNode );
	}

	newItemNode -> nextNodePtr = list -> headNodePtr;
	list -> headNodePtr -> prevNodePtr = newItemNode;

	list -> headNodePtr = newItemNode;
	list -> currentNodePtr = newItemNode;
	list -> currentNodeState = WITHIN_LIST;	
	list -> currentCapacity += 1;

	if ( !newItemNode -> nextNodePtr ) 
	{
		list -> tailNodePtr = newItemNode;
	}

	return SUCCESS_OP_CODE;
}

int ListInsert ( LIST *list , void *item ) 
{
	if ( !list ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( list -> currentNodeState == BEFORE_HEAD || list -> currentNodePtr == list -> headNodePtr ) 
	{
		return ListPrepend ( list , item );
	}

	if ( list -> currentNodeState == AFTER_TAIL ) 
	{
		return ListAppend ( list , item );
	}

	NODE* newItemNode = GetNewNode ( item );
	int noFreeNodesLeft = !newItemNode;
	if ( noFreeNodesLeft ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( list -> currentCapacity <= 0 ) 
	{
		return SetList ( list , item );
	}

	NODE* prevNode = list -> currentNodePtr -> prevNodePtr;
	newItemNode -> prevNodePtr = prevNode;
	prevNode -> nextNodePtr = newItemNode;

	list -> currentNodePtr -> prevNodePtr = newItemNode;
	newItemNode -> nextNodePtr = list -> currentNodePtr;

	list -> currentNodePtr = newItemNode;
	list -> currentCapacity += 1;
	return SUCCESS_OP_CODE;
}

int ListAdd ( LIST *list , void *item ) 
{
	if ( !list ) 
	{
		return FAILURE_OP_CODE;
	}

	if ( list -> currentNodeState == BEFORE_HEAD ) 
	{
		return ListPrepend ( list , item );
	}
	
	if ( list -> currentNodeState == AFTER_TAIL ) 
	{
		return ListAppend ( list , item );
	}

	NODE* newItemNode = GetNewNode ( item );
	int noFreeNodesLeft = !newItemNode;
	if ( noFreeNodesLeft ) 
	{
		return FAILURE_OP_CODE;
	}

	int insertSuccess = InsertNode (
		list -> currentNodePtr,
		list -> currentNodePtr -> nextNodePtr,
		newItemNode
	);
	if ( insertSuccess == FAILURE_OP_CODE ) 
	{
		return FAILURE_OP_CODE;
	}

	list -> currentNodePtr = newItemNode;
	list -> currentCapacity += 1;

	if ( !list -> currentNodePtr -> nextNodePtr ) 
	{
		list -> tailNodePtr = newItemNode;
	}

	return SUCCESS_OP_CODE;
}

void *ListRemove ( LIST *list ) 
{
	if ( !list ) 
	{
		return NULL;
	}

	NODE *oldCurrentNode = list -> currentNodePtr;
	if ( !oldCurrentNode ) 
	{
		return NULL;
	}

	NODE *prevNode = oldCurrentNode -> prevNodePtr;
	NODE *nextNode = oldCurrentNode -> nextNodePtr;

	if ( prevNode ) 
	{
		prevNode -> nextNodePtr = nextNode;
	}
	else // at head 
	{
		list -> headNodePtr = nextNode;
	}

	if ( nextNode ) 
	{
		nextNode -> prevNodePtr = prevNode;
	}
	else // at tail
	{
		list -> tailNodePtr = prevNode;
	}

	list -> currentNodePtr = nextNode;
	list -> currentCapacity -= 1;

	void *value = oldCurrentNode -> valuePtr;
	FreeAllocNode ( oldCurrentNode );
	
	return value;
}

int ListCount ( LIST *list ) 
{
	if ( !list ) 
	{
		return 0;
	}

	int count = list -> currentCapacity;
	return count;
}

void *ListTrim ( LIST *list ) 
{
	if ( !list ) 
	{
		return NULL;
	}

	if ( list -> currentCapacity <= 0 ) 
	{
		return NULL;
	}

	NODE *trimmedNode = list -> tailNodePtr;
	list -> tailNodePtr = list -> tailNodePtr -> prevNodePtr;

	if ( list -> tailNodePtr ) 
	{
		list -> tailNodePtr -> nextNodePtr = NULL;
		list -> currentNodePtr = list -> tailNodePtr;
		list -> currentCapacity -= 1;
	}
	else 
	{
		SetList ( list , NULL );
	}

	void *value = trimmedNode -> valuePtr;
	FreeAllocNode ( trimmedNode );

	return value;
}

void ListFree ( LIST *list , void ( *itemFree ) ( void* ) ) 
{
	if ( !list ) 
	{
		return;
	}

	if ( !itemFree ) 
	{
		return;
	}

	int numNodes = list -> currentCapacity;

	for ( int i = 0 ; i < numNodes ; i++ ) 
	{
		void *value = ListTrim ( list );
		( *itemFree ) ( value );
	}

	FreeAllocList ( list );
}

void ListConcat ( LIST *list1 , LIST **list2 ) 
{
	if ( !list1 || !( *list2 ) || ( *list2 ) -> currentCapacity <= 0 ) 
	{
		SetList ( *list2 , NULL );
		FreeAllocList ( *list2 );
		*list2 = NULL;
		return;
	}

	if ( list1 -> currentCapacity <= 0 ) 
	{
		list1 -> headNodePtr = (* list2 ) -> headNodePtr;
		list1 -> tailNodePtr = (* list2 ) -> tailNodePtr;
		list1 -> currentCapacity = (* list2 ) -> currentCapacity;
	}
	else 
	{
		list1 -> tailNodePtr -> nextNodePtr = (* list2 ) -> headNodePtr;
		list1 -> tailNodePtr = (* list2 ) -> tailNodePtr;
		list1 -> currentCapacity += (* list2 ) -> currentCapacity;
	}

	SetList ( (* list2 ) , NULL );
	FreeAllocList ( (* list2 ) );
	(* list2 ) = NULL;
}

void *ListSearch ( LIST *list , int ( *comparator ) ( void* , void* ) , void *comparisonArg ) 
{
	if ( !list || list -> currentCapacity <= 0 ) 
	{
		return NULL;
	}

	if ( !comparator ) 
	{
		return NULL;
	}

	void *item = ListFirst ( list );

	int numNodes = list -> currentCapacity;
	for ( int i = 0; i < numNodes ; i++ ) 
	{
		int matchFound = ( *comparator ) ( item , comparisonArg );
		if ( matchFound ) 
		{
			return item;
		}

		item = ListNext ( list );
	}

	ListLast ( list );
	ListNext ( list );
	return NULL;
}