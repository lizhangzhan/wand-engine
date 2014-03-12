#include "wand.h"
#include "city.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>

static void timeval_diff(const struct timeval& begin, const struct timeval& end) {
    struct timeval diff;
    if ((end.tv_usec - begin.tv_usec) < 0) {
        diff.tv_sec = end.tv_sec - begin.tv_sec - 1;
        diff.tv_usec = 1000000 + end.tv_usec - begin.tv_usec;
    } else {
        diff.tv_sec = end.tv_sec - begin.tv_sec;
        diff.tv_usec = end.tv_usec - begin.tv_usec;
    }
    std::cout << "cost " << diff.tv_sec << "." << diff.tv_usec / 1000 << " seconds\n";
}

static void load_cap_features(InvertedIndex * ii, FILE * fp) {
    char line[4096];
    char feature[128];
    int score;
    DocumentBuilder db;
    IdType id = 0;
    struct timeval begin, end;

    std::cout << "loading index\n";
    gettimeofday(&begin, 0);
    while((fgets(line, sizeof(line), fp))) {
        if (strcmp("cap_features\n", line) == 0) {
            if (id != 0) {
                ii->insert(db.build());
            }
            db.id(id);
            id++;
            if (id % 10000 == 0) {
                std::cout << "loaded " << id << " documents\n";
            }
        } else {
            if (fscanf(fp, "    %s %d\n", feature, &score) == 2) {
                uint64 hash = CityHash64(feature, strlen(feature));
                db.term(hash, (ScoreType)score);
            }
        }
    }
    gettimeofday(&end, 0);
    std::cout << "loaded " << id << " documents, ";
    timeval_diff(begin, end);
}

static int load_cap_features(InvertedIndex * ii, const char * filename) {
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        std::cout << "can't open " << filename << "\n";
        return -1;
    }
    load_cap_features(ii, fp);
    fclose(fp);
    return 0;
}

static void cap_features_test() {
    InvertedIndex ii;
    if (load_cap_features(&ii, "src/cap-features/offnet-cap") == -1) {
        return;
    }

    const char * query_terms[] = {
        "w-Scottish_National_Party",
        "w-Cinema_of_India",
        "y-yct:001000670",
        "y-yct:001000001",
        "w-John_Goodman",
        "w-2014_Winter_Olympics",
        "w-Manchester_United_F.C.",
    };

    DocumentBuilder db;
    for (size_t i = 0; i < sizeof(query_terms)/sizeof(query_terms[0]); i++) {
        db.term(CityHash64(query_terms[i], strlen(query_terms[i])), 100);
    }
    Document * query = db.build();
    std::vector<Wand::DocIdScore> result;
    Wand wand(ii, 200, 10000);
    wand.set_verbose(0);

    int times = 100;
    struct timeval begin, end;

    std::cout << "Wand::search query " << times << " times, ";
    gettimeofday(&begin, 0);
    for (int i = 0; i < times; i++) {
        wand.search(query->terms, &result);
    }
    gettimeofday(&end, 0);
    timeval_diff(begin, end);
    // std::cout << "search final result:" << "\n";
    // for (size_t i = 0; i < result.size(); i++) {
    //     std::cout << result[i];
    // }

    std::cout << "Wand::search_taat_v1 query " << times << " times, ";
    gettimeofday(&begin, 0);
    for (int i = 0; i < times; i++) {
        wand.search_taat_v1(query->terms, &result);
    }
    gettimeofday(&end, 0);
    timeval_diff(begin, end);
    // std::cout << "search_taat_v1 final result:" << "\n";
    // for (size_t i = 0; i < result.size(); i++) {
    //     std::cout << result[i];
    // }

    query->release_ref();
}

static void simple_test() {
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

    Wand wand(ii, 500, 1);
    wand.set_verbose(0);

    Document * query = db
        .term(0, 1)
        .term(1, 1)
        .term(2, 1)
        .term(3, 1)
        .term(4, 1)
        .build();
    std::vector<Wand::DocIdScore> result;

    wand.search(query->terms, &result);
    std::cout << "search final result:" << "\n";
    for (size_t i = 0; i < result.size(); i++) {
        std::cout << result[i];
    }

    wand.search_taat_v1(query->terms, &result);
    std::cout << "search_taat_v1 final result:" << "\n";
    for (size_t i = 0; i < result.size(); i++) {
        std::cout << result[i];
    }

    wand.search_taat_v2(query->terms, &result);
    std::cout << "search_taat_v2 final result:" << "\n";
    for (size_t i = 0; i < result.size(); i++) {
        std::cout << result[i];
    }

    query->release_ref();
}

int main() {
    simple_test();
    // cap_features_test();
    return 0;
}
