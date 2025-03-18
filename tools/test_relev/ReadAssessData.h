#pragma once

#include "req_result.h"

#include <kernel/groupattrs/categseries.h>
#include <util/digest/murmur.h>

struct TQuery {
public:
    inline TQuery() noexcept
        : Region(-1)
    {
    }

    inline TQuery(const TString& text, TCateg region) noexcept
        : Text(text)
        , Region(region)
    {
    }

    inline bool operator < (const TQuery& other) const noexcept {
        int res = TString::compare(Text, other.Text);
        if (res != 0) {
            return res < 0;
        }

        return Region < other.Region;
    }

    inline bool operator == (const TQuery& other) const noexcept {
        return Region == other.Region && Text == other.Text;
    }

    TString Text;
    TCateg Region;
};

class HashTQueryFunc {
public:
    ui32 operator()(const TQuery& res) const {
        return MurmurHash<ui32>(res.Text.data(), res.Text.size()) + res.Region;
    }
};

void ReadAssessData(const char *pszAssessFileName,
    TVector<TQuery> *queries, TVector<int> *pQueryAges,
    TVector<TString> *pDocs, THashMap<TString,int> *pDocId,
    SHost2Group *pHost2Group,
    TResultHash *pEstimates, bool storeMarks);
