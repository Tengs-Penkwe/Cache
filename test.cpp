#include "catch.hpp"
extern "C"
{
#include "cache.h"
}

TEST_CASE("cache_line_check_validity_and_tag", "[weight=1][part=test]")
{
    cache_line_t cache_line;
    cache_line.is_valid = true;
    cache_line.tag = 10;

    bool actual = !!cache_line_check_validity_and_tag(&cache_line, 10);
    REQUIRE(actual == true);

    actual = !!cache_line_check_validity_and_tag(&cache_line, 9);
    REQUIRE(actual == false);

    cache_line.is_valid = false;
    cache_line.tag = 10;

    actual = !!cache_line_check_validity_and_tag(&cache_line, 10);
    REQUIRE(actual == false);

    actual = !!cache_line_check_validity_and_tag(&cache_line, 9);
    REQUIRE(actual == false);
}

TEST_CASE("cache_line_retrieve_data", "[weight=1][part=test]")
{
    uint64_t block[] = {100, 101};
    cache_line_t cache_line;
    cache_line.block = (uint8_t *)block;

    long data = cache_line_retrieve_data(&cache_line, 0);
    REQUIRE(data == 100);

    data = cache_line_retrieve_data(&cache_line, 8);
    REQUIRE(data == 101);
}

#define ASSERT_EQUAL(A, B) REQUIRE((A) == (B))

TEST_CASE("cache_set_find_matching_line::LRU", "[weight=1][part=test]")
{

    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_LRU;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {true, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {3, 2, 1, 0};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.lru_list = lru_list;

    cache_line_t * actual = cache_set_find_matching_line(&cache, &cache_set, 11);
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 0);
    ASSERT_EQUAL(lru_list[3], 1);

    actual = cache_set_find_matching_line(&cache, &cache_set, 12);
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 0);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 2);

    actual = cache_set_find_matching_line(&cache, &cache_set, 10);
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 0);
    
    actual = cache_set_find_matching_line(&cache, &cache_set, 13);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 0);

    
    actual = cache_set_find_matching_line(&cache, &cache_set, 14);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 0);
}

/*
TEST_CASE("cache_set_find_matching_line::RANDOM", "[weight=1][part=test]")
{

    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_RANDOM;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {true, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {0, 1, 2, 3};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.lru_list = lru_list;

    cache_line_t * actual = cache_set_find_matching_line(&cache, &cache_set, 11);
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = cache_set_find_matching_line(&cache, &cache_set, 12);
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = cache_set_find_matching_line(&cache, &cache_set, 10);
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);
    
    actual = cache_set_find_matching_line(&cache, &cache_set, 13);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    
    actual = cache_set_find_matching_line(&cache, &cache_set, 14);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);
}
*/

TEST_CASE("cache_set_find_matching_line::RANDOM", "[weight=1][part=test]")
{

    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {true, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {3, 2, 1, 0};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.lru_list = lru_list;

    cache_line_t * actual = cache_set_find_matching_line(&cache, &cache_set, 11);
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);

    actual = cache_set_find_matching_line(&cache, &cache_set, 12);
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);

    actual = cache_set_find_matching_line(&cache, &cache_set, 10);
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    
    actual = cache_set_find_matching_line(&cache, &cache_set, 13);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);

    
    actual = cache_set_find_matching_line(&cache, &cache_set, 14);
    ASSERT_EQUAL(actual, NULL);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
}

TEST_CASE("find_available_cache_line::LRU", "[weight=1][part=test]")
{
    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_LRU;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {false, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {3, 2, 1, 0};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.lru_list = lru_list;

    // For testing purposes, we are mocking "generate_random_number" to always return 1
    // If you are really measuring the miss rate of your cache, use "rand" (from stdlib.h) instead of "[](){ return 1; }"
    cache_line_t *actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 0);
    ASSERT_EQUAL(lru_list[3], 2);

    lines[2].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[3]);
    ASSERT_EQUAL(lru_list[0], 1);
    ASSERT_EQUAL(lru_list[1], 0);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    lines[3].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 3);
    ASSERT_EQUAL(lru_list[3], 1);

// printf("%d\n", __LINE__ );
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 2);
    ASSERT_EQUAL(lru_list[1], 3);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 0);
    ASSERT_EQUAL(lru_list[3], 2);

    lines[1].is_valid = false;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 0);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 1);
}

/*
TEST_CASE("find_available_cache_line::RANDOM", "[weight=1][part=test]")
{
    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_RANDOM;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {false, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {0, 1, 2, 3};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.lru_list = lru_list;
    
    // For testing purposes, we are mocking "generate_random_number" to always return 1
    // If you are really measuring the miss rate of your cache, use "rand" (from stdlib.h) instead of "[](){ return 1; }"
    cache_line_t *actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    lines[2].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[3]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    lines[3].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 0; });
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 2; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 5; });
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 6; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 0);
    ASSERT_EQUAL(lru_list[1], 1);
    ASSERT_EQUAL(lru_list[2], 2);
    ASSERT_EQUAL(lru_list[3], 3);
}
*/

TEST_CASE("find_available_cache_line::RANDOM", "[weight=1][part=test]")
{
    cache_t cache;
    cache.policies = CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING;
    cache.num_lines = 16;
    cache.num_sets = 4;
    cache.associativity = cache.num_lines / cache.num_sets;

    cache_set_t cache_set;
    cache_line_t lines[] = {{true, false, false, 10}, {true, false, false, 11}, {false, false, false, 12}, {false, false, false, 13}};
    size_t lru_list[] = {3, 2, 1, 0};
    cache_set.lines = lines;
    cache_set.first_index = 0;
    cache_set.num_marked = 0;
    cache_set.lru_list = lru_list;
    
    // For testing purposes, we are mocking "generate_random_number" to always return 1
    // If you are really measuring the miss rate of your cache, use "rand" (from stdlib.h) instead of "[](){ return 1; }"
    cache_line_t *actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 1);
    ASSERT_EQUAL(lines[0].is_marked, 0);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 0);

    lines[2].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[3]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 2);
    ASSERT_EQUAL(lines[0].is_marked, 0);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 1);

    lines[3].is_valid = true;
    actual = find_available_cache_line(&cache, &cache_set, [](){ return 0; });
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 3);
    ASSERT_EQUAL(lines[0].is_marked, 1);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 1);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 1; });
    ASSERT_EQUAL(actual, &lines[1]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 4);
    ASSERT_EQUAL(lines[0].is_marked, 1);
    ASSERT_EQUAL(lines[1].is_marked, 1);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 1);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 2; });
    ASSERT_EQUAL(actual, &lines[2]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 1);
    ASSERT_EQUAL(lines[0].is_marked, 0);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 0);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 5; });
    ASSERT_EQUAL(actual, &lines[3]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 2);
    ASSERT_EQUAL(lines[0].is_marked, 0);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 1);

    actual = find_available_cache_line(&cache, &cache_set, [](){ return 6; });
    ASSERT_EQUAL(actual, &lines[0]);
    ASSERT_EQUAL(lru_list[0], 3);
    ASSERT_EQUAL(lru_list[1], 2);
    ASSERT_EQUAL(lru_list[2], 1);
    ASSERT_EQUAL(lru_list[3], 0);
    ASSERT_EQUAL(cache_set.num_marked, 3);
    ASSERT_EQUAL(lines[0].is_marked, 1);
    ASSERT_EQUAL(lines[1].is_marked, 0);
    ASSERT_EQUAL(lines[2].is_marked, 1);
    ASSERT_EQUAL(lines[3].is_marked, 1);
}

