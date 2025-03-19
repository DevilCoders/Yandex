#pragma once

#include <kernel/porno/proto/porno_filter_level.pb.h>

enum EPornoQueryType {
    PQT_PORN,         // regular porn query
    PQT_GREY,         // "gray" query
    PQT_CHILD,        // child
    PQT_NOPORN,       // not a porn query
    PQT_CHILD_PORN,   // child porn
};
