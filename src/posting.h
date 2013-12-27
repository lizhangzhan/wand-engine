#ifndef WAND_ENGINE_POSTING_H
#define WAND_ENGINE_POSTING_H

#include "document.h"
#include <ostream>


struct PostingListNode {
    Document * doc;
    ScoreType bound;// bound value used to estimate upper bound
    PostingListNode * next;

    std::ostream& dump(std::ostream& os) const {
        doc->dump(os);
        if (!doc->is_sentinel()) {
            os << "      bound: " << bound << std::endl;
        }
        return os;
    }

    static PostingListNode * get_node() {
        return new PostingListNode();
    }

    static PostingListNode * get_sentinel_node() {
        PostingListNode * sentinel = get_node();
        sentinel->doc = Document::sentinel();
        sentinel->bound = 0;
        sentinel->next = 0;
        return sentinel;
    }

    static void put_node(PostingListNode * node) {
        delete node;
    }

private:
    PostingListNode(): doc(0) {}
    ~PostingListNode() {
        if (doc) {
            doc->release_ref();
        }
    }

private:
    PostingListNode(PostingListNode& other);
    PostingListNode& operator=(PostingListNode& other);
};


class PostingList {
private:
    PostingListNode * list_;
    ScoreType upper_bound_;
    size_t size_;

public:
    PostingList(): list_(PostingListNode::get_sentinel_node()), upper_bound_(0), size_(0) {
    }

    ~PostingList() {
        PostingListNode * p = list_;
        PostingListNode * pp;
        while (p) {
            pp = p;
            p = p->next;
            PostingListNode::put_node(pp);
        }
    }

    PostingListNode * front() const {
        return list_;
    }

    ScoreType get_upper_bound() const {
        return upper_bound_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    // node and node->doc must be produced by "get_node"
    // node->doc, node->bound must be filled before insertion
    void insert(PostingListNode * node) {
        IdType id = node->doc->id;
        PostingListNode * p = list_;
        if (p->doc == 0 || p->doc->id >= id) {
            node->next = list_;
            list_ = node;
        } else {// p->doc->id < id
            PostingListNode * pp = p;
            p = p->next;
            while (p) {
                if (p->doc == 0 || p->doc->id >= id)
                    break;
                pp = p;
                p = p->next;
            }
            node->next = p;
            pp->next = node;
        }

        upper_bound_ = std::max(upper_bound_, node->bound);
        size_++;
    }

    std::ostream& dump(std::ostream& os) const {
        os << "  posting list size: " << size_ << ", upper bound: " << upper_bound_ << std::endl;
        PostingListNode * p = list_;
        PostingListNode * pp;
        while (p) {
            pp = p;
            p = p->next;
            pp->dump(os);
        }
        return os;
    }

private:
    PostingList(PostingList& other);
    PostingList& operator=(PostingList& other);
};

#endif// WAND_ENGINE_POSTING_H
