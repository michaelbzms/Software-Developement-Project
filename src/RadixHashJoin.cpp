#include <iostream>
#include <cstring>
#include <cmath>
#include "../Headers/RadixHashJoin.h"

using namespace std;

#define CHECK(call, msg, action) { if ( ! (call) ) { cerr << msg << endl; action } }
#define MAX(A, B) ( (A) > (B) ? (A) : (B) )

/* Local Functions */
// TODO: read them here instead of RadixHashJoin.h ?

unsigned int H2_N;
unsigned int H2(intField value) { return value&((1<<H2_N)-1); }

Result* radixHashJoin(Relation &R, Relation &S) {
    // Partition R and S, whilst keeping a 'Psum' table for each bucket in R and S (phase 1)
    int H1_N = (unsigned int) (log2( MAX(R.getSize(), S.getSize()) / L2 )+0.5); // H1_N is the same for both Relations round up
    H2_N=H1_N/2;
    CHECK( R.partitionRelation(H1_N) , "partitioning R failed", return NULL; )
    CHECK( S.partitionRelation(H1_N) , "partitioning S failed", return NULL; )
    // Define a Result object to fill
    Result *result = new Result;
    // Choose one of them for indexing, lets say I, and keep the other for scanning, lets say L
    bool saveLfirst;
    Relation *L = NULL, *I = NULL;
    if (R.getSize() < S.getSize()){   // index the smaller of the two Relations
        I = &R;
        L = &S;
        saveLfirst = true;
    } else{
        I = &S;
        L = &R;
        saveLfirst = false;
    }
    // Iteratively perform phases 2 and 3 for all buckets of both similarly partitioned Relations and add results bucket by bucket
	for (unsigned int i = 0 ; i < I->getNumberOfBuckets() ; i++) {
		unsigned int *chain = NULL, *table = NULL;
		CHECK ( indexRelation(I->getJoinFieldBucket(i), I->getBucketSize(i), chain, table) , "indexing of a bucket failed", delete[] chain; delete[] table; delete result; return NULL; )
		CHECK( probeResults(L->getJoinFieldBucket(i), L->getRowIdsBucket(i), I->getJoinFieldBucket(i), I->getRowIdsBucket(i), chain, table, L->getBucketSize(i), result, saveLfirst) , "probing a bucket for results failed", delete[] chain; delete[] table; delete result; return NULL; )
		delete[] chain;
		delete[] table;
	}
    return result;
}

/* Local Function Implementation */
// phase 2: index I's given bucket by creating 'H2HashTable' and 'chain' structures
bool indexRelation(intField *bucketJoinField, unsigned int bucketSize, unsigned int *&chain, unsigned int *&table){
    table = new unsigned int[L1];
    chain = new unsigned int[bucketSize];
    unsigned int *last = new unsigned int[L1];
    memset(table, 0, L1 * sizeof(int));
    memset(chain, 0, bucketSize * sizeof(int));
    for (unsigned int i = bucketSize ; i > 0 ; --i) {
        unsigned int h = H2(bucketJoinField[i - 1]);
        if (table[h] == 0) last[h] = table[h] = i;
        else last[h] = chain[last[h] - 1] = i;
    }
    delete[] last;
    return true;
}

// phase 3: probe linearly L's given bucket and add results based on equal values in indexed I's given bucket
bool probeResults(const intField *LbucketJoinField, const unsigned int *LbucketRowIds,
                  const intField *IbucketJoinField, const unsigned int *IbucketRowIds,
                  const unsigned int *chain, const unsigned int *H2HashTable,
                  unsigned int LbucketSize, Result *result, bool saveLfirst){
	for (unsigned int i = 0; i < LbucketSize; i++) {
		unsigned int h = H2(LbucketJoinField[i]);
		if (H2HashTable[h] == 0) continue;		// no records in I with that value
		unsigned int chainIndex = H2HashTable[h];
		while (chainIndex > 0) {
			if (LbucketJoinField[i] == IbucketJoinField[chainIndex - 1]) {
			    if (saveLfirst) result->addTuple(IbucketRowIds[chainIndex - 1], LbucketRowIds[i]);
			    else result->addTuple(LbucketRowIds[i], IbucketRowIds[chainIndex - 1]);
			}
			chainIndex = chain[chainIndex - 1];
		}
	}
	return true;
}
