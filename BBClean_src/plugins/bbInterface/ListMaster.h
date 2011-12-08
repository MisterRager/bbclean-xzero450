/*===================================================

	LIST MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_ListMaster_h
#define BBInterface_ListMaster_h

//Includes

//Pre-defined structures
struct list;
struct listnode;

//Define these structures
struct listnode
{
	char key[96];
	void *value;

	listnode *next;
	listnode *prev;

	listnode *hash_next; // if more nodes share the same hash value, this points to the next one
};

struct list
{
	listnode *first;
	listnode *last;
	listnode *last_found;
	listnode *hash_table[256]; // all the possible 1-byte hash values
};

//Define these functions internally
int list_startup();
int list_shutdown();

list *list_create();
int list_destroy(list *l);

int list_add(list *l, const char *key, void *value, void** old_value);
int list_remove(list *l, const char *key);
void *list_lookup(list *l, const char *key);
int list_rename(list *l, const char *key, const char *newkey);

#define dolist(_ln, _plist) for (_ln = _plist->first; _ln; _ln = _ln->next)

#endif
/*=================================================*/
