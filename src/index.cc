#include "index.h"

std::ostream& PostingListNode::dump(std::ostream& os) const {
    os << *doc;
    if (!doc->is_sentinel()) {
        os << "      bound: " << bound << std::endl;
    }
    return os;
}

void PostingList::insert(PostingListNode * node) {
    IdType id = node->doc->id;
    PostingListNode * p = first_;

    if (id <= p->doc->id) {
        node->next = first_;
        first_ = node;
    } else {
        // id > p->doc->id
        if (id > upper_id_) {
            // directly put at the back of list
            node->next = 0;
            last_->next = node;
            last_ = node;
            upper_id_ = id;
        } else {
            // linear search the right place to put
            PostingListNode * pp = p;
            p = p->next;
            while (p) {
                if (p->doc->id >= id)
                    break;
                pp = p;
                p = p->next;
            }
            node->next = p;
            pp->next = node;
        }
    }

    upper_bound_ = std::max(upper_bound_, node->bound);
    size_++;
}

std::ostream& PostingList::dump(std::ostream& os) const {
    os << "  posting list size: " << size_ << ", upper bound: " << upper_bound_ << std::endl;
    PostingListNode * p = first_;
    PostingListNode * pp;
    while (p) {
        pp = p;
        p = p->next;
        os << *pp;
    }
    return os;
}

void InvertedIndex::insert(Document * doc) {
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

const PostingList * InvertedIndex::find(IdType term_id) const {
    HashTableType::const_iterator it = ht_.find(term_id);
    if (it == ht_.end()) {
        return 0;
    } else {
        return (*it).second;
    }
}

void InvertedIndex::clear() {
    HashTableType::iterator it = ht_.begin();
    HashTableType::iterator last = ht_.end();
    for (; it != last; ++it) {
        delete (*it).second;
    }
    ht_.clear();
}

std::ostream& InvertedIndex::dump(std::ostream& os) const {
    HashTableType::const_iterator it = ht_.begin();
    HashTableType::const_iterator last = ht_.end();
    for (; it != last; ++it) {
        const IdType& term_id = (*it).first;
        const PostingList& posting_list = *(*it).second;
        os << "term id: " << term_id << std::endl;
        os << posting_list << std::endl;
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const PostingListNode& node) {
    node.dump(os);
    return os;
}

std::ostream& operator << (std::ostream& os, const PostingList& pl) {
    pl.dump(os);
    return os;
}

std::ostream& operator << (std::ostream& os, const InvertedIndex& ii) {
    ii.dump(os);
    return os;
}
