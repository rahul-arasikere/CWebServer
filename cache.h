#ifndef _WEBCACHE_H_
#define _WEBCACHE_H_

// Individual hash table entry
typedef struct cache_entry
{
    char *path; // Endpoint path--key to the cache
    char *content_type;
    int content_length;
    void *content;
    struct cache_entry *prev, *next; // Doubly-linked list
} cache_entry_t;

// A cache
typedef struct cache
{
    struct hashtable *index;
    cache_entry_t *head, *tail; // Doubly-linked list
    int max_size;               // Maxiumum number of entries
    int cur_size;               // Current number of entries
} cache_t;

extern cache_entry_t *alloc_entry(char *path, char *content_type, void *content, int content_length);
extern void free_entry(cache_entry_t *entry);
extern cache_t *cache_create(int max_size, int hashsize);
extern void cache_free(cache_t *cache);
extern void cache_put(cache_t *cache, char *path, char *content_type, void *content, int content_length);
extern cache_entry_t *cache_get(cache_t *cache, char *path);

#endif