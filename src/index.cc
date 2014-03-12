#include "index.h"

#if defined HAVE_STD_TR1_UNORDERED_MAP
# include <tr1/unordered_map>
# define HASH_MAP std::tr1::unordered_map
#else
# include <unordered_map>
# define HASH_MAP std::unordered_map
#endif

std::ostream& PostingListNode::dump(std::ostream& os) const {
    os << *doc;
    if (!doc->is_sentinel()) {
        os << "      bound: " << bound << "\n";
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
        if (id > upper_id_ && last_) {
            // directly put at the back of list
            node->next = last_->next;
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

    if (size_ == 1) {
        last_ = first_;
    }
}

std::ostream& PostingList::dump(std::ostream& os) const {
    os << "  posting list size: " << size_ << ", upper bound: " << upper_bound_ << "\n";
    PostingListNode * p = first_;
    PostingListNode * pp;
    while (p) {
        pp = p;
        p = p->next;
        os << *pp;
    }
    return os;
}

class InvertedIndex::Impl {
private:
    typedef HASH_MAP<IdType, PostingList *> HashTableType;
    HashTableType ht_;

public:
    Impl() : ht_() {}

    ~Impl() {
        clear();
    }

    void insert(Document * doc);
    const PostingList * find(IdType term_id) const;
    void clear();
    std::ostream& dump(std::ostream& os) const;
};

void InvertedIndex::Impl::insert(Document * doc) {
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

const PostingList * InvertedIndex::Impl::find(IdType term_id) const {
    HashTableType::const_iterator it = ht_.find(term_id);
    if (it == ht_.end()) {
        return 0;
    } else {
        return (*it).second;
    }
}

void InvertedIndex::Impl::clear() {
    HashTableType::iterator it = ht_.begin();
    HashTableType::iterator last = ht_.end();
    for (; it != last; ++it) {
        delete (*it).second;
    }
    ht_.clear();
}

std::ostream& InvertedIndex::Impl::dump(std::ostream& os) const {
    HashTableType::const_iterator it = ht_.begin();
    HashTableType::const_iterator last = ht_.end();
    for (; it != last; ++it) {
        const IdType& term_id = (*it).first;
        const PostingList& posting_list = *(*it).second;
        os << "term id: " << term_id << "\n";
        os << posting_list << "\n";
    }
    return os;
}

InvertedIndex::InvertedIndex() {
    impl_ = new Impl();
}

InvertedIndex::~InvertedIndex() {
    delete impl_;
}

void InvertedIndex::insert(Document * doc) {
    impl_->insert(doc);
}

const PostingList * InvertedIndex::find(IdType term_id) const {
    return impl_->find(term_id);
}

void InvertedIndex::clear() {
    impl_->clear();
}

std::ostream& InvertedIndex::dump(std::ostream& os) const {
    return impl_->dump(os);
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
