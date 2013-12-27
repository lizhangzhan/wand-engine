#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "index.h"
#include <algorithm>
#include <ostream>
#include <set>


class Wand {
public:
    struct DocScore {
        const Document * doc;
        ScoreType score;
    };

private:
    struct DocScoreLess {
        bool operator()(const DocScore& a, const DocScore& b) const {
            return a.score < b.score;
        }
    };

    struct TermPostingList {
        IdType term_id;
        const PostingList * posting_list;
        PostingListNode * current;
        size_t remains;
        ScoreType term_weight_in_query;
    };

    struct TermPostingListFirstId {
        bool operator()(const TermPostingList& a, const TermPostingList& b) const {
            return a.current->doc->id < b.current->doc->id;
        }
    };

private:
    const InvertedIndex& ii_;
    const size_t heap_size_;
    size_t skipped_doc_;
    IdType current_doc_id_;
    std::vector<TermPostingList> term_posting_lists_;
    std::set<DocScore, DocScoreLess> heap_;
    ScoreType heap_min_score_;

private:
    static ScoreType dot_product(const std::vector<Term>& query, const std::vector<Term>& doc);
    static ScoreType dot_product(const std::vector<Term>& query, const Document * doc);
    void match_terms(const std::vector<Term>& query);
    void sort_term_posting_lists();
    void advance_term_posting_lists(TermPostingList * tpl, IdType doc_id);
    bool find_pivot_term_index(size_t * index) const;
    size_t pick_term_index(size_t left, size_t right) const;
    bool next(IdType * next_doc_id, size_t * term_index);

public:
    explicit Wand(
        const InvertedIndex& ii,
        size_t heap_size = 1000)
        :ii_(ii), heap_size_(heap_size),
        skipped_doc_(0), current_doc_id_(0), heap_min_score_(0) {
    }

    void search(std::vector<Term>& query, std::vector<DocScore> * result);

private:
    Wand(Wand& other);
    Wand& operator=(Wand& other);
};

#endif// WAND_ENGINE_WAND_H
