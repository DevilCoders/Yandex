#pragma once

#include "categseries.h"
#include "rcu_hash.h"

#include <kernel/search_types/search_types.h>

#include <library/cpp/on_disk/st_hash/static_hash.h>

#include <util/digest/numeric.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/system/defaults.h>
#include <util/system/fs.h>

#include <kernel/doom/wad/mega_wad_reader.h>
#include <kernel/doom/standard_models_storage/standard_models_storage.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/key/fat_key_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/key/key_sampler.h>
#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/offset/tuple_sub_offset.h>

class TBlob;
class TFixedBufferFileOutput;

namespace NGroupingAttrs {

extern const char* const NONAME;

class IMetaDb {
public:
    virtual ~IMetaDb() {};
    virtual void Load(const char* begin, const char* end, bool useRev) = 0;
    virtual void LoadFromPath(const TString& file, bool useRev) = 0;
    virtual void Save(TFixedBufferFileOutput& f) const = 0;
};

class TClassRelationsInfo : public IMetaDb {
private:
    typedef TVector<TCateg> TClassRelations;
    TClassRelations C2PMap;

protected:

    void Load(const char* begin, const char* end, bool useRev) override;

    void LoadFromPath(const TString& /* path */, bool /* useRev */) override {
        Y_FAIL("not used");
    }

    void Save(TFixedBufferFileOutput& f) const override;

    TCateg GetMaxKey() const {
        if (C2PMap.empty())
            return 0;
        return (TCateg)(C2PMap.size() - 1);
    }

public:
    void CategPath(TCateg aClass, TCategSeries *path) const {
        path->Clear();
        while (1) {
            if (aClass != END_CATEG)
                path->AddCateg(aClass);
            if (!IsValidCateg(aClass))
                break;
            aClass = C2PMap[(size_t)aClass];
        }
    }

    bool HasValueInCategPath(TCateg toSearch, TCateg fromSearch) const {
        if (fromSearch == toSearch)
            return true;
        while (IsValidCateg(fromSearch)) {
            fromSearch = C2PMap[(size_t)fromSearch];
            if (fromSearch == toSearch)
                return true;
        }
        return false;
    }

    TCateg Categ2Parent(TCateg aClass) const {
        if (!IsValidCateg(aClass))
            return END_CATEG;
        return C2PMap[(size_t)aClass];
    }

private:
    bool IsValidCateg(TCateg aClass) const {
        return (END_CATEG != aClass) && (aClass >= 0) && ((size_t)aClass < C2PMap.size());
    }

};

class TClassCoeffsInfo : public IMetaDb {
private:
    typedef THashMap<TCateg, float> TClassCoeffs;
    TClassCoeffs C2CoeffMap;

protected:
    void Load(const char* begin, const char* end, bool useRev) override;
    void Save(TFixedBufferFileOutput& f) const override;

public:

    void LoadFromPath(const TString& /* path */, bool /* useRev */) override {
        Y_FAIL("not used");
    }

    float CategCoeff(TCateg aClass) const {
        TClassCoeffs::const_iterator i = C2CoeffMap.find(aClass);
        if (i == C2CoeffMap.end())
            return 1;
        return i->second;
    }

    bool HasCoeffs() const {
        return !C2CoeffMap.empty();
    }
};

class TC2LInfo : public IMetaDb {
private:
    typedef TVector<TCateg> TCategList;
    typedef TVector<TCategList> TLinksList;

    TLinksList C2LMap;

protected:
    void Load(const char* begin, const char* end, bool useRev) override;
    void Save(TFixedBufferFileOutput& f) const override;

public:
    void LoadFromPath(const TString& /* path */, bool /* useRev */) override {
        Y_FAIL("not used");
    }

    TCategSeries &CategLinks(TCateg aClass, TCategSeries &path) const;
    bool IsLink(TCateg parent, TCateg link) const;
};

class IClassNamesStorage {
public:
    virtual ~IClassNamesStorage() {};

    virtual void Load(const char* begin, const char* end, bool useRev) = 0;
    virtual void LoadFromPath(const TString& path, bool useRev) = 0;
    virtual void Save(TFixedBufferFileOutput& f) const = 0;

    virtual const char* Categ2Name(TCateg aClass) const = 0;
    virtual TCateg Name2Categ(const TString& name) const = 0;

    virtual TCateg GetMaxKey() const = 0;
    virtual size_t CategCount() const = 0;
};

class TClassNamesInfo : public IMetaDb {
public:
    TClassNamesInfo(IClassNamesStorage* impl)
        : Impl(impl)
    {}

    void Load(const char* begin, const char* end, bool useRev) override {
        Impl->Load(begin, end, useRev);
    }

    void LoadFromPath(const TString& path, bool useRev) override {
        Impl->LoadFromPath(path, useRev);
    }

    void Save(TFixedBufferFileOutput& f) const override {
        Impl->Save(f);
    }

    const char* Categ2Name(TCateg aClass) const {
        return Impl->Categ2Name(aClass);
    }

    TCateg Name2Categ(const TString& name) const {
        return Impl->Name2Categ(name);
    }

    TCateg GetMaxKey() const {
        return Impl->GetMaxKey();
    }

    size_t CategCount() const {
        return Impl->CategCount();
    }

    const IClassNamesStorage* GetConstClassNamesStorage() const {
        return Impl.Get();
    }
    IClassNamesStorage* GetClassNamesStorage() {
        return Impl.Get();
    }

private:
    THolder<IClassNamesStorage> Impl;
};


/* We know that in h.c2n and in d.c2n all the values are less then ui32 and -1 never exists
   and we know that in TFatSearcher all the strings are null terminated because we return const char *. If you want
   to improve this, be sure that asan works */
class TOffroadWadStaticClassNames : public IClassNamesStorage {
    using TKeySeeker = NOffroad::TFatKeySeeker<ui32, NOffroad::TNullSerializer>;
    using TKeyReader = NOffroad::TKeyReader<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
    using TKeySearcher = NOffroad::TFatSearcher<ui32, NOffroad::TNullSerializer>;
    using TModel = TKeyReader::TModel;
    using TTable = TKeyReader::TTable;
public:
    TOffroadWadStaticClassNames(bool lockMemory)
        : LockMemory_(lockMemory)
    {}

    void Load(const char*, const char*, bool) override {
        Y_ENSURE(false, "Offroad class names do not support Load operation");
    }

    void LoadFromPath(const TString& path, bool useRev) override {
        UseRev_ = useRev;
        Y_ENSURE(NFs::Exists(path));

        Wad_ = NDoom::IWad::Open(path, LockMemory_);

        FatCategToName_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::CategToNameIndexType, NDoom::EWadLumpRole::KeyFat));
        FatSubCategToName_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::CategToNameIndexType, NDoom::EWadLumpRole::KeyIdx));
        KeySearcher_.Reset(FatCategToName_, FatSubCategToName_);

        if (useRev) {
            Model_ = MakeHolder<TModel>();
            Model_->Load(Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::NameToCategIndexType, NDoom::EWadLumpRole::KeysModel)));
            Table_ = MakeHolder<TTable>(*Model_);
            FatNameToCateg_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::EWadIndexType::NameToCategIndexType, NDoom::EWadLumpRole::KeyFat));
            FatSubNameToCateg_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::EWadIndexType::NameToCategIndexType, NDoom::EWadLumpRole::KeyIdx));
            KeyNameToCateg_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::EWadIndexType::NameToCategIndexType, NDoom::EWadLumpRole::Keys));
            KeySeeker_ = MakeHolder<TKeySeeker>(FatNameToCateg_, FatSubNameToCateg_);
        }
    }

    void Save(TFixedBufferFileOutput&) const override {
        Y_ENSURE(false, "Not used by basesearch");
    }

    const char* Categ2Name(TCateg aClass) const override {
        if (aClass < 0 || static_cast<ui64>(aClass) > CategCount()) {
            return NONAME;
        }
        return KeySearcher_.ReadKey(aClass).data();
    }

    TCateg Name2Categ(const TString& name) const override {
        if (!UseRev_) {
            return END_CATEG;
        }
        if (name.empty()) {
            return END_CATEG;
        }
        TKeyReader reader(Table_.Get(), KeyNameToCateg_);
        TStringBuf key = "";
        ui32 data = 0;
        if (!KeySeeker_->LowerBound(name, &key, &data, &reader) || key != name) {
            return END_CATEG;
        }
        return data;
    }

    TCateg GetMaxKey() const override {
        return KeySearcher_.Size() - 1;
    }

    size_t CategCount() const override {
        return KeySearcher_.Size() - 1;
    }

private:
    THolder<NDoom::IWad> Wad_;

    TKeySearcher KeySearcher_;
    THolder<TKeySeeker> KeySeeker_;

    TBlob FatNameToCateg_;
    TBlob FatSubNameToCateg_;
    TBlob KeyNameToCateg_;

    TBlob FatCategToName_;
    TBlob FatSubCategToName_;

    THolder<TTable> Table_;
    THolder<TModel> Model_;
    bool UseRev_ = false;
    bool LockMemory_ = false;
};

class TStaticClassNames : public IClassNamesStorage {
public:
    TStaticClassNames()
        : C2NSthash(nullptr)
    {}

    void Load(const char* begin, const char* end, bool useRev) override;
    void Save(TFixedBufferFileOutput& f) const override;

    void LoadFromPath(const TString& /* path */, bool /* useRev */) override {
        Y_FAIL("Not implemented");
    }

    const char* Categ2Name(TCateg aClass) const override {
        if (aClass == END_CATEG)
            return NONAME;
        if (!C2NSthash)
            return NONAME;
        TClassNamesSthash::const_iterator it = C2NSthash->find(aClass);
        if (it == C2NSthash->end())
            return NONAME;
        return it.Value();
    }

    TCateg Name2Categ(const TString& name) const override {
        TNameValuesHash::const_iterator i = N2CHash.find(name.data());
        if (i == N2CHash.end())
            return END_CATEG;
        return i->second;
    }

    TCateg GetMaxKey() const override {
        if (!C2NSthash)
            return 0;
        TCateg res = 0;
        TClassNamesSthash::const_iterator it = C2NSthash->begin();
        for (; it != C2NSthash->end(); ++it)
            if (it.Key() > res)
                res = it.Key();
        return res;
    }

    size_t CategCount() const override {
        if (!C2NSthash)
            return 0;
        return (size_t)C2NSthash->size();
    }

private:
    TBuffer ClassNamesSthashBuf;

    typedef sthash<TCateg, const char*, THash<TCateg> > TClassNamesSthash;
    const TClassNamesSthash* C2NSthash;

    typedef THashMap<const char*, TCateg> TNameValuesHash;
    TNameValuesHash N2CHash;
};

class TDynamicClassNames : public IClassNamesStorage {
public:
    typedef intptr_t TSafeCateg; // (for binary compatibility with 32 and 64 bits processors)

    struct TSafeCategHash {
        size_t operator()(const TSafeCateg &val) const {
            return NumericHash(val);
        }
    };

    struct TEmptyCateg {
        operator TSafeCateg () const {
            return END_CATEG;
        }
    };

    typedef TRCUHash<TSafeCateg, TString, TEmptyCateg, TSafeCategHash> TClassNames;
    typedef THashMap<TString, TCateg> TNameValues;

private:
    static inline void CheckCateg(const TCateg categId) {
        Y_VERIFY(Min<TSafeCateg>() <= categId && categId <= Max<TSafeCateg>(),
            "categ must be on %lli ... %lli borders , now %lli",
            (long long int)Min<TSafeCateg>(),
            (long long int)Max<TSafeCateg>(),
            (long long int)categId
        );
    }

    TClassNames C2NMap;
    TNameValues N2CMap;

    TCateg GetMaxKey() const override {
        return (TCateg)C2NMap.GetMaxKey();
    }

    TCateg NewCateg(const TString& name);

public:
    TDynamicClassNames()
        : C2NMap(10 * 1024, 0.8f) //reserve for realtime!!!!
    {}

    void Load(const char* begin, const char* end, bool useRev) override;
    void Save(TFixedBufferFileOutput& f) const override;

    void LoadFromPath(const TString& /* path */, bool /* useRev */) override {
        Y_FAIL("Not implemented");
    }

    const char* Categ2Name(TCateg aClass) const override {
        CheckCateg(aClass);
        if (aClass == END_CATEG)
            return NONAME;
        TClassNames::TKeyValue* i = C2NMap->Find((TSafeCateg)aClass);
        if (!i)
            return NONAME;
        return i->second.data();
    }
    // This call is not thread safe.
    TCateg Name2Categ(const TString& name) const override {
        TNameValues::const_iterator i = N2CMap.find(name);
        if (i == N2CMap.end())
            return END_CATEG;
        return i->second;
    }

    size_t CategCount() const override {
        return +C2NMap;
    }

    // called from TMetainfoCreator::AddCateg
    TCateg AddCateg(const TString& name);

    // realtime specific:

    void AddCateg(const TString& name, TCateg categ, TClassNames& workStorage);

    void SetCateg2NameCounters(THashMap<TSafeCateg, size_t>* categCounts) {
        C2NMap.SetKeysCounts(categCounts);
    }

    // need in TRCUHashManager::FinalizeVersion and TRCUHashManager construnctor (inc ref counter of hash, and use new container in resize)
    TClassNames& GetClassNames() {
        return C2NMap;
    }

    void DeleteCateg(TCateg categId) {
        CheckCateg(categId);
        TString name = Categ2Name(categId);
        if (name != NONAME)
            N2CMap.erase(name);
        // no need from C2NMap deletion.
    }

    const TNameValues& GetN2C() const {
        return N2CMap;
    }
};

// for Oxygen.
inline const TDynamicClassNames* GetConstDynamicClassNames(const TClassNamesInfo& info) {
    return (const TDynamicClassNames*)info.GetConstClassNamesStorage();
}
// for realsearch and TMetainfoCreator
inline TDynamicClassNames* GetDynamicClassNames(TClassNamesInfo& info) {
    return (TDynamicClassNames*)info.GetClassNamesStorage();
}

class TMetainfo :
    public TClassRelationsInfo,
    public TClassCoeffsInfo,
    public TC2LInfo,
    public TClassNamesInfo,
    TNonCopyable
{
public:
    enum TDbName {C2P, C2N, C2Co, C2L};

    explicit TMetainfo(bool dynamicC2N, bool useWad = false, bool lockMemory = false)
        : TClassNamesInfo(dynamicC2N ? static_cast<IClassNamesStorage*>(new TDynamicClassNames()) :
                         (useWad ? static_cast<IClassNamesStorage*>(new TOffroadWadStaticClassNames(lockMemory)) : static_cast<IClassNamesStorage*>(new TStaticClassNames())))
    {}

    ~TMetainfo() override {}

    void Print(const char* filename, TDbName dbName);

    void Scan(const char* filename, TDbName dbName, bool use_rev = false);
    void Scan(IInputStream& stream, TDbName dbName, bool use_rev = false);
    void ScanStroka(const TString& str, TDbName dbName, bool use_rev = false);
    void LoadMap(TDbName db, bool userev, const char* filename);
    void LoadC2N(const TString& filename, const TString& attrname);
    void LoadC2NOffroadWad(const TString& filename, const TString& attrname);

    TCateg CalcUpperBound() const;

private:
    IMetaDb* GetMetaDb(TDbName dbName);
    void ScanBlob(const TBlob& blob, TDbName dbName, bool use_rev = false);
};

class IIterator;
bool HasValueInAttrPath(IIterator* attrs, const TMetainfo* pAttr, ui32 nAttr, TCateg value);

template <typename TContainer, typename TFunction>
void ScanContainer(const char* begin, const char* end, TContainer* c, TFunction callback) {
    const char* cur = nullptr;
    const char* ptr = begin;
    while (ptr < end) {
        cur = (const char*)memchr(ptr, '\n', end - ptr);

        if (!cur) {  // newline not found
            cur = end;
        }

        if (ptr != cur) {
            const char* del = (const char*)memchr(ptr, '\t', cur - ptr);
            // See SEARCH-2961 crash report
            Y_ENSURE(del, "Missing delimiter at " << TString(ptr, cur - ptr).Quote() << ". ");
            callback(c, ptr, del - ptr, del + 1, cur - del - 1);
        }

        ptr = cur + 1;
    }
}

void ScanClassNames(const TString& filename, THashMap<TCateg, TString>& res);

}
