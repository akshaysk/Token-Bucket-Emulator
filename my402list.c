/***********************************************************
Doubly linked circular list implementation
***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my402list.h"

int My402ListInit(My402List *MyList)				
{
	if (!MyList)
		return 0;
	else{
		MyList->anchor.prev = &MyList->anchor;
		MyList->anchor.next = &MyList->anchor;
		MyList->num_members = 0;
		MyList->anchor.obj=NULL;
	}
	return TRUE;
}

int My402ListLength(My402List *MyList)		
{
		return MyList->num_members;

}

int My402ListEmpty(My402List *MyList)
{
	if(MyList->num_members == 0)
		return TRUE;
	return FALSE;
}

My402ListElem * My402ListFirst(My402List *MyList)
{
	if(My402ListEmpty(MyList))
		return NULL;
	
	return MyList->anchor.next;
}

My402ListElem * My402ListLast(My402List *MyList)
{
	if(My402ListEmpty(MyList))
		return NULL;
	
	return MyList->anchor.prev;
}

My402ListElem * My402ListNext(My402List *MyList, My402ListElem * MyListElem)
{
		if(MyListElem == My402ListLast(MyList))
			return NULL;
		return MyListElem->next;
	
}

My402ListElem * My402ListPrev(My402List *MyList, My402ListElem * MyListElem)
{
		if(MyListElem == My402ListFirst(MyList))
			return NULL;
		return MyListElem->prev;
	
}

My402ListElem * My402ListFind(My402List *MyList, void *object)
{
	My402ListElem *elem = NULL;
	for(elem=My402ListFirst(MyList);elem != NULL;elem=My402ListNext(MyList,elem))
	{
		if(elem->obj == object)
			return elem;
		 
	}
		
	return NULL;	
}


int My402ListAppend(My402List *MyList, void *object)
{
	My402ListElem *elem, *last;
	
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if (!elem)
		return FALSE;
	elem->obj = object;
	last = My402ListLast(MyList);
	if(last == NULL)
		last = &MyList->anchor;

	elem -> next = &MyList->anchor;
	MyList->anchor.prev = elem;
	last->next = elem;
	elem -> prev = last;
	MyList->num_members++;
	
	return TRUE;				
}


int My402ListPrepend(My402List *MyList, void * object)
{
	My402ListElem *elem, *first;
	
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if(!elem)
		return FALSE;
	
	elem -> obj = object;
	first = My402ListFirst(MyList);
	if(first == NULL)
		first = &MyList->anchor;
	elem -> next = first;
	first -> prev = elem;
	MyList->anchor.next = elem;
	elem -> prev = &MyList->anchor;
	MyList->num_members++;
	return TRUE;
	
}

void My402ListUnlink(My402List *MyList, My402ListElem *elem)
{
	My402ListElem *prev_elem, *next_elem;
	if(elem == &MyList->anchor)
		return;	
	
	prev_elem = My402ListPrev(MyList, elem);
	if(prev_elem == NULL)
		prev_elem = &MyList->anchor;
	
	next_elem = My402ListNext(MyList, elem);
	if(next_elem == NULL)
		next_elem = &MyList->anchor;

	prev_elem->next = next_elem;
	next_elem->prev = prev_elem;
	
	elem->next = NULL;
	elem->prev = NULL;
	elem->obj = NULL;
	free(elem);	
	
	MyList->num_members--;	
} 

void My402ListUnlinkAll(My402List *MyList)
{
	My402ListElem *elem = NULL; 
	
	for(elem = My402ListFirst(MyList);elem != NULL; elem=My402ListFirst(MyList))
		My402ListUnlink(MyList,elem);

}

int My402ListInsertBefore(My402List *MyList, void *object, My402ListElem *elem)
{
	My402ListElem *new_elem = NULL,*prev_elem = NULL;

	if(elem == NULL)
		My402ListPrepend(MyList, object);
	
	else	
	{
		prev_elem = My402ListPrev(MyList, elem);
		if(prev_elem == NULL)
			prev_elem = &MyList->anchor;

		new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		if(!new_elem)
			return FALSE;

		new_elem -> obj = object;
		new_elem -> next = elem;
		elem -> prev = new_elem;
		prev_elem -> next = new_elem;
		new_elem -> prev = prev_elem;

		MyList->num_members++;	
	}
	return 1;	
}

int My402ListInsertAfter(My402List *MyList, void *object, My402ListElem *elem)
{
	My402ListElem *new_elem = NULL,*next_elem = NULL;

	if(elem == NULL)
		My402ListAppend(MyList, object);
	
	else	
	{
		next_elem = My402ListNext(MyList, elem);
		if(next_elem == NULL)
			next_elem = &MyList->anchor;

		new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		if(!new_elem)
			return FALSE;

		new_elem -> obj = object;
		new_elem -> next = next_elem;
		next_elem -> prev = new_elem;
		elem -> next = new_elem;
		new_elem -> prev = elem;

		MyList->num_members++;	
	}
	return TRUE;	
}

void Traverse(My402List *list)
{
        My402ListElem *elem=My402ListFirst(list);
	int *foo;
	for (elem=My402ListFirst(list);elem != NULL;elem=My402ListNext(list, elem)) {
		foo=(int *)(elem->obj);
		printf("%d",*foo);	
        }
	printf("\n");
}


