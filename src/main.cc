#include "document.h"
#include "posting.h"
#include "inverted.h"
#include "wand.h"

int main() {
    DocumentBuilder db;
    InvertedIndex ii;
    ii.insert(db.id(10001).term(1, 6456).term(2, 4567).term(3, 987).build());
    ii.insert(db.id(10302).term(1, 6456).term(2, 4567).term(3, 2987).build());
    ii.insert(db.id(10022).term(10, 9856).build());
    ii.insert(db.id(10009).term(9, 6783).build());
    ii.insert(db.id(10030).term(2, 9856).term(9, 9856).build());
    ii.insert(db.id(10031).term(3, 9856).term(10, 9856).build());
    ii.insert(db.id(10032).term(4, 9856).term(11, 9856).build());
    ii.insert(db.id(10033).term(5, 9856).term(12, 9856).build());

    Wand wand(ii, 500);
    std::vector<Term> terms;
    terms.push_back(Term(3, 99));
    terms.push_back(Term(1, 199));
    terms.push_back(Term(4, 399));
    terms.push_back(Term(5, 599));
    terms.push_back(Term(6, 99));
    terms.push_back(Term(7, 199));
    terms.push_back(Term(8, 299));
    terms.push_back(Term(9, 99));
    terms.push_back(Term(10, 199));
    std::vector<Wand::DocScore> result;
    wand.search(terms, &result);

    return 0;
}
