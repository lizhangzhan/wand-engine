#ifndef WAND_ENGINE_POSTING_H
#define WAND_ENGINE_POSTING_H

#include <assert.h>
#include <stdint.h>

template<class Value, class UpperBoundOfValue>
class Posting {
public:
    typedef Value ElementType;

    struct PostingListNode {
        ElementType value;
        PostingListNode * next;
    };

    typedef PostingListNode NodeType;

    struct Iterator {
        NodeType * node;

        explicit Iterator(NodeType * _node): node(_node) {}

        ElementType * next() {
            assert(node);
            ElementType * ret = &node->value;
            node = node->next;
            return ret;
        }

        bool hasNext() const {
            return node != 0;
        }
    };

    typedef Iterator IteratorType;

private:
    uint64_t upper_bound_;
    NodeType * list_;
    UpperBoundOfValue up_of_value_;

public:
    Posting(): upper_bound_(0), list_(0) {}

    IteratorType iterate() {
        return Iterator(list_);
    }

    uint64_t getUpperBound() const {
        return upper_bound_;
    }

    void setUpperBound(uint64_t upper_bound) {
        upper_bound_ = upper_bound;
    }
};

#endif// WAND_ENGINE_POSTING_H
