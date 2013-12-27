#ifndef WAND_ENGINE_POSTING_H
#define WAND_ENGINE_POSTING_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
#include <functional>
#include <ostream>


typedef uint64_t UBType;
typedef uint64_t DocIdType;


template<class T>
class PostingListNode {
public:
    T * value;
    UBType bound;// bound value used to estimate upper bound
    DocIdType id;// usually, doc id
    PostingListNode * next;

    void dump(std::ostream& os) const {
        if (value) {
            os << "    " << *value << ", " << bound << ", " << id << std::endl;
        } else {
            os << "    (sentinel)" << bound << ", " << id << std::endl;
        }
    }

    bool is_sentinel() const {
        return value == 0 && bound == (UBType)-1 && id == (DocIdType)-1;
    }

    static PostingListNode * get_node() {
        return new PostingListNode();
    }

    static PostingListNode * get_sentinel_node() {
        PostingListNode * sentinel = get_node();
        sentinel->value = 0;
        sentinel->bound = (UBType)-1;
        sentinel->id = (DocIdType)-1;
        sentinel->next = 0;
        return sentinel;
    }

    static void put_node(PostingListNode * node) {
        delete node;
    }
private:
    PostingListNode(): value(0) {}
    ~PostingListNode() {delete value;}
    PostingListNode(PostingListNode& other);
    PostingListNode& operator=(PostingListNode& other);
};


template<class T>
class PostingList {
public:
    typedef PostingListNode<T> NodeType;

    static inline NodeType * get_node() {
        return NodeType::get_node();
    }

    static inline NodeType * get_sentinel_node() {
        return NodeType::get_sentinel_node();
    }

    static inline void put_node(NodeType * node) {
        NodeType::put_node(node);
    }

private:
    NodeType * list_;
    UBType upper_bound_;
    size_t size_;

public:
    PostingList(): list_(get_sentinel_node()), upper_bound_(0), size_(0) {
    }

    ~PostingList() {
        NodeType * p = list_;
        NodeType * pp;
        while (p) {
            pp = p;
            p = p->next;
            put_node(pp);
        }
    }

    NodeType * front() const {
        return list_;
    }

    UBType get_upper_bound() const {
        return upper_bound_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    // node and node->value must be on heap, produced by "get_node"
    // node->value, node->bound, node->id must be filled before insertion
    void insert(NodeType * node) {
        assert(node);
        assert(node->value);

        DocIdType id = node->id;
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

private:
    PostingList(PostingList& other);
    PostingList& operator=(PostingList& other);
};

#endif// WAND_ENGINE_POSTING_H
