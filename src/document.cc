#include "document.h"

std::ostream& Document::dump(std::ostream& os) const {
    if (!is_sentinel()) {
        os << "    doc id: " << id << std::endl;
        for (size_t i = 0; i < terms.size(); i++) {
            const Term& term = terms[i];
            os << "      term id: " << term.id << ", weight: " << term.weight << std::endl;
        }
    } else {
        os << "    (sentinel)" << std::endl;
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
    Document * doc = Document::create();
    doc->id = _id;
    std::sort(terms.begin(), terms.end(), TermLess());
    doc->terms.swap(terms);
    terms.clear();
    return doc;
}
