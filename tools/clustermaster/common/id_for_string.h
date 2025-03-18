#pragma once

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class TIdForString {
public:
    typedef ui32 TListId;
    typedef ui32 TId;

private:
    typedef THashMap<TString, ui32> TIdByName;
    TIdByName IdByName;

    typedef TVector<TString> TNameById;
    TNameById NameById;

    const ui32 ListId;

private:
    TId GetOrCreateIdByName(const TString& name);

public:
    TIdForString(TListId listId, const TVector<TString>& names);

    TListId GetListId() const { return ListId; }

    void CheckListId(TListId listId) const;

    TId Size() const { return NameById.size(); }

    struct TIdSafe {
        const TListId ListId;
        const TId Id;

        TIdSafe()
            : ListId(Max<TListId>())
            , Id(Max<TId>())
        { }

        TIdSafe(TListId listId, TId id)
            : ListId(listId)
            , Id(id)
        { }

        TIdSafe(const TIdSafe& that) = default;

        TIdSafe& operator++() {
            ++const_cast<TId&>(Id);
            return *this;
        }

        TIdSafe& operator=(const TIdSafe& that) {
            const_cast<TId&>(Id) = that.Id;
            const_cast<TId&>(ListId) = that.ListId;
            return *this;
        }

        bool operator==(const TIdSafe& that) const {
            return this->Id == that.Id && this->ListId == that.ListId;
        }

        bool operator!=(const TIdSafe& that) const {
            return !(*this == that);
        }
    };

    const TString& GetNameById(TId id) const {
        if (id >= IdByName.size())
            ythrow yexception() << "wrong id: " << id << ", id count: " << IdByName.size();
        return NameById.at(id);
    }

    const TString& GetNameById(const TIdSafe& id) const {
        CheckListId(id.ListId);
        return GetNameById(id.Id);
    }

    const TString& GetSingleName() const {
        if (Size() != 1) {
            ythrow yexception() << "need single name, got " << Size();
        }
        return NameById.front();
    }
    const TNameById& GetNames() const {
        return NameById;
    }
    TMaybe<TIdSafe> FindIdByName(const TString& name) const {
        TIdByName::const_iterator it = IdByName.find(name);
        if (it == IdByName.end()) {
            return TMaybe<TIdSafe>();
        } else {
            return TIdSafe(ListId, it->second);
        }
    }
    TIdSafe GetIdByName(const TString& name) const {
        TMaybe<TIdSafe> id = FindIdByName(name);
        if (!id) {
            ythrow yexception() << "id not found by name " << name;
        }
        return *id;
    }
    TIdSafe GetSafeId(TId id) const {
        if (id >= Size()) {
            ythrow yexception() << "invalid id " << id << ", max allowed " << Size();
        }
        return TIdSafe(ListId, id);
    }
    TIdSafe GetFirstId() const {
        return GetSafeId(0);
    }
    TIdSafe GetLastId() const {
        return GetSafeId(Size() - 1);
    }
    bool HasName(const TString& name) const {
        return IdByName.find(name) != IdByName.end();
    }

    class TIterator {
        friend class TIdForString;
    private:
        const TIdForString& Parent;
        TMaybe<TIdSafe> Current;
        TIterator(const TIdForString& parent);
    public:
        bool Next();
        const TIdSafe& operator*() const;
        const TIdSafe* operator->() const;
        const TString& GetName() const;
    };

    TIterator Iterator() const {
        return TIterator(*this);
    }

};


template <>
struct THash<TIdForString::TIdSafe> {

    size_t operator()(const TIdForString::TIdSafe& v) const {
        return v.Id ^ v.ListId;
    }

};
