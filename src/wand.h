#ifndef WAND_ENGINE_WAND_H
#define WAND_ENGINE_WAND_H

#include "inverted.h"
#include <algorithm>
#include <ostream>
#include <set>
#include <iostream>


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
    static ScoreType dot_product(const std::vector<Term>& query, const std::vector<Term>& doc) {
        ScoreType dp = 0;
        size_t i = 0, j = 0, imax = query.size(), jmax = doc.size();
        for (; i < imax && j < jmax; ) {
            if (query[i].id < doc[j].id) {
                i++;
            } else if (query[i].id > doc[j].id) {
                j++;
            } else {
                dp += (query[i].weight * doc[j].weight);
                i++;
                j++;
            }
        }
        return dp;
    }

    static ScoreType dot_product(const std::vector<Term>& query, const Document * doc) {
        return dot_product(query, doc->terms);
    }

    void match_terms(const std::vector<Term>& query) {
        size_t s = query.size();
        for (size_t i = 0; i < s; i++) {
            IdType term_id = query[i].id;
            const PostingList * posting_list = ii_.find(term_id);
            if (posting_list) {
                PostingListNode * first = posting_list->front();
                if (first) {
                    TermPostingList tpl;
                    tpl.term_id = term_id;
                    tpl.posting_list = posting_list;
                    tpl.current = first;
                    tpl.remains = posting_list->size();
                    tpl.term_weight_in_query = term_id;
                    term_posting_lists_.push_back(tpl);
                }
            }
        }
    }

    void sort_term_posting_lists() {
        std::sort(term_posting_lists_.begin(), term_posting_lists_.end(),
            TermPostingListFirstId());
    }

    void advance_term_posting_lists(TermPostingList * tpl, IdType doc_id) {
        PostingListNode *& current = tpl->current;
        while (current->doc->id < doc_id) {
            current = current->next;
            skipped_doc_++;
            tpl->remains--;
        }
        // current->doc->id >= doc_id
    }

    bool find_pivot_term_index(size_t * index) const {
        ScoreType acc_upper_bound = 0;
        size_t s = term_posting_lists_.size();
        for (size_t i = 0; i < s; i++) {
            const TermPostingList * tpl = &term_posting_lists_[i];
            acc_upper_bound += tpl->posting_list->get_upper_bound() * tpl->term_weight_in_query;
            if (acc_upper_bound >= heap_min_score_) {
                *index = i;
                return true;
            }
        }
        return false;
    }

    size_t pick_term_index(size_t left, size_t right) const {
        //// The simplest way: always return the first one.
        //return left;

        // We can have many strategies to pick a term.
        // One rule is: picking this term will skip more doc:
        // TermPostingList with largest 'remains'
        size_t max_remains = 0;
        size_t index = left;
        for (size_t i = left; i < right; i++) {
            const TermPostingList * tpl = &term_posting_lists_[i];
            if (tpl->remains > max_remains) {
                max_remains = tpl->remains;
                index = i;
            }
        }
        return index;
    }

    bool next(IdType * next_doc_id, size_t * term_index) {
        for (;;) {
            sort_term_posting_lists();
            size_t pivot_index;
            if (!find_pivot_term_index(&pivot_index)) {
                // no more doc
                return false;
            }

            TermPostingList * pivot = &term_posting_lists_[pivot_index];
            IdType pivot_doc_id = pivot->current->doc->id;

            if (Document::is_sentinel(pivot_doc_id)) {
                return false;
            }

            if (pivot_doc_id <= current_doc_id_) {
                // pivot has already been considered,
                // advance one of the preceding terms
                size_t term_index = pick_term_index(0, pivot_index);
                advance_term_posting_lists(&term_posting_lists_[term_index], current_doc_id_ + 1);
            } else {
                if (pivot_doc_id == term_posting_lists_[0].current->doc->id) {
                    current_doc_id_ = pivot_doc_id;
                    *next_doc_id = pivot_doc_id;
                    *term_index = pivot_index;
                    return true;
                } else {
                    size_t term_index = pick_term_index(0, pivot_index);
                    advance_term_posting_lists(&term_posting_lists_[term_index], pivot_doc_id);
                }
            }
        }
    }

public:
    explicit Wand(
        const InvertedIndex& ii,
        size_t heap_size = 1000)
        :ii_(ii), heap_size_(heap_size),
        skipped_doc_(0), current_doc_id_(0), heap_min_score_(0) {
    }

    void search(std::vector<Term>& query, std::vector<DocScore> * result) {
        std::sort(query.begin(), query.end(), TermLess());
        match_terms(query);
        if (term_posting_lists_.empty()) {
            return;
        }

        IdType next_doc_id;
        size_t term_index;
        bool found = true;

        for (;;) {
            found = next(&next_doc_id, &term_index);
            if (found) {
                const TermPostingList * tpl = &term_posting_lists_[term_index];
                const Document * doc = tpl->current->doc;
                std::cout << "found doc id: " << next_doc_id << std::endl;
                std::cout << "matched term id: " << tpl->term_id << std::endl;

                DocScore ds;
                ds.doc = doc;
                ds.score = dot_product(query, doc);

                if (heap_.size() < heap_size_) {
                    heap_.insert(ds);
                } else {
                    // heap_.size() == heap_size_
                    // update heap_ and heap_min_score_
                    std::set<DocScore, DocScoreLess>::iterator it = heap_.begin();
                    if (ds.score > (*it).score) {
                        heap_.erase(it);
                        heap_.insert(ds);
                        heap_min_score_ = (*heap_.begin()).score;
                    }
                }
            } else {
                break;
            }
        }

        std::cout << "skipped doc: " << skipped_doc_ << std::endl;
    }

private:
    Wand(Wand& other);
    Wand& operator=(Wand& other);
};

#endif// WAND_ENGINE_WAND_H
