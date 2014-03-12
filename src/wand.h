#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "index.h"
#include <algorithm>
#include <ostream>
#include <set>
#include <map>


class Wand {
public:
    struct DocScore {
        const Document * doc;
        ScoreType score;

        std::ostream& dump(std::ostream& os) const;
    };

    struct DocScore_ScoreLess {
        bool operator()(const DocScore& a, const DocScore& b) const {
            return a.score < b.score;
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

    struct TermPostingListFirstId {
        bool operator()(const TermPostingList& a, const TermPostingList& b) const {
            return a.current->doc->id < b.current->doc->id;
        }
    };

private:
    typedef std::multiset<DocScore, DocScore_ScoreLess> HeapType;
    const InvertedIndex& ii_;
    const size_t heap_size_;
    const ScoreType threshold_;
    size_t skipped_doc_;
    IdType current_doc_id_;
    ScoreType current_threshold_;
    std::vector<TermPostingList> term_posting_lists_;
    HeapType heap_;
    int verbose_;

private:
    static ScoreType dot_product(const std::vector<Term>& query, const std::vector<Term>& doc);
    static ScoreType full_evaluate(const std::vector<Term>& query, const Document * doc);
    void match_terms(const std::vector<Term>& query);
    void sort_term_posting_lists();
    void advance_term_posting_lists(TermPostingList * tpl, IdType doc_id);
    bool find_pivot_index(size_t * index) const;
    size_t pick_term_index(size_t right) const;
    bool next(size_t * term_index);

    void clean() {
        skipped_doc_ = 0;
        current_doc_id_ = 0;
        current_threshold_ = threshold_;
        term_posting_lists_.clear();
        heap_.clear();
    }

public:
    explicit Wand(
        const InvertedIndex& ii,
        size_t heap_size = 1000,
        ScoreType threshold = 0)
        : ii_(ii), heap_size_(heap_size), threshold_(threshold),
        skipped_doc_(0), current_doc_id_(0),
        current_threshold_(threshold),
        term_posting_lists_(), heap_(),
        verbose_(0) {
    }

    void search(std::vector<Term>& query, std::vector<DocScore> * result);
    // only for comparison
    void search_taat_v1(std::vector<Term>& query, std::vector<DocScore> * result) const;
    void search_taat_v2(std::vector<Term>& query, std::vector<DocScore> * result) const;

    void set_verbose(int verbose) {
        verbose_ = verbose;
    }

    std::ostream& dump(std::ostream& os) const;
private:
    Wand(Wand& other);
    Wand& operator=(Wand& other);
};

std::ostream& operator << (std::ostream& os, const Wand::DocScore& doc);
std::ostream& operator << (std::ostream& os, const Wand::TermPostingList& term);
std::ostream& operator << (std::ostream& os, const Wand& wand);

#endif// WAND_ENGINE_WAND_H
