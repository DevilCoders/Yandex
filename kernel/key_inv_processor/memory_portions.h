#pragma once

#include <kernel/keyinv/indexfile/indexstorageface.h>
#include <kernel/keyinv/indexfile/memoryportion.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class TMemoryPortionUsage: public IYndexStorage {
private:
    TVector< TAutoPtr<NIndexerCore::TMemoryPortion> > Portions;
    IYndexStorage::FORMAT Format;
    int Counter, MaxDocs, PortionNum;
    TString Prefix, Suffix;
    TString Dir;
    const bool BuildByKeysFlag;

public:
    TMemoryPortionUsage(IYndexStorage::FORMAT format, int maxDocs, TString prefix, TString suffix, TString dir, const bool buildByKeysFlag);
    ~TMemoryPortionUsage() override {
        VERIFY_WITH_LOG(!Counter, "Incorrect MemoryPortions usage");
    }

    bool Close() {
        return StoreResult();
    }

    virtual void StorePositions(const char* keyText, SUPERLONG* positions, size_t posCount) override {
        Portions.back()->StorePositions(keyText, positions, posCount);
    }

    bool IncDoc();

private:
    bool StoreResult();

private:
    TAtomicSharedPtr<NIndexerCore::TMemoryPortion> GetResultAndFlush();
};

class TMemoryPortionAttr: public TMemoryPortionUsage {
public:
    explicit TMemoryPortionAttr(IYndexStorage::FORMAT format, int maxDocs, TString prefix, TString suffix, TString dir, const bool buildByKeysFlag)
        : TMemoryPortionUsage(format, maxDocs, prefix, suffix, dir, buildByKeysFlag) {
    }

    virtual void StorePositions(const char* keyText, SUPERLONG* positions, size_t posCount) override {
        if (*keyText == '#' || *keyText == '(' || *keyText == ')')
            TMemoryPortionUsage::StorePositions(keyText, positions, posCount);
        VERIFY_WITH_LOG(keyText[0] != '(' || keyText[1] != 'z' || keyText[2] < '0' || keyText[2] > '9', "custom parser config error");
    }
};

class TMemoryPortionBody: public TMemoryPortionUsage {
public:
    explicit TMemoryPortionBody(IYndexStorage::FORMAT format, int maxDocs, TString prefix, TString suffix, TString dir, const bool buildByKeysFlag)
        : TMemoryPortionUsage(format, maxDocs, prefix, suffix, dir, buildByKeysFlag) {
    }

    virtual void StorePositions(const char* keyText, SUPERLONG* positions, size_t posCount) override {
        if (*keyText == '#' || *keyText == '(' || *keyText == ')')
            return;
        TMemoryPortionUsage::StorePositions(keyText, positions, posCount);
    }
};
