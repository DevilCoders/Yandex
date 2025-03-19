#pragma once

#include <kernel/search_types/search_types.h>

#include <util/generic/vector.h>

class TCategSeries {

public:
    TCategSeries();

    void AddCateg(TCateg c);

    const TCateg *Begin() const;

    const TCateg *End() const;

    TCateg GetCateg(size_t i) const;

    bool Has(TCateg c) const;

    void Clear();

    void Sort();
    void SortAndUnique();

    bool Empty() const;

    size_t size() const;

    void Reset(const TCategSeries& other);

    bool operator != (const TCategSeries& other) const;
    bool operator == (const TCategSeries& other) const;

private:
    enum {
        MAX_CATEG_COUNT = 7
    };
    void AddCategSlow(TCateg c);

private:
    size_t CategCount;
    TCateg CategBuf[MAX_CATEG_COUNT];
    TVector<TCateg> Categs;
};

