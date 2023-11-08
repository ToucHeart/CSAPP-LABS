#include <limits.h>
#include <string.h>
#include <assert.h>
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "csim.h"

static bool verbose;                             // verbose mode
static unsigned setSize, setLineSize, blockSize; // set address bits,line counts in each set, block address bits
static char *traceName;                          // trace name in cmd argument
static unsigned long long visitAddr;             // visit address in each line
static int visitSize;                            // be ignored
static cacheLine **cachelines;                   // simulate cache lines
static int hit, miss, eviction;                  // each case count

#define DEBUG_PRINT(valid, msg, ...) \
    if (valid)                       \
    {                                \
        printf(msg, ##__VA_ARGS__);  \
    }

#define VERBOSE_PRINT(msg, ...)     \
    if (verbose)                    \
    {                               \
        printf(msg, ##__VA_ARGS__); \
    }

static void usage()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n"
           "\n"
           "Examples:\n"
           "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
    exit(EXIT_SUCCESS);
}

/* cmd example:  ./csim-ref -v -s 4 -E 1 -b 4 -t traces/yi.trace  */
static void parseArgs(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("%s: Missing required command line argument\n", argv[0]);
        usage();
    }
    int ch;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            usage();
            break;
        case 'v':
            verbose = true;
            DEBUG_PRINT(false, "verbose is true\n")
            break;
        case 's':
            setSize = atoi(optarg);
            DEBUG_PRINT(false, "setSize is %d\n", setSize)
            break;
        case 'E':
            setLineSize = atoi(optarg);
            DEBUG_PRINT(false, "setLineSize is %d\n", setLineSize);
            break;
        case 'b':
            blockSize = atoi(optarg);
            DEBUG_PRINT(false, "blockSize is %d\n", blockSize);
            break;
        case 't':
            traceName = optarg;
            DEBUG_PRINT(false, "traceName is %s\n", traceName);
            break;
        default:
            usage();
            break;
        }
    }
    if (!setSize || !setLineSize || !blockSize || !traceName)
    {
        printf("Lack argument\n");
        exit(EXIT_FAILURE);
    }
}

char *getAddress(char *line)
{
    char *beg = line;

    while (*line != ',')
    {
        line++;
    }
    *line = '\0';

    char *ret = line + 1;

    sscanf(beg, "%llx", &visitAddr);

    DEBUG_PRINT(false, "address is %llx\n", visitAddr);

    return ret;
}

/*
Each data load (L) or store (S) operation can cause at most onecache miss.
The data modify operation(M) is treated as a load followed by a store to the same address.
Thus, an M operation can result intwo cache hits,
or a miss and a hit plus a possible eviction.
*/

// add lru val in valid line whose lru is less than given lru
void addLru(int beg, int end, int lru)
{
    for (unsigned j = beg; j < end; ++j)
    {
        if (cachelines[j]->valid && cachelines[j]->lru < lru)
        {
            ++cachelines[j]->lru;
        }
    }
}

void visitCache()
{
    unsigned setIdx = (visitAddr >> blockSize) & ((1 << setSize) - 1);
    unsigned tag = (visitAddr >> (blockSize + setSize));
    unsigned beg = setIdx * setLineSize, end = (setIdx + 1) * setLineSize;
    // find whether exists
    for (unsigned i = beg; i < end; ++i)
    {
        if (cachelines[i]->tag == tag && cachelines[i]->valid)
        {
            hit++;
            int lru = cachelines[i]->lru;
            addLru(beg, end, lru);
            cachelines[i]->lru = 0;
            VERBOSE_PRINT("hit ")
            return;
        }
    }
    // not found
    miss++;
    VERBOSE_PRINT("miss ")
    // add new
    // has empty cache line
    for (unsigned i = beg; i < end; ++i)
    {
        if (!cachelines[i]->valid)
        {
            addLru(beg, end, INT_MAX);
            cachelines[i]->valid = true;
            cachelines[i]->tag = tag;
            cachelines[i]->lru = 0;
            return;
        }
    }
    // doesn't has empty cache line
    int maxval = INT_MIN;
    int maxidx = -1;
    for (unsigned i = beg; i < end; ++i)
    {
        if (cachelines[i]->lru > maxval)
        {
            maxval = cachelines[i]->lru;
            maxidx = i;
        }
    }
    addLru(beg, end, INT_MAX);
    cachelines[maxidx]->valid = true;
    cachelines[maxidx]->tag = tag;
    cachelines[maxidx]->lru = 0;
    eviction++;
    VERBOSE_PRINT("eviction ")
}

void getSize(char *line)
{
    sscanf(line, "%d", &visitSize);
    DEBUG_PRINT("size is %d\n", visitSize);
}

void removeLinefeed(char *line)
{
    for (; *line != '\0'; ++line)
    {
        if (*line == '\n')
        {
            *line = '\0';
            return;
        }
    }
}

void parseFile()
{
    FILE *file = fopen(traceName, "r");
    if (!file)
    {
        perror(traceName);
        exit(EXIT_FAILURE);
    }
    char line[1024] = {0};

    while (fgets(line, sizeof(line), file))
    {
        if (line[0] != ' ')
            continue;
        if (verbose)
        {
            removeLinefeed(line);
            printf("%s ", line + 1);
        }
        char type = line[1];
        getSize(getAddress(line + 3));
        switch (type)
        {
        case 'L':
            visitCache();
            break;
        case 'M':
            visitCache();
            visitCache();
            break;
        case 'S':
            visitCache();
            break;
        default:
            printf("Unknown type: %c\n", type);
            break;
        }
        VERBOSE_PRINT("\n")
    }
}

void initCache()
{
    int lineCount = setLineSize * (1 << setSize);
    cachelines = malloc(sizeof(cacheLine *) * lineCount);
    for (int i = 0; i < lineCount; ++i)
    {
        cachelines[i] = malloc(sizeof(cacheLine));
        memset(cachelines[i], 0, sizeof(cacheLine));
        cachelines[i]->lru = setLineSize;
    }
}

int main(int argc, char *argv[])
{
    parseArgs(argc, argv);
    initCache();
    parseFile();
    printSummary(hit, miss, eviction);
    return 0;
}
