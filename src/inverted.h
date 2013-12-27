#ifndef WAND_ENGINE_INVERTED_H
#define WAND_ENGINE_INVERTED_H

#include "posting.h"
#include <assert.h>
#include <unordered_map>
#include <ostream>

template<class Term, class Doc, class HashTerm, class EqualTerm, class GetBoundOfDoc, class GetIDOfDoc>
class InvertedIndex {
public:
    typedef Term TermType;
    typedef Doc DocType;
    typedef PostingList<Doc> PostingListType;
    typedef typename PostingListType::NodeType PostingListNodeType;
    typedef typename PostingListType::IteratorType PostingListIteratorType;
    typedef std::unordered_map<Term, PostingListType, HashTerm, EqualTerm> HashTableType;

private:
    HashTableType ht_;
    const GetBoundOfDoc get_bound_;
    const GetIDOfDoc get_id_;

public:
    InvertedIndex(): ht_(), get_bound_(), get_id_() {
    }

    ~InvertedIndex() {
    }

    // doc must be on heap, produced by "new"
    void insert(const Term& term, Doc * doc) {
        assert(doc);

        PostingListNodeType * node = PostingListType::get_node();
        node->value = doc;
        node->bound = get_bound_(*doc);
        node->id = get_id_(*doc);

        PostingListType& posting = ht_[term];
        posting.insert(node);
    }

    const PostingListType * find(const Term& term) const {
        HashTableType::const_iterator it = ht_.find(term);

        if (it == ht_.end()) {
            return 0;
        }
        else {
            return &(*it).second;
        }
    }

    void dump(std::ostream& os) const {
        HashTableType::const_iterator it = ht_.begin();
        HashTableType::const_iterator last = ht_.end();
        for (; it != last; ++it) {
            const Term& term = (*it).first;
            const PostingListType& posting_list = (*it).second;
            os << "term: " << term << std::endl;
            posting_list.dump(os);
        }
    }
};

#endif// WAND_ENGINE_INVERTED_H
