#include "document.h"
#include <algorithm>

ScoreType Document::get_weight(IdType term_id) const {
    TermVector::const_iterator it =
        std::lower_bound(terms.begin(), terms.end(), term_id, TermLess());
    if (it == terms.end()) {
        return 0;
    }
    return (*it).weight;
}

std::ostream& Document::dump(std::ostream& os) const {
    if (!is_sentinel()) {
        os << "    doc id: " << id << "\n";
        for (size_t i = 0, s = terms.size(); i < s; i++) {
            const Term& term = terms[i];
            os << "      term id: " << term.id << ", weight: " << term.weight << "\n";
        }
    } else {
        os << "    (sentinel)\n";
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const Document& doc) {
    doc.dump(os);
    return os;
}

DocumentBuilder& DocumentBuilder::id(IdType id) {
    _id = id;
    return *this;
}

DocumentBuilder& DocumentBuilder::term(IdType id, ScoreType weight) {
    Term term(id, weight);
    terms.push_back(term);
    return *this;
}

Document * DocumentBuilder::build() {
    // sort and dedup terms
    std::sort(terms.begin(), terms.end(), TermLess());
    terms.erase(std::unique(terms.begin(), terms.end(), TermIdEqualer()), terms.end());

    Document * doc = Document::create();
    doc->id = _id;
    doc->terms.swap(terms);

    _id = 0;
    terms.clear();
    return doc;
}
