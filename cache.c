#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

/**
 * Allocate a cache entry
 */
cache_entry_t *alloc_entry(char *path, char *content_type, void *content, int content_length)
{
    cache_entry_t *entry = (cache_entry_t *)malloc(sizeof(cache_entry_t));
    entry->path = (char *)malloc(sizeof(char) * strlen(path));
    entry->content_type = (char *)malloc(sizeof(char) * strlen(content_type));
    entry->content = (char *)malloc(sizeof(char) * content_length);
    strcpy(entry->path, path);
    strcpy(entry->content_type, content_type);
    strcpy(entry->content, content);
    entry->content_length = content_length;
    return entry;
}

/**
 * Deallocate a cache entry
 */
void free_entry(cache_entry_t *entry)
{
    free(entry->path);
    free(entry->content_type);
    free(entry->content);
    free(entry);
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(cache_t *cache, cache_entry_t *ce)
{
    // Insert at the head of the list
    if (cache->head == NULL)
    {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    }
    else
    {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Move a cache entry to the head of the list
 */
void dllist_move_to_head(cache_t *cache, cache_entry_t *ce)
{
    if (ce != cache->head)
    {
        if (ce == cache->tail)
        {
            // We're the tail
            cache->tail = ce->prev;
            cache->tail->next = NULL;
        }
        else
        {
            // We're neither the head nor the tail
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }
        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Removes the tail from the list and returns it
 * 
 */
cache_entry_t *dllist_remove_tail(cache_t *cache)
{
    cache_entry_t *oldtail = cache->tail;
    cache->tail = oldtail->prev;
    cache->tail->next = NULL;
    cache->cur_size--;
    return oldtail;
}

/**
 * Create a new cache
 * 
 * max_size: maximum number of entries in the cache
 * hashsize: hashtable size (0 for default)
 */
cache_t *cache_create(int max_size, int hashsize)
{
    cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
    cache->index = hashtable_create(hashsize, NULL);
    cache->max_size = max_size;
    cache->cur_size = 0;
    cache->head = NULL;
    cache->tail = NULL;
    return cache;
}

// Free cache table
void cache_free(cache_t *cache)
{
    hashtable_destroy(cache->index);
    cache_entry_t *cur_entry = cache->head;
    while (cur_entry != NULL)
    {
        cache_entry_t *next_entry = cur_entry->next;
        free_entry(cur_entry);
        cur_entry = next_entry;
    }
    free(cache);
}

/**
 * Store an entry in the cache
 * 
 */
void cache_put(cache_t *cache, char *path, char *content_type, void *content, int content_length)
{
    cache_entry_t *ce = alloc_entry(path, content_type, content, content_length);
    if (cache->cur_size + 1 > cache->max_size)
    {
        // cache table full, delete least used item
        cache_entry_t *old_entry = dllist_remove_tail(cache);
        hashtable_delete(cache->index, old_entry->path);
        free_entry(old_entry);
    }
    // change size
    cache->cur_size++;
    dllist_insert_head(cache, ce);
    hashtable_put(cache->index, path, ce);
}

/**
 * Retrieve an entry from the cache
 */
cache_entry_t *cache_get(cache_t *cache, char *path)
{
    cache_entry_t *ce = (cache_entry_t *)hashtable_get(cache->index, path);
    if (ce != NULL)
    {
        // cache found, move to head for persistant storage
        dllist_move_to_head(cache, ce);
    }
    return ce;
}
