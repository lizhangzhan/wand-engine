#ifndef WAND_ENGINE_POSTING_H
#define WAND_ENGINE_POSTING_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
#include <functional>

template<class T>
class Posting {
public:
    typedef T ElementType;

    struct PostingListNode {
        ElementType * value;
        uint64_t bound;
        uint64_t id;
        PostingListNode * next;

        PostingListNode(): value(0) {}
        PostingListNode(ElementType * _value): value(_value) {}
        ~PostingListNode() {delete value;}
    };

    typedef PostingListNode NodeType;

    struct Iterator {
        NodeType * node;

        explicit Iterator(NodeType * _node): node(_node) {}

        ElementType * next() {
            assert(node);
            ElementType * ret = node->value;
            node = node->next;
            return ret;
        }

        bool hasNext() const {
            return node != 0;
        }
    };

    typedef Iterator IteratorType;

private:
    NodeType * list_;
    uint64_t upper_bound_;
    size_t size_;

public:
    Posting(): list_(0), upper_bound_(0), size_(0) {}

    ~Posting() {
        clear();
    }

    IteratorType iterate() const {
        return Iterator(list_);
    }

    uint64_t getUpperBound() const {
        return upper_bound_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    void clear() {
        NodeType * p = list_;
        NodeType * pp;
        while (p) {
            pp = p;
            p = p->next;
            delete pp;
        }

        list_ = 0;
        upper_bound_ = 0;
        size_ = 0;
    }

    // node and node->value must be on heap, produced by "new"
    // node->value, node->bound, node->id must be filled before insertion
    void insert(NodeType * node) {
        assert(node);
        assert(node->value);

        uint64_t id = node->id;
        NodeType * p = list_;
        if (p == 0 || p->id < id) {
            node->next = p;
            p = node;
        } else {// p->id >= id
            NodeType * pp;
            pp = p;
            p = p->next;
            while (p) {
                if (p->id < id)
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
};

#endif// WAND_ENGINE_POSTING_H
