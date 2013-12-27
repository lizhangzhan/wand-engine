#ifndef WAND_ENGINE_POSTING_H
#define WAND_ENGINE_POSTING_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
#include <functional>
#include <ostream>

template<class T>
class PostingList {
public:
    struct PostingListNode {
        T * value;
        uint64_t bound;// bound value used to estimate upper bound
        uint64_t id;// usually, doc id
        PostingListNode * next;

        PostingListNode(): value(0) {}
        ~PostingListNode() {delete value;}

        void dump(std::ostream& os) const {
            os << "    " << *value << ", " << bound << ", " << id << std::endl;
        }
    };
    typedef PostingListNode NodeType;

    static NodeType * get_node() {
        return new PostingListNode();
    }

    static void put_node(NodeType * node) {
        delete node;
    }

    struct Iterator {
        NodeType * node;

        explicit Iterator(NodeType * _node): node(_node) {}

        T * value() {
            assert(node);
            T * ret = node->value;
            node = node->next;
            return ret;
        }

        bool has_next() const {
            return node != 0;
        }
    };
    typedef Iterator IteratorType;

private:
    NodeType * list_;
    uint64_t upper_bound_;
    size_t size_;

public:
    PostingList(): list_(0), upper_bound_(0), size_(0) {}

    ~PostingList() {
        clear();
    }

    IteratorType iterate() const {
        return Iterator(list_);
    }

    uint64_t get_upper_bound() const {
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
            put_node(pp);
        }

        list_ = 0;
        upper_bound_ = 0;
        size_ = 0;
    }

    // node and node->value must be on heap, produced by "get_node"
    // node->value, node->bound, node->id must be filled before insertion
    void insert(NodeType * node) {
        assert(node);
        assert(node->value);

        uint64_t id = node->id;
        NodeType * p = list_;
        if (p == 0 || p->id >= id) {
            node->next = list_;
            list_ = node;
        } else {// p->id < id
            NodeType * pp = p;
            p = p->next;
            while (p) {
                if (p->id >= id)
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

    void dump(std::ostream& os) const {
        os << "  upper bound: " << upper_bound_ << std::endl;
        os << "  size: " << size_ << std::endl;
        NodeType * p = list_;
        NodeType * pp;
        while (p) {
            pp = p;
            p = p->next;
            pp->dump(os);
        }
    }
};

#endif// WAND_ENGINE_POSTING_H
