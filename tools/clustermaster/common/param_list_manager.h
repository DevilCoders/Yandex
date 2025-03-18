#pragma once


#include "id_for_string.h"

#include <util/digest/sequence.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

class TParamListManager {
public:
    typedef TVector<TString> TListType;
    typedef TIdForString TListById;

    class TListReference {
        friend class TParamListManager;
    public:
        TListReference()
            : N(Max<ui32>())
        { }

        TListReference(const TListReference& that) = default;

        bool operator==(const TListReference& that) const {
            return this->N == that.N;
        }

        bool operator!=(const TListReference& that) const {
            return !(*this == that);
        }

        TListReference& operator=(const TListReference& that) {
            const_cast<TListById::TListId&>(N) = that.N;
            return *this;
        }

        const TListById::TListId N;
    private:
        TListReference(ui32 n)
            : N(n)
        { }
    };

private:
    typedef THashMap<TListType, TListReference, TSimpleRangeHash> TListToReference;
    TListToReference ListToReference;

    typedef TVector<TIdForString> TLists;
    TLists Lists;

public:


    TListReference GetOrCreateList(const TListType& strings);
    TVector<TListReference> GetOrCreateLists(const TVector<TListType>& stringss);

    // for tests only
    const TIdForString& GetList(const TListReference& listReference) const;
};
