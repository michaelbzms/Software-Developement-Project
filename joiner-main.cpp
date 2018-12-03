#include <iostream>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "Headers/SQLParser.h"
#include "Headers/Relation.h"
#include "Headers/util.h"


#define CHECK(call, msg, actions) { if ( !(call) ) { std::cerr << msg << std::endl; actions } }

#define STARTING_VECTOR_SIZE 16


using namespace std;


/* Local Functions */
unsigned int find_rel_pos(QueryRelation **QueryRelations, unsigned int size, unsigned int rel_id);


int main(){
    cout << "Loading relations..." << endl;
    // first read line-by-line for relations' file names until read "Done"
    vector<string> *fileList = new vector<string>();
    fileList->reserve(STARTING_VECTOR_SIZE);
    string currName;
    while ( getline(cin, currName) && currName != "Done" ) {
        fileList->push_back(currName);
    }
    CHECK( !cin.eof() && !cin.fail() , "Error: reading filenames from cin failed", delete fileList; return -1; )
    CHECK( !fileList->empty() , "Warning: No filenames were given", ; )
    // and load all files into memory
    const int Rlen = (int) fileList->size();
    Relation **R = new Relation *[Rlen]();
    {
        unsigned int i = 0;
        try {
            for (auto iter = fileList->begin(); iter != fileList->end(); iter++, i++) {
                R[i] = new Relation((*iter).c_str());
            }
        } catch (...) {
            cerr << "Error: At least one of the files wasn't found or was inaccessible." << endl;
            for (auto iter = fileList->begin(); iter != fileList->end(); iter++, i++) delete R[i];
            delete fileList;
            return -1;
        }
    }
    delete fileList;
    // wait for 1 second
    cout << "Loading done! Accepting Queries..." << endl;
    sleep(1);
    // then start parsing 'sql' statements
    string currStatement;
    while ( getline(cin, currStatement) && !cin.eof() && !cin.fail() ){
        CHECK( currStatement != "F" , "Warning: Received an empty batch of statements.", continue; )
        do {
            SQLParser *p = new SQLParser(currStatement.c_str());     // example: "0 2 4|0.1=1.2&1.0=2.1&0.1>3000|0.0 1.1";

            bool abort = false;

            // reset all ids in R before every query (TODO: change if we do not use search to find pos in R for rel_id)
            for (unsigned int i = 0 ; i < Rlen ; i++){
                R[i]->setId(-1);   // -1 is 111...1 in unsigned int.
            }

            // execute FROM: load original Relations to an array of pointers to such (which will only get "smaller" during the query)
            QueryRelation **QueryRelations = new QueryRelation*[p->nrelations]();
            int *seen_at = new int[Rlen]();
            for (int i = 0 ; i < Rlen ; i++) seen_at[i] = -1;       // init to -1
            for (unsigned int i = 0; i < p->nrelations ; i++){
                CHECK( p->relations[i] < Rlen, "SQL Error: SQL query contains non-existant Relation in \'FROM\'. Aborting query...", delete[] QueryRelations; delete p; abort = true; break; )
                if ( seen_at[p->relations[i]] == -1 ) {
                    QueryRelations[i] = R[p->relations[i]];
                    R[p->relations[i]]->setId(i);                  // id of relations are in ascending order in 'FROM'
                    seen_at[p->relations[i]] = true;
                } else {
                    unsigned int *temprowids = new unsigned int[R[p->relations[i]]->getSize()];
                    for (unsigned int j = 0 ; j < R[p->relations[i]]->getSize() ; j++){ temprowids[j] = j + 1; }
                    QueryRelations[i] = new IntermediateRelation(i, temprowids, R[p->relations[i]]->getSize(), R[seen_at[p->relations[i]]]);
                    delete[] temprowids;
                }
            }
            delete[] seen_at;
            if (abort) continue;

            // execute WHERE
            // First, do filters
            for (int i = 0 ; i < p->nfilters ; i++){
                //FILTER
                const filter &filter = p->filters[i];
                CHECK( (filter.rel_id < p->nrelations), "SQL Error: SQL filter contains a relation that does not exist in \'FROM\'. Aborting query...",
                       for (int ii = 0 ; ii < p->nrelations; ii++) { if ( QueryRelations[ii] != NULL && QueryRelations[ii]->isIntermediate ) delete QueryRelations[ii]; } delete[] QueryRelations; delete p; abort = true; break; )
                QueryRelations[filter.rel_id] = QueryRelations[filter.rel_id]->performFilter(filter.rel_id, filter.col_id, filter.value, filter.cmp);
            }
            if (abort) continue;
            // Then equal columns operations
            for (int i = 0 ; i < p->npredicates ; i++){
                if ( p->predicates[i].rela_id == p->predicates[i].relb_id && p->predicates[i].cola_id != p->predicates[i].colb_id){
                    // EQUAL COLUMNS UNARY OPERATION
                    const predicate &predicate = p->predicates[i];
                    CHECK( (predicate.rela_id < p->nrelations), "SQL Error: SQL equal columns predicate on one relation that does not exist in \'FROM\'. Aborting query...",
                           for (int ii = 0 ; ii < p->nrelations; ii++) { if ( QueryRelations[ii] != NULL && QueryRelations[ii]->isIntermediate ) delete QueryRelations[ii]; } delete[] QueryRelations; delete p; abort = true; break;)
                    QueryRelations[predicate.rela_id] = QueryRelations[predicate.rela_id]->performEqColumns(predicate.rela_id, predicate.cola_id, predicate.colb_id);
                }
                // else if (  p->predicates[i].rela_id == p->predicates[i].relb_id  ) -> ignore predicate
            }
            if (abort) continue;
            // And afterwards all the joins
            for (int i = 0 ; i < p->npredicates ; i++){
                const predicate &predicate = p->predicates[i];
                CHECK( (predicate.rela_id < p->nrelations && predicate.relb_id < p->nrelations), "SQL Error: SQL join predicate contains a relation that does not exist in \'FROM\'. Aborting query...",
                       for (int ii = 0 ; ii < p->nrelations; ii++) { if ( QueryRelations[ii] != NULL && QueryRelations[ii]->isIntermediate ) delete QueryRelations[ii]; } delete[] QueryRelations; delete p; abort = true; break; )
                unsigned int rela_pos = find_rel_pos(QueryRelations, p->nrelations, predicate.rela_id);
                unsigned int relb_pos = find_rel_pos(QueryRelations, p->nrelations, predicate.relb_id);
                CHECK( rela_pos != -1 && relb_pos != -1, "Warning: Something went wrong joining relations, cannot find intermediate for one. Please debug.", continue; )
                if ( rela_pos != relb_pos ){
                    // JOIN
                    if (rela_pos < relb_pos){
                        QueryRelations[rela_pos] = QueryRelations[rela_pos]->performJoinWith(*QueryRelations[relb_pos], predicate.rela_id, predicate.cola_id, predicate.relb_id, predicate.colb_id);
                        if ( QueryRelations[rela_pos]->isIntermediate && QueryRelations[relb_pos]->isIntermediate ) delete QueryRelations[relb_pos];   // if only QueryRelations[relb_pos] is Intermediate then we do not want to delete it (!)
                        QueryRelations[relb_pos] = NULL;
                    } else {
                        QueryRelations[relb_pos] = QueryRelations[relb_pos]->performJoinWith(*QueryRelations[rela_pos], predicate.relb_id, predicate.colb_id, predicate.rela_id, predicate.cola_id);
                        if ( QueryRelations[relb_pos]->isIntermediate && QueryRelations[rela_pos]->isIntermediate ) delete QueryRelations[rela_pos];
                        QueryRelations[rela_pos] = NULL;
                    }
                } else {   // (!) if two tables are joined two times then the first it will be join whilst the second time it will be an equal columns operation!
                    // EQUAL COLUMNS UNARY OPERATION
                    QueryRelations[rela_pos] = QueryRelations[rela_pos]->performEqColumns(predicate.rela_id, predicate.cola_id, predicate.colb_id);
                }
            }
            if (abort) continue;
            CHECK( QueryRelations[0] != NULL, "Warning: Something not supposed to happen happened. Please debug...",
                   for (int i = 0 ; i < p->nrelations; i++) { if ( QueryRelations[i] != NULL && QueryRelations[i]->isIntermediate ) delete QueryRelations[i]; } delete[] QueryRelations; delete p; break; )
            // Last but not least any cross-products left to do
            while ( count_not_null((void **) QueryRelations, p->nrelations) > 1 ){
                // CROSS PRODUCT: perform cross product between remaining Relations uniting them into one in the leftest position until only one QueryRelation remains
                int i = 1;
                while ( i < p->nrelations && QueryRelations[i] == NULL ) i++;
                if ( i == p->nrelations) { cerr << "Warning: should not happen" << endl; break; }
                QueryRelations[0] = QueryRelations[0]->performCrossProductWith(*QueryRelations[i]);
                if (QueryRelations[0]->isIntermediate && QueryRelations[i]->isIntermediate) delete QueryRelations[i];
                QueryRelations[i] = NULL;
            }

			// Choose one:
            //QueryRelations[0]->performSum(p->projections,p->nprojections);
            QueryRelations[0]->performSelect(p->projections, p->nprojections);

            // cleanup
            for (int i = 0 ; i < p->nrelations; i++){
                if ( QueryRelations[i] != NULL && QueryRelations[i]->isIntermediate ) delete QueryRelations[i];
            }
            delete[] QueryRelations;
            delete p;

            getline(cin, currStatement);
        } while (!cin.eof() && !cin.fail() && currStatement != "F");
        if (cin.eof() || cin.fail()) break;
        cout << endl;
    }
    //CHECK( cin.eof() && !cin.fail(), "Error: reading statements from cin failed", ; )
    // cleanup
    for (unsigned int i = 0 ; i < Rlen ; i++ ) {
        delete R[i];
    }
    delete[] R;
    return 0;
}


/* Local Function Implementation */
unsigned int find_rel_pos(QueryRelation **QueryRelations, unsigned int size, unsigned int rel_id){
    for (unsigned int i = 0 ; i < size ; i++){
        if (QueryRelations[i] != NULL && QueryRelations[i]->containsRelation(rel_id)){
            return i;
        }
    }
    return -1;  // unsigned -> 111..1 -> should be caught by a CHECK(..)
}
