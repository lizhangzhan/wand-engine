#include "wand.h"
#include <iostream>
#include <string>

int main() {
    typedef InvertedIndex<std::string, int, std::hash<std::string>, std::equal_to<std::string>,
        std::identity<int>, std::identity<int> > InvertedIndexSI;
    typedef WandOperator<InvertedIndexSI> WandSI;

    InvertedIndexSI ii;
    ii.insert("metallica", new int(1));
    ii.insert("metallica", new int(2));
    ii.insert("metallica", new int(3));
    ii.insert("metallica", new int(4));

    ii.insert("slayer", new int(2));
    ii.insert("slayer", new int(3));
    ii.insert("slayer", new int(5));
    ii.insert("slayer", new int(6));

    ii.insert("megadeth", new int(1));
    ii.insert("megadeth", new int(2));
    ii.insert("megadeth", new int(3));
    ii.insert("megadeth", new int(6));

    //ii.dump(std::cout);

    //const InvertedIndexSI::PostingListType * posting_list = ii.find("slayer");
    //if (posting_list) {
    //    posting_list->dump(std::cout);
    //}


    WandSI wand(ii);
    std::vector<std::string> terms;
    terms.push_back("megadeth");
    terms.push_back("slayer");
    terms.push_back("metallica");
    std::vector<WandSI::DocScore> result;
    wand.search(terms, &result);

    return 0;
}
