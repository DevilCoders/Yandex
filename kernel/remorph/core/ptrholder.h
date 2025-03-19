#pragma once

#include <util/generic/ptr.h>
#include <util/generic/hash.h>

namespace NRemorph {

template <class T, class TIdType>
struct TPtrHolder {
    typedef TIntrusivePtr<T> TPtr;
    typedef THashMap<TIdType, TPtr> TStorage;
    TStorage Storage;
    T* Store(T* p) {
        TPtr t(p);
        if (Storage.size() > Max<TIdType>()) {
            ythrow yexception() << "Too many ids";
        }
        t->Id = Storage.size();
        return Storage.insert(std::make_pair(t->Id, t)).first->second.Get();
    }
    const T* Get(TIdType id) const {
        typename TStorage::const_iterator it = Storage.find(id);
        return it == Storage.end() ? nullptr : it->second.Get();
    }
};

} // NRemorph
