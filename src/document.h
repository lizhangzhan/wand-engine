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

    std::ostream& dump(std::ostream& os) const;

private:
    Document(Document& other);
    Document& operator=(Document& other);
};
std::ostream& operator << (std::ostream& os, const Document& doc);


struct DocumentBuilder {
    IdType _id;
    std::vector<Term> terms;

    DocumentBuilder() {}
    DocumentBuilder& id(IdType id);
    DocumentBuilder& term(IdType id, ScoreType weight);
    Document * build();

private:
    DocumentBuilder(DocumentBuilder& other);
    DocumentBuilder& operator=(DocumentBuilder& other);
};

#endif// WAND_ENGINE_DOCUMENT_H
