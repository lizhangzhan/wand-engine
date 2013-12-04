#ifndef WAND_ENGINE_INVERTED_H
#define WAND_ENGINE_INVERTED_H

#include "posting.h"
#include <assert.h>
#include <ext/hash_map>

template<class Term, class Doc, class HashTerm, class EqualTerm, class GetBoundOfDoc, class GetIDOfDoc>
class Inverted {
public:
    typedef Term TermType;
    typedef Doc DocType;
    typedef Posting<DocType> PostingType;
    typedef typename PostingType::NodeType PostingNodeType;
    typedef typename PostingType::IteratorType PostingIteratorType;
    typedef __gnu_cxx::hash_map<TermType, PostingType, HashTerm, EqualTerm> HashTableType;

private:
    HashTableType ht_;
    const GetBoundOfDoc get_bound_;
    const GetIDOfDoc get_id_;

public:
    Inverted(): ht_(), get_bound_(), get_id_() {
    }

    ~Inverted() {
    }

    // doc must be on heap, produced by "new"
    void insert(const TermType& term, DocType * doc) {
        assert(doc);

        PostingNodeType * node = new PostingNodeType(doc);
        node->bound = get_bound_(*doc);
        node->id = get_id_(*doc);

        PostingType& posting = ht_[term];
        posting.insert(node);
    }

private:

};

#endif// WAND_ENGINE_INVERTED_H
