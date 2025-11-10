#include <stdint.h>

#pragma once
#include "memory.h"
#include "paging.h"

/* Run a void(void) function inside the function-space. This helper will
 * switch to the function page-directory, set up a stack inside the
 * function-space stack region, call the function, restore the original
 * stack and page-directory, and return.
 */
void run_in_function_space(void (*func)(void));