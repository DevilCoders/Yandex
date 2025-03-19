#include "categseries.h"

#include <util/system/yassert.h>
#include <util/generic/algorithm.h>

void TCategSeries::AddCategSlow(TCateg c) {
    if (CategCount == MAX_CATEG_COUNT) {
        Y_ASSERT(Categs.empty());
        Categs.assign(CategBuf, CategBuf + CategCount);
    }
    Categs.push_back(c);
    ++CategCount;
    Y_ASSERT(CategCount == Categs.size());
}

void TCategSeries::Sort() {
    if (CategCount > MAX_CATEG_COUNT) {
        ::Sort(Categs.begin(), Categs.end());
        for (int i = 0; i < MAX_CATEG_COUNT; ++i)
            CategBuf[i] = Categs[i];
    } else {
        Y_ASSERT(Categs.empty());
        ::Sort(CategBuf, CategBuf + CategCount);
    }

}

void TCategSeries::SortAndUnique() {
    if (CategCount > MAX_CATEG_COUNT) {
        ::Sort(Categs.begin(), Categs.end());
        Categs.erase(::Unique(Categs.begin(), Categs.end()), Categs.end());
        CategCount = Categs.size();
        if (CategCount > MAX_CATEG_COUNT) {
            for (size_t i = 0; i < MAX_CATEG_COUNT; ++i)
                CategBuf[i] = Categs[i];
        } else {
            for (size_t i = 0; i < CategCount; ++i)
                CategBuf[i] = Categs[i];
            Categs.clear();
        }
    } else {
        Y_ASSERT(Categs.empty());
        ::Sort(CategBuf, CategBuf + CategCount);
        CategCount = ::Unique(CategBuf, CategBuf + CategCount) - CategBuf;
    }
}

void TCategSeries::Reset(const TCategSeries& other) {
    CategCount = other.CategCount;
    memcpy(CategBuf, other.CategBuf, sizeof(CategBuf));
    Categs = other.Categs;
}

bool TCategSeries::operator != (const TCategSeries& other) const {
    return !(operator == (other));
}

bool TCategSeries::operator == (const TCategSeries& other) const {
    if (CategCount != other.CategCount) {
        return false;
    }

    if (memcmp(CategBuf, other.CategBuf, sizeof(CategBuf)) != 0) {
        return false;
    }

    if (Categs != other.Categs) {
        return false;
    }

    return true;
}

TCategSeries::TCategSeries()
    : CategCount(0)
{
    memset(CategBuf, 0, sizeof(CategBuf));
}

void TCategSeries::AddCateg(TCateg c) {
    if (CategCount >= MAX_CATEG_COUNT) {
        AddCategSlow(c);
    } else
        CategBuf[CategCount++] = c;
}

const TCateg *TCategSeries::Begin() const {
    if (CategCount <= MAX_CATEG_COUNT)
        return &CategBuf[0];
    else
        return &*Categs.begin();
}

const TCateg *TCategSeries::End() const {
    return Begin() + CategCount;
}

TCateg TCategSeries::GetCateg(size_t i) const {
    Y_ASSERT(i < CategCount);
    if (i < MAX_CATEG_COUNT)
        return CategBuf[i];
    else
        return Categs[i];
}

bool TCategSeries::Has(TCateg c) const {
    if (CategCount <= MAX_CATEG_COUNT)
        return ::Find(CategBuf, CategBuf + CategCount, c) != CategBuf + CategCount;
    else {
        Y_ASSERT(!Categs.empty());
        return ::Find(Categs.begin(), Categs.end(), c) != Categs.end();
    }
}

void TCategSeries::Clear() {
    if (CategCount > MAX_CATEG_COUNT)
        Categs.erase(Categs.begin(), Categs.end());
    else
        Y_ASSERT(Categs.empty());
    CategCount = 0;
}

bool TCategSeries::Empty() const {
    return CategCount == 0;
}

size_t TCategSeries::size() const {
    return CategCount;
}
