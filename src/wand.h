#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "inverted.h"
#include <stdint.h>
#include <vector>
#include <algorithm>

template<class InvertedIndex>
class WandOperator {
public:
    typedef typename InvertedIndex::TermType TermType;
    typedef typename InvertedIndex::TermType DocType;
    typedef typename InvertedIndex::PostingListType PostingListType;
    typedef typename InvertedIndex::PostingListNodeType PostingListNodeType;
    struct DocScore {
        const DocType * doc;
        UBType wand_score;
    };

private:
    static const DocIdType LAST_ID = (DocIdType)-1;

    struct TermPostingList {
        const TermType * term;
        const PostingListType * posting_list;
        PostingListNodeType * current_doc;
    };

    struct TermPostingListFirstId {
        bool operator()(const TermPostingList& a, const TermPostingList& b) const {
            return a.current_doc->id < b.current_doc->id;
        }
    };

    const InvertedIndex& ii_;
    const UBType score_threshold_;
    const size_t heap_size_;

    size_t skipped_doc_;
    DocIdType current_doc_id_;
    UBType theta_;
    std::vector<TermPostingList> term_posting_lists_;
    std::vector<DocScore> heap_;

private:
    void match_terms(const std::vector<TermType>& terms) {
        size_t s = terms.size();
        for (size_t i = 0; i < s; i++) {
            const PostingListType * posting_list = ii_.find(terms[i]);
            if (posting_list) {
                PostingListNodeType * current_doc = posting_list->front();
                if (current_doc) {
                    TermPostingList tpl;
                    tpl.term = &terms[i];
                    tpl.posting_list = posting_list;
                    tpl.current_doc = current_doc;
                    term_posting_lists_.push_back(tpl);
                }
            }
        }
    }

    void sort_term_posting_lists() {
        std::sort(term_posting_lists_.begin(), term_posting_lists_.end(),
            TermPostingListFirstId());
    }

    DocIdType advance_term_posting_lists(TermPostingList * term_posting_lists, DocIdType doc_id) {
        PostingListNodeType * current_doc = term_posting_lists->current_doc;
        while (current_doc->id < doc_id) {
            current_doc = current_doc->next;
            skipped_doc_++;
        }
        // current_doc->id >= doc_id
        return current_doc->id;
    }

    bool find_pivot_term_index(size_t * index) const {
        UBType acc_upper_bound = 0;
        size_t s = term_posting_lists_.size();
        for (size_t i = 0; i < s; i++) {
            acc_upper_bound += term_posting_lists_[i].posting_list->get_upper_bound();
            if (acc_upper_bound >= theta_) {
                *index = i;
                return true;
            }
        }
        return false;
    }

    size_t pick_term_index(size_t left, size_t right) {
        // We can have many strategies to pick a term.
        // One rule is: picking this term will skip more documents.
        // In this function, we choose the longest posting list.
        size_t size = 0;
        size_t index = -1;
        for (size_t i = left; i < right; i++) {
            size_t tmp = term_posting_lists_[i].posting_list->size();
            if (tmp > size) {
                size = tmp;
                index = i;
            }
        }
        return index;
    }

public:
    explicit WandOperator(
        const InvertedIndex& ii,
        UBType score_threshold = 0,
        size_t heap_size = 1000)
        :ii_(ii), score_threshold_(score_threshold), heap_size_(heap_size),
        skipped_doc_(0), current_doc_id_(0), theta_(0) {
    }

    void search(const std::vector<TermType>& terms, std::vector<DocScore> * result) {
        match_terms(terms);
        if (term_posting_lists_.empty()) {
            return;
        }

        sort_term_posting_lists();
        // TODO
    }

private:
    WandOperator(WandOperator& other);
    WandOperator& operator=(WandOperator& other);
};

#endif// WAND_ENGINE_WAND_H
