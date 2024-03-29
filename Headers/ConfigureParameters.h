#ifndef PROJECT_CONFIGUREPARAMETERS_H
#define PROJECT_CONFIGUREPARAMETERS_H


///////////////////////////////
/// JOINER-RHJ  PARAMETERS  ///
///////////////////////////////

#define CACHE 512                  // expected average bucket size: smaller CACHE SIZE means more buckets (=> more parallel joins but also bigger parallel partition cost and less cache exploiting)
#define NUMBER_OF_THREADS 8        // joiner's scheduler thread pool size (Recommended: CPU_CORES * THREADS_PER_CORE)
#define BUFFER_SIZE (128*1024)     // buffer size for result nodes. Recommended: 128KB


///////////////////////////////
/// CHOOSE PARALLEL OPTIONS ///
///////////////////////////////

//#define PARALLEL_IMPLEMENTATION_OF_PARTITION_INSTEAD_OF_THEIR_CALL   // if #defined it seems to be slower
#define DO_FILTER_PARALLEL
#define DO_EQUALCOLUMNS_PARALLEL


///////////////////////////////
///    QUERY OPTIMIZATION   ///
///////////////////////////////
#define DO_QUERY_OPTIMIZATION      // use query optimization or not?
#define PARALLEL_OPTIMIZATION      // run Query Optimization an a different thread while the main thread executes FROM and filters & eq columns
#define JOIN_STATS_FOR_F 2         // 0 -> Cartesian Product | 1 -> Divide by u - l + 1 (NOT Recommended) | 2 -> Divide by MAX(dA, dB) (Recommended)


///////////////////////////////
///        DEBUG MODE?      ///
///////////////////////////////

//#define DDEBUG


#endif
