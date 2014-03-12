#ifndef WAND_ENGINE_INDEX_H
#define WAND_ENGINE_INDEX_H

#include "document.h"
#include <ostream>

struct PostingListNode {
    Document * doc;
    ScoreType bound;// bound value used to estimate upper bound
    PostingListNode * next;

    std::ostream& dump(std::ostream& os) const;

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
    PostingListNode() : doc(0) {}
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
    PostingListNode * first_;
    PostingListNode * last_;
    IdType upper_id_;
    ScoreType upper_bound_;
    size_t size_;

public:
    PostingList() :
        first_(PostingListNode::get_sentinel_node()),
        last_(0),
        upper_id_(0),
        upper_bound_(0),
        size_(0) {
        }

    ~PostingList() {
        PostingListNode * p = first_;
        PostingListNode * pp;
        while (p) {
            pp = p;
            p = p->next;
            PostingListNode::put_node(pp);
        }
    }

    PostingListNode * front() const {
        return first_;
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
    void insert(PostingListNode * node);
    std::ostream& dump(std::ostream& os) const;

private:
    PostingList(PostingList& other);
    PostingList& operator=(PostingList& other);
};

class InvertedIndex {
private:
    class Impl;
    Impl * impl_;
public:
    InvertedIndex();
    ~InvertedIndex();

    // callers can't use "doc" any more.
    void insert(Document * doc);
    const PostingList * find(IdType term_id) const;
    void clear();
    std::ostream& dump(std::ostream& os) const;

private:
    InvertedIndex(InvertedIndex& other);
    InvertedIndex& operator=(InvertedIndex& other);
};

std::ostream& operator << (std::ostream& os, const PostingListNode& doc);
std::ostream& operator << (std::ostream& os, const PostingList& doc);
std::ostream& operator << (std::ostream& os, const InvertedIndex& doc);

#endif// WAND_ENGINE_INDEX_H
