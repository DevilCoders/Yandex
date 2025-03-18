#include "id_for_string.h"

#include "vector_to_string.h"

#include <util/generic/bt_exception.h>

TIdForString::TId TIdForString::GetOrCreateIdByName(const TString& name) {
    TIdByName::const_iterator it = IdByName.find(name);
    if (it == IdByName.end()) {
        TId id = NameById.size();
        NameById.push_back(name);
        IdByName[name] = id;
        return id;
    } else {
        return it->second;
    }
}


TIdForString::TIdForString(TIdForString::TListId listId, const TVector<TString>& names)
    : ListId(listId)
{
    for (TVector<TString>::const_iterator it = names.begin(); it != names.end(); ++it) {
        GetOrCreateIdByName(*it);
    }
    if (Size() != names.size()) {
        ythrow yexception() << "there are non-unique names in list: " << ToString(names) << ".";
    }
}

void TIdForString::CheckListId(TIdForString::TListId listId) const {
    if (ListId != listId)
        ythrow TWithBackTrace<yexception>() << "list mismatch, got " << listId << ", expecting " << ListId;
}

template <>
void Out<TIdForString::TIdSafe>(IOutputStream& os, TTypeTraits<TIdForString::TIdSafe>::TFuncParam id) {
    os << "(" << id.Id << ",l=" << id.ListId << ")";
}

TIdForString::TIterator::TIterator(const TIdForString& parent)
    : Parent(parent)
{ }

bool TIdForString::TIterator::Next() {
    if (Parent.Size() == 0)
        return false;

    if (!Current) {
        Current = TIdSafe(Parent.GetListId(), 0);
        return true;
    } else {
        if (Current->Id + 1 == Parent.Size())
            return false;
        ++*Current;
        return true;
    }
}

const TIdForString::TIdSafe& TIdForString::TIterator::operator *() const {
    return *Current;
}

const TIdForString::TIdSafe* TIdForString::TIterator::operator ->() const {
    return &**this;
}

const TString& TIdForString::TIterator::GetName() const {
    return Parent.GetNameById(**this);
}

template <>
void Out<TIdForString>(IOutputStream& out, TTypeTraits<TIdForString>::TFuncParam list) {
    out << ToString(list.GetNames());
}
