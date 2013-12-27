#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "inverted.h"
#include <stdint.h>
#include <vector>

template<class InvertedIndex>
class WandOperator {
public:
    typedef typename InvertedIndex::TermType TermType;
    typedef typename InvertedIndex::TermType DocType;
    typedef typename InvertedIndex::PostingListType PostingListType;
    struct DocScore {
        const DocType * doc;
        uint64_t wand_score;
    };

private:
    struct TermPostingList {
        const TermType * term;
        const PostingListType * posting_list;
    };

    const InvertedIndex& ii_;
    const uint64_t score_threshold_;
    const size_t heap_size_;

    size_t skipped_doc_;
    std::vector<TermPostingList> term_posting_lists_;
    std::vector<DocScore> heap_;
    uint64_t current_doc_id_;

private:
    void match_terms(const std::vector<TermType>& terms) {
        size_t s = terms.size();
        for (size_t i = 0; i < s; i++) {
            const PostingListType * posting_list = ii_.find(terms[i]);
            if (posting_list) {
                TermPostingList tpl;
                tpl.term = &terms[i];
                tpl.posting_list = posting_list;
                term_posting_lists_.push_back(tpl);
            }
        }
    }

public:
    explicit WandOperator(
        const InvertedIndex& ii,
        uint64_t score_threshold = 0,
        size_t heap_size = 1000)
        :ii_(ii), score_threshold_(score_threshold), heap_size_(heap_size),
        skipped_doc_(0) {
    }

    void search(const std::vector<TermType>& terms, std::vector<DocScore> * result) {
        match_terms(terms);
        // TODO
    }
};

#endif// WAND_ENGINE_WAND_H
