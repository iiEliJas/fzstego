#ifndef TEST_UTILS
#define TEST_UTILS

#include <stdio.h>
 
static int tests_run    = 0;
static int tests_passed = 0;
 
static void pass(const char *name){
    tests_passed++;
    tests_run++;
    printf("  [PASS] %s\n", name);
}
 
static void fail(const char *name, const char *msg){
    tests_run++;
    printf("  [FAIL] %s - %s\n", name, msg);
}

#endif //TEST
