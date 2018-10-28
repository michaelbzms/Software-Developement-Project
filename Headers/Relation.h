#ifndef RELATION_H
#define RELATION_H

#include "FieldTypes.h"

#define CPU_CACHE 4096
#define H2SIZE 3

unsigned int H1(intField, unsigned int N);

class Relation {
	private:
		unsigned int size;
        intField *joinField;
        unsigned int *rowids;
        unsigned int *Psum;
        unsigned int numberOfBuckets;
    public:
		Relation(unsigned int _size, const intField *_joinField, const unsigned int *_rowids);
		~Relation();
		/* Accessors  */
        unsigned int getSize() const { return size; }
        unsigned int getBuckets() const { return numberOfBuckets; }
        unsigned int *getIds(int i) const { return rowids+Psum[i]; }
        intField *getField(int i) const { return joinField+Psum[i]; }
        unsigned int getBucketSize(int i) const { return i==numberOfBuckets-1 ? size-Psum[i] : Psum[i+1]-Psum[i]; }
        /* Operations */
		bool partitionRelation(unsigned int avg_bucket_size, unsigned int forced_H1_N = 0);   // partitions Relation by creating Psum and reordering it's tuples
        /* Debug */
        void printDebugInfo();
    private:
        unsigned int pickH1_N(unsigned int avg_bucket_size);
};

#endif
