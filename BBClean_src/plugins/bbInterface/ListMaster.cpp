/*===================================================

	LIST MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>

//Includes
#include "Definitions.h"

//Parent Include
#include "ListMaster.h"

//Local variables
//list *list_iterator_list;
//listnode *list_iterator_current = NULL;
//listnode *list_iterator_last = NULL;

/*

  Some notes on the list management code:

  Lists are currently implimented as linked lists,
  and lookup is performed using a linear search.
  This isn't exactly optimal, but I don't think a
  more complex algorithm is necessary at this time.
  Once BBInterface, becomes more functional, and
  therefore will be used more frequently, this code
  can be changed without affecting the rest of the
  code.

  In the future, I would suggest a linked list of
  list nodes, in addition to a hash map that maps
  key to linked list node.  This way, items can be
  added in O(1), items can be looked up in close to
  O(1), items can be deleted in close to O(1), and
  the entire list can STILL be listed in the same
  order it was created.

  Also, there is a tendency for one control to be
  used many times in a row (when loading config
  files), so the last searched result is cached, and
  we compare on that before doing any search.  This
  results in a big speedup.

  //-- pkt-zer0's notes --//
  
  I added the hash map to the lists. It uses a 8-bit
  hash, and a very simple hash function. The hash map
  maps to a single linked list of elements with the
  given hash. This should allow for much faster data
  access, with minimal additional memory requirement.
  The limit on the maximal number of elements (256)
  has been removed, since I did not see anything else
  limiting the size of lists. If that is not so, DO
  put the size limit back in place.

  Desperately in need of a less pathetic hash_func().

  //----------------------//

*/

// NOTE: Needs to be re-made into a templated class. (Mis-cast the void*s a bit too often. :P)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//hash_func
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned char hash_func(char const *str)
{
	int i;
    for( i=0; *str; str++ ) i = 53*i + *str;
    return( i % 256 );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_startup()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
list *list_create()
{
	list *newlist = new list;
	newlist->first = NULL;
	newlist->last = NULL;
	newlist->last_found = NULL;
	for (int i=0; i<256; i++) newlist->hash_table[i] = NULL; // could use memset, too
	return newlist;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_destroy(list *l)
{
	if (l)
	{
		listnode *ln = l->first;
		listnode *temp;
		while (ln)
		{
			temp = ln->next;
			delete ln;
			ln = temp;
		}
		delete l;
	}   
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_add
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_add(list *l, const char *key, void *value, void **old_value)
{
	if (old_value) *old_value = NULL;

	//Check for error conditions
	if (
		!! !l                  //If no list, forget it
		|| strlen(key) >= 96    //If key is longer than 96 chars, forget it      
		) return 1;

	//Look it up, and make sure it is unique!
	unsigned char hash_val = hash_func(key);
	listnode *ln = l->hash_table[hash_val];
	while (ln && strcmp(key, ln->key)) ln = ln->hash_next; // only check among elements with the same hash
	if (ln)
	{
		if (NULL == old_value) return 1;
		*old_value = ln->value;
		ln->value = value;
		return 0;
	}

	//Create the new list node
	ln = new listnode;
	strcpy(ln->key, key);
	ln->value = value;
	
	//Set its prev and next pointers
	ln->prev = l->last;
	ln->next = NULL;

	//Insert it into the linked list
	if (!l->first) l->first = ln;
	if (l->last) l->last->next = ln;
	l->last = ln;

	//Insert it into the hash table as well
	ln->hash_next = l->hash_table[hash_val];
	l->hash_table[hash_val] = ln;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_remove
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_remove(list *l, const char *key)
{
	//Hash table lookup
	unsigned char hash_val = hash_func(key);
	listnode *ln = l->hash_table[hash_val];

	while (ln && strcmp(key, ln->key)) { ln = ln->hash_next; }

	//If found, remove it
	if (ln)
	{
		if (!ln->next) l->last = ln->prev;
		else ln->next->prev = ln->prev;
		if (!ln->prev) l->first = ln->next;
		else ln->prev->next = ln->next;
		if (ln == l->last_found) l->last_found = NULL;
		// Unlink from hash list, too
		if (l->hash_table[hash_val] == ln) { l->hash_table[hash_val] = ln->hash_next; }
		else {
			// A hash_prev pointer would be wasting space, so this is how it'll have to be done. Ugly, yes.
			listnode *lnprev = l->hash_table[hash_val];
			while (lnprev->hash_next && strcmp(key, lnprev->hash_next->key)) { lnprev = lnprev->hash_next; }
			lnprev->hash_next = lnprev->hash_next->hash_next;
		}
		delete ln;
		return 0;
	}
	else
	{
		return 1;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_lookup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *list_lookup(list *l, const char *key)
{
	//Shortcut, the last found is cached for quick lookup
	if (l->last_found && !strcmp(key, l->last_found->key))
		return l->last_found->value;

	//Hash table lookup
	unsigned char hash_val = hash_func(key);
	listnode *ln = l->hash_table[hash_val];
	while (ln && strcmp(key, ln->key)) ln = ln->hash_next;
	if (ln)
	{
		l->last_found = ln;
		return ln->value;
	}
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_rename
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int list_rename(list *l, const char *key, const char *newkey)
{
	//Lookup the old item
	unsigned char hash_val = hash_func(key);
	listnode *ln = l->hash_table[hash_val], *lnprev = NULL;
	while (ln && strcmp(key, ln->key))
	{
		lnprev = ln;
		ln = ln->hash_next;
	}
	//If not found, error
	if (!ln) return 1;

	//Remove and re-hash
	if (!lnprev) l->hash_table[hash_val] = ln->hash_next;
	else lnprev->hash_next = ln->hash_next;
	//Insert it again
	hash_val = hash_func(newkey);
	ln->hash_next = l->hash_table[hash_val];
	l->hash_table[hash_val] = ln;

	//Copy the key
	strcpy(ln->key, newkey);

	//No errors
	return 0;
}

/* //unsafe due to the use of global vars
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_iterator_begin
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void list_iterator_begin(list *l)
{
	list_iterator_list = l;
	list_iterator_current = l->first;
	list_iterator_last = NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//list_iterator_next
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
listnode *list_iterator_next()
{
	if (!list_iterator_current) return NULL;
	list_iterator_last = list_iterator_current;
	list_iterator_current = list_iterator_current->next;
	return list_iterator_last;
}
*/
