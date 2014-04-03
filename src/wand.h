#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "index.h"
#include <ostream>
#include <set>
#include <vector>

class Wand {
public:
    struct DocIdScore {
        IdType doc_id;
        ScoreType score;

        DocIdScore() {}
        DocIdScore(IdType _doc_id, ScoreType _score) : doc_id(_doc_id), score(_score) {}

        std::ostream& dump(std::ostream& os) const;
    };

    struct DocIdScore_ScoreLess {
        bool operator()(const DocIdScore& a, const DocIdScore& b) const {
            return a.score < b.score;
        }
    };

    struct DocIdScore_ScoreGreat {
        bool operator()(const DocIdScore& a, const DocIdScore& b) const {
            return a.score > b.score;
        }
    };

    struct TermPostingList {
        IdType term_id;
        const PostingList * posting_list;
        PostingListNode * current;
        size_t remains;
        ScoreType weight_in_query;

        std::ostream& dump(std::ostream& os) const;
    };

    struct TermPostingList_DocIdLess {
        bool operator()(const TermPostingList& a, const TermPostingList& b) const {
            return a.current->doc->id < b.current->doc->id;
        }
    };

private:
    typedef std::vector<TermPostingList> TermPostingListSetType;
    typedef std::multiset<DocIdScore, DocIdScore_ScoreLess> DocHeapType;
    const InvertedIndex& ii_;
    const size_t heap_size_;
    const ScoreType threshold_;
    size_t skipped_doc_;
    IdType current_doc_id_;
    ScoreType current_threshold_;
    TermPostingListSetType term_posting_list_set_;
    DocHeapType doc_heap_;
    int verbose_;

private:
    static ScoreType dot_product(const TermVector& query, const TermVector& doc);
    static ScoreType full_evaluate(const TermVector& query, const Document * doc);
    void match_terms(const TermVector& query);
    void advance_term_posting_list(size_t to_advance, IdType doc_id);
    bool find_pivot(size_t * pivot) const;
    size_t pick_term(size_t pivot) const;
    bool next(size_t * next_term);

    void clean() {
        skipped_doc_ = 0;
        current_doc_id_ = 0;
        current_threshold_ = threshold_;
        term_posting_list_set_.clear();
        doc_heap_.clear();
    }

public:
    explicit Wand(
        const InvertedIndex& ii,
        size_t heap_size = 1000,
        ScoreType threshold = 0)
        : ii_(ii), heap_size_(heap_size), threshold_(threshold),
        skipped_doc_(0), current_doc_id_(0),
        current_threshold_(threshold),
        term_posting_list_set_(), doc_heap_(),
        verbose_(0) {
    }

    void search(TermVector& query, std::vector<DocIdScore> * result);
    // only for comparison
    void search_taat_v1(TermVector& query, std::vector<DocIdScore> * result) const;
    void search_taat_v2(TermVector& query, std::vector<DocIdScore> * result) const;

    void set_verbose(int verbose) {
        verbose_ = verbose;
    }

    std::ostream& dump(std::ostream& os) const;
private:
    Wand(Wand& other);
    Wand& operator=(Wand& other);
};

std::ostream& operator << (std::ostream& os, const Wand::DocIdScore& doc);
std::ostream& operator << (std::ostream& os, const Wand::TermPostingList& term);
std::ostream& operator << (std::ostream& os, const Wand& wand);

#endif// WAND_ENGINE_WAND_H
