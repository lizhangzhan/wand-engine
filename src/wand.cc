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
    for (size_t i = 0, s = query.size(); i < s; i++) {
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
                term_posting_list_set_.insert(tpl);
            }
        }
    }
}

void Wand::advance_term_posting_list(const TermPostingListSetType::const_iterator& to_advance,
        IdType doc_id) {
    TermPostingList tpl = (*to_advance);// TODO copy
    term_posting_list_set_.erase(to_advance);

    // Find a doc after 'tpl.current', whose id >= 'doc_id',
    // and set tpl.current to this doc.
    PostingListNode *& current = tpl.current;

    while (current->doc->id < doc_id) {
        current = current->next;
        skipped_doc_++;
        tpl.remains--;
    }
    assert(current->doc->id >= doc_id);

    term_posting_list_set_.insert(tpl);
}

bool Wand::find_pivot(TermPostingListSetType::const_iterator * pivot) const {
    ScoreType acc_score = 0;
    TermPostingListSetType::const_iterator first = term_posting_list_set_.begin();
    TermPostingListSetType::const_iterator last = term_posting_list_set_.end();
    for (; first != last; ++first) {
        const TermPostingList& tpl = (*first);
        acc_score += tpl.posting_list->get_upper_bound() * tpl.weight_in_query;
        // Another policy is to disregard term weight in query:
        // acc_score += tpl.posting_list->get_upper_bound();
        // This policy is not as accurate.
        if (acc_score >= current_threshold_) {
            *pivot = first;
            return true;
        }
    }
    return false;
}

Wand::TermPostingListSetType::const_iterator
Wand::pick_term(const TermPostingListSetType::const_iterator& pivot) const {
    // The simplest way: always return the first one(current term).
    return term_posting_list_set_.begin();

    // We can have many strategies to pick a term.
    // One principle is: picked term will skip more doc.
    // That is the TermPostingList with the largest 'remains':
}

bool Wand::next(TermPostingListSetType::const_iterator * next_term) {
    for (;;) {
        TermPostingListSetType::const_iterator pivot;
        if (!find_pivot(&pivot)) {
            // no more doc
            return false;
        }

        IdType pivot_doc_id = (*pivot).current->doc->id;
        if (Document::is_sentinel(pivot_doc_id)) {
            // no more doc
            return false;
        }

        if (pivot_doc_id <= current_doc_id_) {
            // pivot has already been considered, advance one of the preceding terms.
            // this kind of advance is not considered as a skip,
            // because at least one advance shall come.
            skipped_doc_--;
            TermPostingListSetType::const_iterator picked = pick_term(pivot);
            assert((*picked).current->doc->id < current_doc_id_ + 1);
            advance_term_posting_list(picked, current_doc_id_ + 1);
        } else {
            if (pivot_doc_id == term_posting_list_set_.begin()->current->doc->id) {
                // two valid outputs of this function
                current_doc_id_ = pivot_doc_id;
                *next_term = pivot;
                return true;
            } else {
                //// not enough mass yet on pivot, advance all of the preceding terms
                // for (;;)
                // {
                //     TermPostingListSetType::const_iterator first = term_posting_list_set_.begin();
                //     if (first == pivot)
                //         break;
                //     advance_term_posting_list(first, pivot_doc_id);
                // }

                // In the original paper, author only advances one term posting list like this:
                // not enough mass yet on pivot, advance one of the preceding terms
                TermPostingListSetType::const_iterator picked = pick_term(pivot);
                advance_term_posting_list(picked, pivot_doc_id);
            }
        }
    }
}

void Wand::search(TermVector& query, std::vector<DocIdScore> * result) {
    std::sort(query.begin(), query.end(), TermLess());
    match_terms(query);
    if (term_posting_list_set_.empty()) {
        // no doc matched
        return;
    }

    bool found;

    if (verbose_) {
        std::cout << *this << "\n";
    }

    for (;;) {
        TermPostingListSetType::const_iterator pivot;
        found = next(&pivot);
        if (!found) {
            break;
        }

        const TermPostingList& tpl = (*pivot);
        const Document * doc = tpl.current->doc;

        DocIdScore ds;
        ds.doc_id = doc->id;
        ds.score = full_evaluate(query, doc);

        if (doc_heap_.size() < heap_size_) {
            if (ds.score > current_threshold_) {
                doc_heap_.insert(ds);
            }
        } else {
            // Heap is full,
            // update 'doc_heap_' and 'current_threshold_' if its score > min score in heap.
            DocHeapType::iterator it = doc_heap_.begin();
            if (ds.score > (*it).score) {
                doc_heap_.erase(it);
                doc_heap_.insert(ds);
                current_threshold_ = (*doc_heap_.begin()).score;
            }
        }

        if (verbose_) {
            std::cout << *this << "\n";
            std::cout << "matched term id(pivot): " << tpl.term_id
                << ", doc id: " << current_doc_id_ << "\n";
            std::cout << "\n";
        }
    }

    result->assign(doc_heap_.rbegin(), doc_heap_.rend());
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
    for (; first != last; ++first)
        result->push_back(DocIdScore((*first).first, (*first).second));
    std::sort(result->begin(), result->end(), DocIdScore_ScoreGreat());
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
    for (; first != last; ++first)
        result->push_back(DocIdScore((*first).first, (*first).second));
    std::sort(result->begin(), result->end(), DocIdScore_ScoreGreat());
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
    {
        TermPostingListSetType::const_iterator first = term_posting_list_set_.begin();
        TermPostingListSetType::const_iterator last = term_posting_list_set_.end();
        for (; first != last; ++first) {
            os << (*first);
        }
    }

    os << "doc heap:" << "\n";
    {
        DocHeapType::const_iterator first = doc_heap_.begin();
        DocHeapType::const_iterator last = doc_heap_.end();
        for (; first != last; ++ first) {
            os << *first;
        }
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
