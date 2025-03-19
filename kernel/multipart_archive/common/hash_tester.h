#pragma once

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>

template<class T>
TString GetString(T data) {
    return ToString(data);
}

template<>
TString GetString(TBlob data) {
    return TString((char*)data.Data(), data.Size());
}

template <class THash, class TValue = ui64>
class THashTester {
public:
    THashTester(THash& hash)
        : Hash(hash)
    {
    }

    bool Insert(const TString& key, TValue val) {
        TValue v;
        bool newKey = !Hash.Find(key, v);

        if (!Hash.Insert(key, val))
            return false;

        if (newKey)
            DocsCount++;

        UNIT_ASSERT_VALUES_EQUAL(Hash.Size(), DocsCount);
        UNIT_ASSERT(Hash.Find(key, v));
        UNIT_ASSERT_EQUAL(GetString(val), GetString(val));
        return true;
    }

    bool Remove(const TString& key) {
        if (!Hash.Remove(key)) {
            return false;
        }
        CHECK_WITH_LOG(DocsCount > 0);
        DocsCount--;
        TValue v;
        UNIT_ASSERT(!Hash.Find(key, v));
        UNIT_ASSERT_VALUES_EQUAL(Hash.Size(), DocsCount);
        return true;
    }
private:
    ui64 DocsCount = 0;
    THash& Hash;
};

