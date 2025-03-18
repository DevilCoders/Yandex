#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

template <class T>
class TCatalogus: TNonCopyable {
private:
    typedef TVector<T*> TVectorType;
    typedef THashMap<TString, size_t> THash;

    TVectorType Vector;
    THash Hash;

public:
    typedef T value_type;
    typedef typename TVectorType::const_iterator const_iterator;
    typedef typename TVectorType::iterator iterator;

    TCatalogus() {
    }

    ~TCatalogus() {
        for (typename TVectorType::iterator i = Vector.begin(); i != Vector.end(); ++i)
            delete *i;
    }

    const_iterator begin() const {
        return Vector.begin();
    }

    const_iterator end() const {
        return Vector.end();
    }

    iterator begin() {
        return Vector.begin();
    }

    iterator end() {
        return Vector.end();
    }

    const_iterator find(const TString& name) const {
        typename THash::const_iterator i = Hash.find(name);

        if (i == Hash.end()) {
            return end();
        } else {
            return begin() + i->second;
        }
    }

    iterator find(const TString& name) {
        typename THash::iterator i = Hash.find(name);

        if (i == Hash.end()) {
            return end();
        } else {
            return begin() + i->second;
        }
    }

    bool push_back(T* what, const TString& name) {
        if (Hash.insert(std::make_pair(name, Vector.size())).second) {
            Vector.push_back(what);
            return true;
        } else {
            return false;
        }
    }

    void swap(TCatalogus& right) {
        Vector.swap(right.Vector);
        Hash.swap(right.Hash);
    }

    void reset() {
        TCatalogus().swap(*this);
    }

    size_t size() const {
        return Vector.size();
    }
};
