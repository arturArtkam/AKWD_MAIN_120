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

#define MY_ASSERT(expr) \
  do {                                \
    if (!(expr)) {                    \
      my_assert(__FILE__, __LINE__);  \
    }                                 \
  } while (0)
