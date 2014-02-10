#include "wand.h"
#include <iostream>

ScoreType Wand::dot_product(const std::vector<Term>& query, const std::vector<Term>& doc) {
    // 'query' and 'doc' are sorted by term id
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

ScoreType Wand::dot_product(const std::vector<Term>& query, const Document * doc) {
    return dot_product(query, doc->terms);
}

void Wand::match_terms(const std::vector<Term>& query) {
    size_t s = query.size();
    for (size_t i = 0; i < s; i++) {
        const Term& term = query[i];
        IdType term_id = term.id;
        ScoreType weight = term.weight;
        const PostingList * posting_list = ii_.find(term_id);
        if (posting_list) {
            PostingListNode * first = posting_list->front();
            if (first) {
                TermPostingList tpl;
                tpl.term_id = term_id;
                tpl.posting_list = posting_list;
                tpl.current = first;
                tpl.remains = posting_list->size();
                tpl.weight_in_query = weight;
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
    PostingListNode *& current = tpl->current;
    while (current->doc->id < doc_id) {
        current = current->next;
        skipped_doc_++;
        tpl->remains--;
    }
    skipped_doc_--;
    // current->doc->id >= doc_id
}

bool Wand::find_pivot_term_index(size_t * index) const {
    ScoreType acc_upper_bound = 0;
    size_t s = term_posting_lists_.size();
    for (size_t i = 0; i < s; i++) {
        const TermPostingList * tpl = &term_posting_lists_[i];
        acc_upper_bound += tpl->posting_list->get_upper_bound() * tpl->weight_in_query;
        if (acc_upper_bound >= heap_min_score_ && acc_upper_bound >= threshold_) {
            *index = i;
            return true;
        }
    }
    return false;
}

size_t Wand::pick_term_index(size_t left, size_t right) const {
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

bool Wand::next(IdType * next_doc_id, size_t * term_index) {
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

void Wand::search(std::vector<Term>& query, std::vector<DocScore> * result) {
    std::sort(query.begin(), query.end(), TermLess());
    match_terms(query);
    if (term_posting_lists_.empty()) {
        return;
    }

    IdType next_doc_id;
    size_t term_index;
    bool found = true;

    std::cout << *this << std::endl;

    for (;;) {
        found = next(&next_doc_id, &term_index);

        if (found) {
            const TermPostingList * tpl = &term_posting_lists_[term_index];
            const Document * doc = tpl->current->doc;

            DocScore ds;
            ds.doc = doc;
            ds.score = dot_product(query, doc);

            if (heap_.size() < heap_size_) {
                heap_.insert(ds);
                std::cout << heap_.size() << std::endl;
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

            std::cout << *this << std::endl;
            std::cout << "matched term id(pivot): " << tpl->term_id
                << ", doc id: " << next_doc_id << std::endl;
        } else {
            break;
        }
        std::cout << std::endl;
    }

    result->assign(heap_.begin(), heap_.end());
//    std::cout << "result size: " << result->size() << std::endl;
//    std::cout << "skipped doc: " << skipped_doc_ << std::endl;
}

std::ostream& Wand::DocScore::dump(std::ostream& os) const {
    os << "  doc id: " << doc->id << ", doc score: " << score << std::endl;
    return os;
}

std::ostream& Wand::TermPostingList::dump(std::ostream& os) const {
    os << "  term_id: " << term_id << std::endl;
    if (current->doc->is_sentinel()) {
        os << "    all docs processed" << std::endl;
    } else {
        os << "    current doc id: " << current->doc->id << std::endl;
        os << "    remains: " << remains << std::endl;
    }
    os << "    weight in query: " << weight_in_query << std::endl;
    return os;
}

std::ostream& Wand::dump(std::ostream& os) const {
    os << "heap size: " << heap_size_ << std::endl;
    os << "threshold: " << threshold_ << std::endl;
    os << "skipped doc: " << skipped_doc_ << std::endl;
    os << "current doc id: " << current_doc_id_ << std::endl;
    os << "heap min score: " << heap_min_score_ << std::endl;

    os << "posting list:" << std::endl;
    for (size_t i = 0; i < term_posting_lists_.size(); i++) {
        os << term_posting_lists_[i];
    }

    os << "heap:" << std::endl;
    std::set<DocScore, DocScoreLess>::const_iterator first = heap_.begin();
    std::set<DocScore, DocScoreLess>::const_iterator last = heap_.end();
    for (; first != last; ++ first) {
        os << *first;
    }

    return os;
}

std::ostream& operator << (std::ostream& os, const Wand::DocScore& doc) {
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
