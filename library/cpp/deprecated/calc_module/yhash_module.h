#pragma once

#include "copy_points.h"
#include "simple_module.h"

#include <util/generic/hash.h>

template <class K, class T>
class TYHashModule: public TSimpleModule {
public:
    typedef K TKey;
    typedef T TValue;
    typedef THashMap<K, T> THash;
    typedef typename THash::iterator iterator;
    typedef typename THash::const_iterator const_iterator;

private:
    THolder<THash> Hash;

    TYHashModule()
        : TSimpleModule("TYHashModule")
    {
        Bind(this).template To<const TKey&, const TValue&, TValue*&, &TYHashModule::Insert>("insert");
        Bind(this).template To<const TKey&, TValue*&, &TYHashModule::Find>("find");
        Bind(this).template To<const TKey&, TValue*&, &TYHashModule::GetAt>("get_at");
        Bind(this).template To<size_t, &TYHashModule::GetSize>("size");
        Bind(this).template To<TYHashModule*>(this, "module_output");
        Bind(this).template To<&TYHashModule::Start>("start");
        Bind(this).template To<&TYHashModule::Finish>("finish");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TYHashModule;
    }

    class TYHashModuleIterator: public TSimpleModule {
    private:
        TMasterCopyPoint<TYHashModule*> HashPublisherPoint;
        iterator ForIteration;
        iterator CurrentIterator;
        iterator EndIterator;
        bool Eof;

        TYHashModuleIterator()
            : TSimpleModule("TYHashModuleIterator")
            , HashPublisherPoint(this, /*default=*/nullptr, "module_input")
        {
            Bind(this).template To<&TYHashModuleIterator::Prepare>("start");
            Bind(this).template To<TValue*, &TYHashModuleIterator::Next>("next");
            Bind(this).template To<&TYHashModuleIterator::Erase>("erase");
        }

    public:
        static TCalcModuleHolder BuildModule() {
            return new TYHashModuleIterator;
        }

    private:
        void Prepare() {
            EndIterator = HashPublisherPoint.GetValue()->Hash->end();
            CurrentIterator = ForIteration = HashPublisherPoint.GetValue()->Hash->begin();
            Eof = (CurrentIterator == EndIterator);
        }

        TValue* Next() {
            if (Eof) {
                return nullptr;
            }
            CurrentIterator = ForIteration;
            if (ForIteration != EndIterator) {
                ++ForIteration;
            }

            Eof = (ForIteration == EndIterator);
            return &CurrentIterator->second;
        }

        void Erase() {
            HashPublisherPoint.GetValue()->Hash->erase(CurrentIterator);
            EndIterator = HashPublisherPoint.GetValue()->Hash->end();
        }
    };

private:
    void Insert(const TKey& key, const TValue& val, TValue*& res) {
        res = &Hash->insert(std::make_pair(key, val)).first->second;
    }

    void Find(const TKey& key, TValue*& value) {
        iterator i = Hash->find(key);
        value = nullptr;

        if (i != Hash->end()) {
            value = &i->second;
        }
    }

    void GetAt(const TKey& key, TValue*& res) {
        res = &(*Hash)[key];
    }

    size_t GetSize() {
        return Hash->size();
    }
    void Start() {
        Hash.Reset(new THash());
    }
    void Finish() {
        Hash.Destroy();
    }
};
