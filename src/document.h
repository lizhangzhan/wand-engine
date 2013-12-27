#ifndef WAND_ENGINE_DOCUMENT_H
#define WAND_ENGINE_DOCUMENT_H

#include <stdint.h>
#include <algorithm>
#include <ostream>
#include <vector>


typedef uint64_t IdType;
typedef uint64_t ScoreType;


struct Term {
    IdType id;
    ScoreType weight;
    Term(IdType _id, ScoreType _weight) :id(_id), weight(_weight) {}
};


struct TermLess {
    bool operator()(const Term& a, const Term& b) const {
        return a.id < b.id;
    }
};


struct Document {
    IdType id;
    std::vector<Term> terms;

private:
    Document() :ref(1) {}
    Document(IdType _id) :id(_id), ref(1) {}
    int ref;

public:
    static Document * create() {
        return new Document();
    }

    static Document * sentinel() {
        static Document doc((IdType)-1);
        return &doc;
    }

    static bool is_sentinel(IdType id) {
        return id == (IdType)-1;
    }

    bool is_sentinel() const {
        return id == (IdType)-1;
    }

    void add_ref() {
        ref++;
    }

    void release_ref() {
        if (is_sentinel()) {
            return;
        }

        if (--ref == 0) {
            delete this;
        }
    }

    std::ostream& dump(std::ostream& os) const {
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

private:
    Document(Document& other);
    Document& operator=(Document& other);
};


struct DocumentBuilder {
    IdType _id;
    std::vector<Term> terms;

    DocumentBuilder() {}

    DocumentBuilder& id(IdType id) {
        _id = id;
        return *this;
    }

    DocumentBuilder& term(IdType id, ScoreType weight) {
        Term term(id, weight);
        terms.push_back(term);
        return *this;
    }

    Document * build() {
        Document * doc = Document::create();
        doc->id = _id;
        std::sort(terms.begin(), terms.end(), TermLess());
        doc->terms.swap(terms);
        terms.clear();
        return doc;
    }

private:
    DocumentBuilder(DocumentBuilder& other);
    DocumentBuilder& operator=(DocumentBuilder& other);
};

#endif// WAND_ENGINE_DOCUMENT_H
