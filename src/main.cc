#include "document.h"
#include "index.h"
#include "wand.h"
#include <iostream>

int main() {
    DocumentBuilder db;
    InvertedIndex ii;
    ii.insert(db.id(1).term(0, 0).term(1, 0).term(3, 0).build());
    ii.insert(db.id(2).term(1, 0).term(2, 0).build());
    ii.insert(db.id(3).term(0, 0).term(2, 0).build());
    ii.insert(db.id(4).term(1, 0).term(3, 0).build());
    ii.insert(db.id(5).term(3, 0).term(4, 0).build());
    ii.insert(db.id(6).term(2, 0).build());

    ii.insert(db.id(26).term(0, 0).build());
    ii.insert(db.id(10).term(1, 1).build());
    ii.insert(db.id(100).term(1, 1).build());
    ii.insert(db.id(34).term(2, 2).build());
    ii.insert(db.id(56).term(2, 2).build());
    ii.insert(db.id(23).term(3, 3).build());
    ii.insert(db.id(70).term(3, 3).build());
    ii.insert(db.id(200).term(3, 3).build());
    ii.insert(db.id(14).term(4, 4).build());
    ii.insert(db.id(78).term(4, 4).build());

    // std::cout << ii << std::endl;

    Wand wand(ii, 50, 4);
    std::vector<Term> terms;
    terms.push_back(Term(0, 1));
    terms.push_back(Term(1, 1));
    terms.push_back(Term(2, 1));
    terms.push_back(Term(3, 1));
    terms.push_back(Term(4, 1));
    std::vector<Wand::DocScore> result;
    wand.search(terms, &result);

    return 0;
}
