/* Nic Pucci
 * LIST HEADER
*/

#ifndef LIST_H
#define LIST_H 

/* PUBLIC ACCESS CONSTANT VARIABLES FOR TEST DRIVER */
const int SUCCESS_OP_CODE;
const int FAILURE_OP_CODE;

enum CURRENT_NODE_STATE {
	BEFORE_HEAD,
	WITHIN_LIST,
	AFTER_TAIL
};

typedef struct node
{
	void *valuePtr;
	struct node *prevNodePtr;	
	struct node *nextNodePtr;
	int allocID; // index in its array
} NODE;

typedef struct list 
{
	NODE *currentNodePtr;
	NODE *headNodePtr;
	NODE *tailNodePtr;
	int currentCapacity;
	enum CURRENT_NODE_STATE currentNodeState;
	int allocID; // index in its array
} LIST;


LIST *ListCreate ();

int ListCount ( LIST *list );

void *ListFirst ( LIST *list );

void *ListLast ( LIST *list );

void *ListNext ( LIST *list );

void *ListPrev ( LIST *list );

void *ListCurr ( LIST *list );

int ListAdd ( LIST *list , void *item );

int ListInsert ( LIST *list , void *item );

int ListAppend ( LIST *list , void *item );

int ListPrepend ( LIST *list , void *item );

void *ListRemove ( LIST *list );

void ListConcat ( LIST *list1 , LIST **list2 );

void ListFree ( LIST *list , void ( *itemFree ) ( void* ) );

void *ListTrim ( LIST *list );

void *ListSearch ( LIST *list , int ( *comparator ) ( void* , void* ) , void* comparisonArg );

#endif