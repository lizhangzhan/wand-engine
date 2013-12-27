#ifndef WAND_ENGINE_INVERTED_H
#define WAND_ENGINE_INVERTED_H

#include "posting.h"
#include <ostream>
#include <unordered_map>


class InvertedIndex {
public:
    typedef std::unordered_map<IdType, PostingList *> HashTableType;

private:
    HashTableType ht_;

public:
    InvertedIndex(): ht_() {
    }

    ~InvertedIndex() {
        clear();
    }

    // callers can't use doc any more.
    void insert(Document * doc) {
        size_t term_size = doc->terms.size();
        for (size_t i = 0; i < term_size; i++) {
            const Term& term = doc->terms[i];

            PostingListNode * node = PostingListNode::get_node();
            node->doc = doc;
            doc->add_ref();
            node->bound = term.weight;

            PostingList *& posting = ht_[term.id];
            if (posting == 0) {
                posting = new PostingList();
            }
            posting->insert(node);
        }

        doc->release_ref();
    }

    const PostingList * find(IdType term_id) const {
        HashTableType::const_iterator it = ht_.find(term_id);
        if (it == ht_.end()) {
            return 0;
        } else {
            return (*it).second;
        }
    }

    void clear() {
        HashTableType::iterator it = ht_.begin();
        HashTableType::iterator last = ht_.end();
        for (; it != last; ++it) {
            delete (*it).second;
        }
        ht_.clear();
    }

    std::ostream& dump(std::ostream& os) const {
        HashTableType::const_iterator it = ht_.begin();
        HashTableType::const_iterator last = ht_.end();
        for (; it != last; ++it) {
            const IdType& term_id = (*it).first;
            const PostingList& posting_list = *(*it).second;
            os << "term id: " << term_id << std::endl;
            posting_list.dump(os);
        }
        return os;
    }

private:
    InvertedIndex(InvertedIndex& other);
    InvertedIndex& operator=(InvertedIndex& other);
};

#endif// WAND_ENGINE_INVERTED_H
