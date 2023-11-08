#include <stdint.h>
typedef struct cacheLine
{
    bool valid;
    uint64_t tag;
    int lru;
} cacheLine;

