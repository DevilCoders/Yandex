#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

struct TXmlSearchRequest {
    struct TGroupBy {
        TString Attr;
        TString Mode;
        TString GroupsOnPage;
        TString DocsInGroup;
        TString CurCateg;
        TString Depth;
        TString KillDup;
    };
    typedef TVector<TGroupBy> TGroupByList;

    TString Query;
    TString SortBy;
    TString Page;
    TString MaxPassages;
    TString MaxPassageLength;
    TString MaxTitleLength;
    TString MaxHeadlineLength;
    TString MaxTextLength;
    TString ReqId;
    bool NoCache;

    TGroupByList Groupings;

    TXmlSearchRequest();
    TString ToCgiString() const;
};

bool ParseXmlSearch(IInputStream& input, TXmlSearchRequest& result);
