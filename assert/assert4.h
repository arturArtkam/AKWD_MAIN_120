#pragma once

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void my_assert(const char* file, uint32_t line);

#ifdef __cplusplus
}
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define MY_ASSERT(expr)                   \
  do {                                    \
    if (!(expr)) {                        \
      my_assert(__FILENAME__, __LINE__);  \
    }                                     \
  } while (0)
