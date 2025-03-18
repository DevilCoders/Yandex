#include "param_list_manager.h"

#include <util/generic/yexception.h>

TParamListManager::TListReference TParamListManager::GetOrCreateList(
        const TParamListManager::TListType& strings)
{
    ListToReference.find(strings);
    TListToReference::const_iterator it = ListToReference.find(strings);
    if (it != ListToReference.end()) {
        return it->second;
    }

    ui32 n = Lists.size();
    TListReference listReference(n);

    Lists.push_back(TIdForString(n, strings));
    ListToReference[strings] = listReference;

    return listReference;
}

TVector<TParamListManager::TListReference> TParamListManager::GetOrCreateLists(
        const TVector<TParamListManager::TListType>& stringss)
{
    TVector<TParamListManager::TListReference> r;
    r.reserve(stringss.size());
    for (TVector<TParamListManager::TListType>::const_iterator strings = stringss.begin();
            strings != stringss.end();
            ++strings)
    {
        r.push_back(GetOrCreateList(*strings));
    }
    return r;
}

const TParamListManager::TListById& TParamListManager::GetList(
        const TListReference& listReference) const
{
    if (listReference == TListReference()) {
        ythrow yexception() << "uninitialized TListReference";
    }
    const TListById& r = Lists.at(listReference.N);
    if (r.GetListId() != listReference.N) {
        ythrow yexception() << "list id mismatch";
    }
    return r;
}
