#include "Headers/RadixHashJoin.h"
#include <stdio.h>

#define try(a) fprintf(stdout,"\n%s\n",#a);a();
#define assert(a) if (!(a)) fprintf(stderr,"assert %s failed\n",#a);

void test_partition(){
    intField *joinField = new intField[7];
    joinField[0] = 'a';
    joinField[1] = 'b';
    joinField[2] = 'a';
    joinField[3] = 'a';
    joinField[4] = 'c';
    joinField[5] = 'b';
    joinField[6] = 'c';
    unsigned int *rowids = new unsigned int[7];
    for (unsigned int i = 0 ; i < 7 ; i++) { rowids[i] = i+1; }
    Relation *R = new Relation(7, joinField, rowids);
    assert( R->partitionRelation() )
    R->printDebugInfo();
    delete R;
}

void test_index() {
	int *chain,*table;
	intField bucket[10]={3,1,17,23,12,127,123,2,3,10};
	
	assert(indexRelation(bucket,10,chain,table));	
	assert(table[0]==8);
	assert(chain[1]==-1);
	assert(chain[2]==-1);
	assert(chain[3]==2);
	
	delete[] chain;
	delete[] table;
}

int main() {
	try(test_partition);
	try(test_index);
	return 0;
}

