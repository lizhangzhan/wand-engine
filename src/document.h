#ifndef WAND_ENGINE_DOCUMENT_H
#define WAND_ENGINE_DOCUMENT_H

#include <stdint.h>
#include <ostream>
#include <vector>

typedef uint64_t IdType;
typedef uint64_t ScoreType;

struct Term {
    IdType id;
    ScoreType weight;
    Term(IdType _id, ScoreType _weight) : id(_id), weight(_weight) {}
};

typedef std::vector<Term> TermVector;

struct TermLess {
    bool operator()(const Term& a, const Term& b) const {
        return a.id < b.id;
    }
    bool operator()(IdType id, const Term& term) const {
        return id < term.id;
    }
    bool operator()(const Term& term, IdType id) const {
        return term.id < id;
    }
};

struct TermIdEqualer {
    bool operator()(const Term& a, const Term& b) const {
        return a.id == b.id;
    }
};

struct Document {
    IdType id;
    TermVector terms;// "terms" must be sorted

private:
    Document() :ref(1) {}
    Document(IdType _id) : id(_id), ref(1) {}
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

    ScoreType get_weight(IdType term_id) const;
    std::ostream& dump(std::ostream& os) const;

private:
    Document(Document& other);
    Document& operator=(Document& other);
};

std::ostream& operator << (std::ostream& os, const Document& doc);

struct DocumentBuilder {
    IdType _id;
    TermVector terms;

    DocumentBuilder() : _id(0) {}
    DocumentBuilder& id(IdType id);
    DocumentBuilder& term(IdType id, ScoreType weight);
    Document * build();

private:
    DocumentBuilder(DocumentBuilder& other);
    DocumentBuilder& operator=(DocumentBuilder& other);
};

#endif// WAND_ENGINE_DOCUMENT_H
