#ifndef __CONCAT
#define __CONCAT(a, b) a##b
#endif

#ifndef __EXPAND_CONCAT
#define __EXPAND_CONCAT(a, b) __CONCAT(a, b)
#endif
