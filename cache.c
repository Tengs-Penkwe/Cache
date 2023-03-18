#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/*
 * Initialize a new cache set with the given associativity and index of the first cache line.
 */
static void cache_set_init(cache_set_t *cache_set, size_t associativity, cache_line_t *lines, size_t first_index) {
    cache_set->lines = lines;
    cache_set->first_index = first_index;
    cache_set->lru_list = malloc(associativity * sizeof(size_t));
    cache_set->num_marked = 0;
    
    for (int i = 0; i < associativity; i++) {
        cache_set->lines[first_index + i].is_valid = false;
        cache_set->lru_list[i] = i;
    }
}

/*
 * Given a value n which is a power of 2 (for example, a block size or a
 * number of sets in a cache), calculate log_2 of n.
 */
static size_t logbase2(size_t value) {
    size_t ans = 0;
    while (value > 1) {
        ans++;
        value >>= 1;
    }
    return ans;
}

/*
 * Given a number of bits, return a mask that many bits wide.
 */
static uint64_t maskbits(int nbits) {
    return (1L << nbits) - 1;
}

/*
 * Create a new cache that contains a total of num_bytes bytes, divided into
 * lines each of which is block_size bytes long, with the given associativity,
 * and the given set of cache policies for replacement and write operations.
 */
cache_t *cache_new(size_t num_bytes, size_t block_size, size_t associativity, uint8_t policies) {

    // Create the cache and initialize constant fields.
    cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
    cache->access_count = 0;
    cache->miss_count = 0;
    cache->policies = policies;

    // Initialize size fields.
    cache->line_size = block_size;
    cache->num_lines = num_bytes / block_size;
    cache->associativity = associativity;
    cache->num_sets = cache->num_lines / associativity;

    // Initialize shifts and masks in cache structure
    uint64_t offset_mask, index_mask;
    unsigned int offset_bits, index_bits;

    offset_bits = logbase2(block_size);
    offset_mask = maskbits(offset_bits);
    index_bits = logbase2(cache->num_sets);
    index_mask = maskbits(index_bits);

    // We shift by the number of bits in the offset to get
    // to the index bits.
    cache->block_offset_mask = offset_mask;
    cache->cache_index_shift = offset_bits;
    cache->cache_index_mask = index_mask << offset_bits;

    // We shift by the number of offset bits and index bits
    // to get the tag bits.
    cache->tag_shift = offset_bits + index_bits;
    cache->tag_mask = maskbits(sizeof(uintptr_t) - cache->tag_shift) << cache->tag_shift;

    // Allocate the cache memory
    cache->memory = malloc(num_bytes);
    uint8_t *memory = cache->memory;

    // Initialize cache lines.
    cache->lines = (cache_line_t *)calloc(cache->num_lines, sizeof(cache_line_t));
    for (size_t i = 0; i < cache->num_lines; i++) {
        cache->lines[i].block = memory;
        memory += cache->line_size;
    }
    
    // Initialize cache sets.
    cache->sets = (cache_set_t *)calloc(cache->num_sets, sizeof(cache_set_t));
    size_t first_index = 0;
    for (size_t i = 0; i < cache->num_sets; i++) {
        cache_set_init(&cache->sets[i], associativity, cache->lines, first_index);
        first_index += associativity;
    }

    return cache;
}


/**
 * Frees all memory allocated for a cache.
 */
void cache_free(cache_t *cache) {
  free(cache->memory);

  free(cache->lines);

  for (size_t i = 0; i < cache->num_sets; i++) {
    free(cache->sets[i].lru_list);
  }

  free(cache->sets);

  free(cache);
}

/*
 * Determine whether or not a cache line is valid for a given tag.
 */
bool cache_line_check_validity_and_tag(cache_line_t *cache_line, uintptr_t tag) {
    
  //ALARM check dirty
  if (cache_line->is_valid && tag == cache_line->tag) {
    return true;
  } else {
    return false;
  }
}

/*
 * Return uint64_t integer data from a cache line.
 */
uint64_t cache_line_retrieve_data(cache_line_t *cache_line, size_t offset) {

  //ALARM across the border

  uint64_t data = 0;
  for (int i = 0; i < 8; i++) {
    data += cache_line->block[offset+i] << (i*8);
  }
  
  return data;
}

/*
 * Move the cache lines inside a cache set so the cache line with the
 * given index is tagged as the most recently used one. The least
 * recently used cache line will be the 0'th one in the set, the
 * second least recently used cache line will be next, etc.  Cache
 * lines whose valid bit is 0 will occur before all cache lines whose
 * valid bit is 1.
 */
static void cache_line_make_mru(cache_t *cache, cache_set_t *cache_set, size_t line_index) {
    size_t index_of_line_index = -1;
    for (size_t i = 0; i < cache->associativity; i++) {
        if (cache_set->lru_list[i] == line_index) {
            index_of_line_index = i;
            break;
        }
    }

    for (size_t i = index_of_line_index + 1; i < cache->associativity; i++) {
        cache_set->lru_list[i - 1] = cache_set->lru_list[i];
    }
    cache_set->lru_list[cache->associativity - 1] = line_index;
}

/*
 * Retrieve a matching cache line from a set, if one exists.
 */
cache_line_t *cache_set_find_matching_line(cache_t *cache, cache_set_t *cache_set, uintptr_t tag) {

  cache_line_t* lines = cache_set->lines;
  for (int i = cache_set->first_index; i < cache->associativity; i++) {
    if ( cache_line_check_validity_and_tag(lines + i, tag)){
      if (cache->policies == CACHE_REPLACEMENTPOLICY_LRU) {
        cache_line_make_mru(cache, cache_set, i);
      }
      return lines + i;
    }
  }
  return NULL;
}

/*
 * Function to choose a random unmarked line from the cache. If all lines are
 * marked, then it unmarks them all first.
 */
size_t choose_unmarked_cache_line(cache_t *cache, cache_set_t *cache_set, func_t generate_random_number) {

  cache_line_t* lines = cache_set->lines;

  //unmark all if all were marked
  if (cache_set->num_marked == cache->associativity) {
    for(int i = 0; i < cache->associativity; i++) {
      lines[i].is_marked = false;
      cache_set->num_marked = 0;
    }
  } 

  //if there is invalid cache, replace it
  for (int i = cache_set->first_index; i < cache->associativity; i++) {
    if (lines[i].is_valid == false) {
      return i;
    }
  }

  int select = generate_random_number() % (cache->associativity - cache_set->num_marked);
  for (int i = cache_set->first_index; i < cache->associativity; i++) {
    if (select == 0 && lines[i].is_marked == false) {
      return i;
    }
    if (lines[i].is_marked == false) {
      select --;
    } 
  }

  //ALARM
  return 0;
}
   
void printlru(cache_set_t *cache_set) {
    printf("lru");
    for(int j = 0; j < 4; j++) {
      printf("%zu", cache_set->lru_list[j]);
    }
    printf("\n");
}
/*
 * Function to find a cache line to use for new data. Uses either a
 * line not being used, or a suitable line to be replaced, based on
 * the cache's replacement policy.
 */
cache_line_t *find_available_cache_line(cache_t *cache, cache_set_t *cache_set, func_t generate_random_number) {

  cache_line_t* lines = cache_set->lines;

  switch (cache->policies) {
    case CACHE_REPLACEMENTPOLICY_LRU : {
      size_t index = 0;
      for (int i =  (cache -> associativity ); i > (cache_set-> first_index); i--) {
        index = cache_set->lru_list[i-1];
        if (!lines[index].is_valid) {
          cache_line_make_mru(cache, cache_set, index);
          return lines + index;
        }
      }
      
      index = cache_set->lru_list[0];
      cache_line_make_mru(cache, cache_set, index);
      return lines + index;
    }
    case CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING: {
      size_t index =  choose_unmarked_cache_line(cache, cache_set, generate_random_number);
      lines[index].is_marked = true;
      cache_set->num_marked ++;
      return lines + index;
    }
    default: return NULL; // Added to remove warning; remove once function is implemented.
  }
}

/*
 * Add a block to a given cache set.
 */
static cache_line_t *cache_set_add(cache_t *cache, cache_set_t *cache_set, uintptr_t address, uintptr_t tag, func_t generate_random_number) {

    // First locate the cache line to use.
    cache_line_t *line = find_available_cache_line(cache, cache_set, generate_random_number);

    // Now set it up.
    line->tag = tag;
    line->is_valid = true;
    memcpy(line->block, (void *)(address & ~cache->block_offset_mask), cache->line_size);

    // And return it.
    return line;
}

/*
 * Read a single uint64_t integer from the cache.
 */
uint64_t cache_read(cache_t *cache, uintptr_t address, func_t generate_random_number) {
  
  int cache_index = cache->cache_index_mask & address;
  uintptr_t tag = cache->tag_mask & address;
  size_t offset = cache->block_offset_mask & address;

  cache_line_t* line = cache_set_find_matching_line(cache, cache->sets + cache_index, tag);
  if (line != NULL) {
    return cache_line_retrieve_data(line, offset);
  }

  /* TO BE COMPLETED BY THE STUDENT */
  return 0; // Added to remove warning; remove once function is implemented.
}

/*
 * Write a single integer to the cache.
 */
void cache_write(cache_t *cache, uintptr_t address, uint64_t value, func_t generate_random_number) {
    /* OPTIONAL FUNCTION, YOU ARE NOT REQUIRED TO IMPLEMENT IT */
}

/*
 * Return the number of cache misses since the cache was created.
 */
uint32_t cache_miss_count(cache_t *cache) {

    return cache->miss_count;
}

/*
 * Return the number of cache accesses since the cache was created.
 */
uint32_t cache_access_count(cache_t *cache) {

    return cache->access_count;
}

