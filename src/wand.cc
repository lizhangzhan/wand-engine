#include "wand.h"
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>

ScoreType Wand::dot_product(const TermVector& query, const TermVector& doc) {
    // 'query' and 'doc' must be sorted by term id in advance.
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

ScoreType Wand::full_evaluate(const TermVector& query, const Document * doc) {
    return dot_product(query, doc->terms);
}

void Wand::match_terms(const TermVector& query) {
    size_t s = query.size();
    for (size_t i = 0; i < s; i++) {
        const Term& term = query[i];
        IdType term_id = term.id;
        ScoreType term_weight = term.weight;
        const PostingList * posting_list = ii_.find(term_id);
        if (posting_list) {
            PostingListNode * first = posting_list->front();
            if (first) {
                TermPostingList tpl;
                tpl.term_id = term_id;
                tpl.posting_list = posting_list;
                tpl.current = first;
                tpl.remains = posting_list->size();
                tpl.weight_in_query = term_weight;
                term_posting_lists_.push_back(tpl);
            }
        }
    }
}

void Wand::sort_term_posting_lists() {
    std::sort(term_posting_lists_.begin(), term_posting_lists_.end(),
        TermPostingListFirstId());
}

void Wand::advance_term_posting_lists(TermPostingList * tpl, IdType doc_id) {
    // Find a doc after 'tpl->current', whose id >= 'doc_id',
    // and set tpl->current to this doc.
    PostingListNode *& current = tpl->current;

    while (current->doc->id < doc_id) {
        current = current->next;
        skipped_doc_++;
        tpl->remains--;
    }
    assert(current->doc->id >= doc_id);
}

bool Wand::find_pivot_index(size_t * pivot_index) const {
    ScoreType acc_score = 0;
    size_t s = term_posting_lists_.size();
    for (size_t i = 0; i < s; i++) {
        const TermPostingList * tpl = &term_posting_lists_[i];
        acc_score += tpl->posting_list->get_upper_bound() * tpl->weight_in_query;
        // Another policy is to disregard term weight in query:
        // acc_score += tpl->posting_list->get_upper_bound();
        // This policy is not as accurate.
        if (acc_score >= current_threshold_) {
            *pivot_index = i;
            return true;
        }
    }
    return false;
}

size_t Wand::pick_term_index(size_t right) const {
    // The simplest way: always return the first one.
    return 0;

    // We can have many strategies to pick a term.
    // One principle is: picked term will skip more doc.
    // That is the TermPostingList with the largest 'remains':
    // size_t max_remains = 0;
    // size_t index = 0;
    // for (size_t i = 0; i < right; i++) {
    //     const TermPostingList * tpl = &term_posting_lists_[i];
    //     if (tpl->remains > max_remains) {
    //         max_remains = tpl->remains;
    //         index = i;
    //     }
    // }
    // return index;
}

bool Wand::next(size_t * term_index) {
    for (;;) {
        sort_term_posting_lists();

        size_t pivot_index;
        if (!find_pivot_index(&pivot_index)) {
            // no more doc
            return false;
        }

        TermPostingList * pivot = &term_posting_lists_[pivot_index];
        IdType pivot_doc_id = pivot->current->doc->id;
        if (Document::is_sentinel(pivot_doc_id)) {
            // no more doc
            return false;
        }

        if (pivot_doc_id <= current_doc_id_) {
            // pivot has already been considered, advance one of the preceding terms.
            size_t term_index = pick_term_index(pivot_index);
            TermPostingList * tpl = &term_posting_lists_[term_index];
            assert(tpl->current->doc->id < current_doc_id_ + 1);
            // this kind of advance is not considered as a skip,
            // because at least one advance shall come
            skipped_doc_--;
            advance_term_posting_lists(tpl, current_doc_id_ + 1);
        } else {
            if (pivot_doc_id == term_posting_lists_[0].current->doc->id) {
                // two valid outputs of this function
                current_doc_id_ = pivot_doc_id;
                *term_index = pivot_index;
                return true;
            } else {
                // advance all term posting lists
                for (size_t i = 0, s = term_posting_lists_.size(); i < s; i++) {
                    advance_term_posting_lists(&term_posting_lists_[i], pivot_doc_id);
                }
                // In the original paper, author only advances one term posting list like this:
                // size_t term_index = pick_term_index(pivot_index);
                // advance_term_posting_lists(&term_posting_lists_[term_index], pivot_doc_id);
            }
        }
    }
}

void Wand::search(TermVector& query, std::vector<DocIdScore> * result) {
    std::sort(query.begin(), query.end(), TermLess());
    match_terms(query);
    if (term_posting_lists_.empty()) {
        // no doc matched
        return;
    }

    size_t pivot_index;
    bool found;

    if (verbose_) {
        std::cout << *this << "\n";
    }

    for (;;) {
        found = next(&pivot_index);
        if (!found) {
            break;
        }

        const TermPostingList * tpl = &term_posting_lists_[pivot_index];
        const Document * doc = tpl->current->doc;

        DocIdScore ds;
        ds.doc_id = doc->id;
        ds.score = full_evaluate(query, doc);

        if (heap_.size() < heap_size_) {
            if (ds.score > current_threshold_) {
                heap_.insert(ds);
            }
        } else {
            // Heap is full, update 'heap_' and 'current_threshold_' if its score > min score in heap.
            HeapType::iterator it = heap_.begin();
            if (ds.score > (*it).score) {
                heap_.erase(it);
                heap_.insert(ds);
                current_threshold_ = (*heap_.begin()).score;
            }
        }

        if (verbose_) {
            std::cout << *this << "\n";
            std::cout << "matched term id(pivot): " << tpl->term_id
                << ", doc id: " << current_doc_id_ << "\n";
            std::cout << "\n";
        }
    }

    result->assign(heap_.begin(), heap_.end());
    clean();
}

void Wand::search_taat_v1(TermVector& query, std::vector<DocIdScore> * result) const {
    typedef std::map<IdType, ScoreType> DocIdScoreMapType;
    DocIdScoreMapType doc_map;

    size_t s = query.size();
    for (size_t i = 0; i < s; i++) {
        const Term& term = query[i];
        IdType term_id = term.id;
        ScoreType term_weight = term.weight;
        const PostingList * posting_list = ii_.find(term_id);
        if (posting_list) {
            PostingListNode * first = posting_list->front();
            while (first) {
                Document * doc = first->doc;
                if (doc->is_sentinel()) {
                    break;
                }
                IdType doc_id = doc->id;

                DocIdScoreMapType::iterator it = doc_map.find(doc_id);
                if (it == doc_map.end()) {
                    ScoreType score = doc->get_weight(term_id) * term_weight;
                    doc_map.insert(std::make_pair(doc_id, score));
                } else {
                    ScoreType& score = (*it).second;
                    score += doc->get_weight(term_id) * term_weight;
                }

                first = first->next;
            }
        }
    }

    result->clear();
    result->reserve(doc_map.size());
    DocIdScoreMapType::const_iterator first = doc_map.begin();
    DocIdScoreMapType::const_iterator last = doc_map.end();
    for (; first != last; ++first) {
        result->push_back(DocIdScore((*first).first, (*first).second));
    }
}

void Wand::search_taat_v2(TermVector& query, std::vector<DocIdScore> * result) const {
    typedef std::map<IdType, ScoreType> DocIdScoreMapType;
    DocIdScoreMapType doc_map;

    size_t s = query.size();
    for (size_t i = 0; i < s; i++) {
        const Term& term = query[i];
        IdType term_id = term.id;
        const PostingList * posting_list = ii_.find(term_id);
        if (posting_list) {
            PostingListNode * first = posting_list->front();
            while (first) {
                Document * doc = first->doc;
                if (doc->is_sentinel()) {
                    break;
                }
                IdType doc_id = doc->id;

                DocIdScoreMapType::iterator it = doc_map.find(doc_id);
                if (it == doc_map.end()) {
                    ScoreType score = full_evaluate(query, doc);
                    doc_map.insert(std::make_pair(doc_id, score));
                }

                first = first->next;
            }
        }
    }

    result->clear();
    result->reserve(doc_map.size());
    DocIdScoreMapType::const_iterator first = doc_map.begin();
    DocIdScoreMapType::const_iterator last = doc_map.end();
    for (; first != last; ++first) {
        result->push_back(DocIdScore((*first).first, (*first).second));
    }
}

std::ostream& Wand::DocIdScore::dump(std::ostream& os) const {
    os << "  doc id: " << doc_id << ", score: " << score << "\n";
    return os;
}

std::ostream& Wand::TermPostingList::dump(std::ostream& os) const {
    os << "  term_id: " << term_id << "\n";
    if (current->doc->is_sentinel()) {
        os << "    all docs processed" << "\n";
    } else {
        os << "    current doc id: " << current->doc->id << "\n";
        os << "    remains: " << remains << "\n";
    }
    os << "    weight in query: " << weight_in_query << "\n";
    return os;
}

std::ostream& Wand::dump(std::ostream& os) const {
    os << "heap size: " << heap_size_ << "\n";
    os << "threshold: " << threshold_ << "\n";
    os << "skipped doc: " << skipped_doc_ << "\n";
    os << "current doc id: " << current_doc_id_ << "\n";
    os << "current threshold: " << current_threshold_ << "\n";

    os << "posting list:" << "\n";
    for (size_t i = 0, s = term_posting_lists_.size(); i < s; i++) {
        os << term_posting_lists_[i];
    }

    os << "heap:" << "\n";
    HeapType::const_iterator first = heap_.begin();
    HeapType::const_iterator last = heap_.end();
    for (; first != last; ++ first) {
        os << *first;
    }

    return os;
}

std::ostream& operator << (std::ostream& os, const Wand::DocIdScore& doc) {
    doc.dump(os);
    return os;
}

std::ostream& operator << (std::ostream& os, const Wand::TermPostingList& term) {
    term.dump(os);
    return os;
}

std::ostream& operator << (std::ostream& os, const Wand& wand) {
    wand.dump(os);
    return os;
}
