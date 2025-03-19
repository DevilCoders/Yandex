#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/cast.h>
#include <util/generic/algorithm.h>

#include <kernel/qtree/richrequest/range.h>
#include <kernel/qtree/richrequest/serialization/flags.h>

#include <array>

namespace NRichTreeProtocol {
    class TMarkupDataBase;
}

namespace NSearchQuery {

class TMarkupDataBase;
using TMarkupDataPtr = TIntrusivePtr<TMarkupDataBase>;

class TMarkupDataBase: public TAtomicRefCount<TMarkupDataBase> {
public:
    virtual ~TMarkupDataBase() = default;

    // defined in TMarkupData below
    virtual EMarkupType MarkupType() const = 0;

    bool EqualTo(const TMarkupDataBase& rhs) const {
        if (MarkupType() != rhs.MarkupType())
            return false;
        return DoEqualTo(rhs);
    }

    template <class TConcreteMarkupData>
    TConcreteMarkupData& As() {
        return *CheckedCast<TConcreteMarkupData*>(this);
    }

    template <class TConcreteMarkupData>
    const TConcreteMarkupData& As() const {
        return *CheckedCast<const TConcreteMarkupData*>(this);
    }

    // called by TRichRequestNode::AddMarkup, if inserting same-type markup at the same coordinates
    // return true if the new markup shouldn't be inserted
    virtual bool Merge(TMarkupDataBase& /*newNode*/) {
        return false; // not processed
    }

    virtual TMarkupDataPtr Clone() const = 0;

    // return false, if you don't want serialize this object
    virtual bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const = 0;

    // return NULL if this is not an object of proper type
 /* static TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message);*/
 protected:
    virtual bool DoEqualTo(const TMarkupDataBase& rhs) const = 0;
};

template <EMarkupType T>
class TMarkupData: public TMarkupDataBase {
public:
    static const EMarkupType SType = T;

    EMarkupType MarkupType() const override {
        return T;
    }
protected:
    TMarkupData() = default;
};

struct TMarkupItem {
    TRange Range;
    TMarkupDataPtr Data;

    TMarkupItem(const TMarkupItem& src) = default;
    TMarkupItem(const TRange& range, TMarkupDataPtr data)
        : Range(range)
        , Data(std::move(data))
    {
    }

    bool operator == (const TMarkupItem& rhs) const {
        return Range == rhs.Range && Data->EqualTo(*rhs.Data);
    }

    TMarkupItem& operator = (TMarkupItem&& src) = default;

    template <class TConcreteMarkupData>
    TConcreteMarkupData& GetDataAs() const {
        return Data->As<TConcreteMarkupData>();
    }
};

class TMarkup {
template <class T1, class T2> friend class TAllMarkupIteratorBase;
public:
    using TItems = TVector<TMarkupItem>;
private:
    using TItemsArray = std::array<TItems, MT_LIST_SIZE>;
    TItemsArray ItemsArray;
public:
    bool operator ==(const TMarkup& rhs) const {
        return ItemsArray == rhs.ItemsArray;
    }
    bool Empty() const {
        for (const TItems& items : ItemsArray) {
            if (!items.empty())
                return false;
        }
        return true;
    }
    template <class TConcreteMarkupData>
    const TItems& GetItems() const {
        return ItemsArray[TConcreteMarkupData::SType];
    }
    template <class TConcreteMarkupData>
    TItems& GetItems() {
        return ItemsArray[TConcreteMarkupData::SType];
    }
    const TItems& GetItems(EMarkupType type) const {
        return ItemsArray[type];
    }
    TItems& GetItems(EMarkupType type) {
        return ItemsArray[type];
    }
    void Add(const TMarkupItem& i) {
        ItemsArray[i.Data->MarkupType()].emplace_back(i);
    }
    template <class TIterator>
    void Add(const TIterator& it_) {
        for (TIterator it(it_); !it.AtEnd(); ++it)
            Add(*it);
    }
    void Clear() {
        for (TItems& items : ItemsArray) {
            items.clear();
        }
    }
    void Swap(TMarkup& m) {
        ItemsArray.swap(m.ItemsArray);
    }

    void Sort() {
        for (TItems& items : ItemsArray)
            ::StableSort(items.begin(), items.end(), ItemOrder);
    }

    void DeepSort();
private:
    static bool ItemOrder(const TMarkupItem& a, const TMarkupItem& b) {
        return a.Range < b.Range;
    }
};

} // namespace
