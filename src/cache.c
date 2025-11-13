#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

/**
 * Allocate a cache entry
 */
struct cache_entry *alloc_entry(char *path, char *content_type, void *content, int content_length)
{
    struct cache_entry *entry = malloc(sizeof(entry));
    entry->content = malloc(content_length);
    memcpy(entry->content, content, content_length);
    entry->path = path;
    entry->content_type = content_type;
    entry->content_length = content_length;
    return entry;
}

/**
 * Deallocate a cache entry
 */
void free_entry(struct cache_entry *entry)
{
    if (!entry)
        return;

    free(entry->path);
    free(entry->content_type);
    free(entry->content);
    free(entry->prev);
    free(entry->next);

    free(entry);
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(struct cache *cache, struct cache_entry *ce)
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
void dllist_move_to_head(struct cache *cache, struct cache_entry *ce)
{
    if (ce != cache->head)
    {
        if (ce == cache->tail)
        {
            // We're the tail
            cache->tail = ce->prev;   // point the tail to the previous entry.
            cache->tail->next = NULL; // detach the old tail.
        }
        else
        {
            // We're neither the head nor the tail
            ce->prev->next = ce->next; // attach the element behind us to the element in front of us.
            ce->next->prev = ce->prev; // attach the element in front of us the element behind us.
        }

        ce->next = cache->head; // point our next to the current head.
        cache->head->prev = ce; // point the current head prev pointer to us.
        ce->prev = NULL;        // we are the head so no prev pointer.
        cache->head = ce;       // set this cache entry to the head.
    }
}

/**
 * Removes the tail from the list and returns it
 *
 * NOTE: does not deallocate the tail
 */
struct cache_entry *dllist_remove_tail(struct cache *cache)
{
    struct cache_entry *oldtail = cache->tail;

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
struct cache *cache_create(int max_size, int hashsize)
{
    struct cache *new_cache = malloc(sizeof(max_size));
    struct hashtable *index = hashtable_create(hashsize, NULL);

    new_cache->max_size = max_size;
    new_cache->cur_size = 0;
    new_cache->index = index;
    new_cache->head = NULL;
    new_cache->tail = NULL;

    return new_cache;
}

void cache_free(struct cache *cache)
{
    struct cache_entry *cur_entry = cache->head;

    hashtable_destroy(cache->index);

    while (cur_entry != NULL)
    {
        struct cache_entry *next_entry = cur_entry->next;

        free_entry(cur_entry);

        cur_entry = next_entry;
    }

    free(cache);
}

/**
 * Store an entry in the cache
 *
 * This will also remove the least-recently-used items as necessary.
 *
 * NOTE: doesn't check for duplicate cache entries
 */
void cache_put(struct cache *cache, char *path, char *content_type, void *content, int content_length)
{
    struct cache_entry *entry = malloc(sizeof(struct cache_entry));
    entry->path = path;
    entry->content_type = content_type;
    entry->content_length = content_length;
    entry->content = content;
    entry->prev = NULL;

    cache->cur_size += 1;
    dllist_insert_head(cache, entry);
    hashtable_put(cache->index, path, entry);

    while (cache->cur_size > cache->max_size)
    {
        printf("Cleaning up cache\n");
        struct cache_entry *old_tail = dllist_remove_tail(cache);
        hashtable_delete(cache->index, path);
        free_entry(old_tail);
        cache->cur_size -= 1;
    }
}

/**
 * Retrieve an entry from the cache
 */
struct cache_entry *cache_get(struct cache *cache, char *path)
{
    struct cache_entry *entry;
    entry = hashtable_get(cache->index, path);
    if (!entry) return NULL;
    dllist_move_to_head(cache, entry);
    return entry;
}
