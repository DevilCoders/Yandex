#include "metainfo.h"

#include "iterator.h"

#include <util/folder/dirut.h>
#include <util/generic/cast.h>
#include <util/generic/map.h>
#include <util/memory/blob.h>
#include <util/memory/pool.h>
#include <util/stream/file.h>
#include <util/string/cast.h>


namespace NGroupingAttrs {

template <typename TContainer>
inline static void AddSimple(TContainer* c, const char* s1, size_t l1, const char* s2, size_t l2) {
    (*c)[FromString<typename TContainer::key_type>(s1, l1)] = FromString<typename TContainer::mapped_type>(s2, l2);
}

template <typename TContainer>
inline static void AddToHash(TContainer* c, const char* s1, size_t l1, const char* s2, size_t l2) {
    (*c).Add(FromString<typename TContainer::key_type>(s1, l1), FromString<typename TContainer::mapped_type>(s2, l2));
}

template <typename TContainer>
void PrintContainer(IOutputStream& f, const TContainer& c) {
    for (typename TContainer::const_iterator ci = c.begin(); ci != c.end(); ++ci) {
        f << ci->first << "\t" << ci->second << "\n";
    }
}

inline static void AddC2P(TVector<TCateg>* c, const char* s1, size_t l1, const char* s2, size_t l2) {
    TCateg categ = FromString<TCateg>(s1, l1);
    Y_ASSERT(categ >= 0);
    const size_t newSize = IntegerCast<size_t>(categ + 1);
    if (c->size() < newSize) {
        c->resize(newSize, END_CATEG);
    }
    (*c)[IntegerCast<size_t>(categ)] = FromString<TCateg>(s2, l2);
}

void TClassRelationsInfo::Load(const char* begin, const char* end, bool /*useRev*/) {
    ScanContainer(begin, end, &C2PMap, AddC2P);
}

void TClassRelationsInfo::Save(TFixedBufferFileOutput& f) const {
    TClassRelations::const_iterator ci;
    for (ci = C2PMap.begin(); ci != C2PMap.end(); ++ci)
        f << ci - C2PMap.begin() << "\t" << *ci << "\n";
}

void TClassCoeffsInfo::Load(const char* begin, const char* end, bool /*useRev*/) {
    ScanContainer(begin, end, &C2CoeffMap, AddSimple<TClassCoeffs>);
}

void TClassCoeffsInfo::Save(TFixedBufferFileOutput& f) const {
    PrintContainer(f, C2CoeffMap);
}

inline static void AddC2L(TVector< TVector<TCateg> >* c, const char* s1, size_t l1, const char* s2, size_t l2) {
    TCateg categ = FromString<TCateg>(s1, l1);
    const size_t newSize = IntegerCast<size_t>(categ + 1);
    if (c->size() < newSize) {
        c->resize(newSize);
    }
    (*c)[IntegerCast<size_t>(categ)].push_back(FromString<TCateg>(s2, l2));
}

void TC2LInfo::Load(const char* begin, const char* end, bool /*useRev*/) {
    ScanContainer(begin, end, &C2LMap, AddC2L);
}

void TC2LInfo::Save(TFixedBufferFileOutput& /*f*/) const {
    ythrow yexception() << "Not implemented"; //And not used. At least before now.
}

TCategSeries& TC2LInfo::CategLinks(TCateg aClass, TCategSeries &path) const {
    if (aClass < (TCateg)C2LMap.size() && aClass != END_CATEG) { // valid aClass has links
        for (size_t i = 0; i < C2LMap[IntegerCast<size_t>(aClass)].size(); i++) { // fill path
            path.AddCateg(C2LMap[IntegerCast<size_t>(aClass)][i]);
        }
    }
    return path;
}

bool TC2LInfo::IsLink(TCateg parent, TCateg link) const {
    if (parent == END_CATEG || link == END_CATEG || parent >= (TCateg)C2LMap.size())
        return false;
    for (unsigned i = 0; i < C2LMap[IntegerCast<size_t>(parent)].size(); i++)
        if (C2LMap[IntegerCast<size_t>(parent)][i] == link)
            return true;
    return false;
}

class TClassNamesHashAndPool {
public:
    TClassNamesHashAndPool()
        : Pool(1 << 20)
    {}

    void Add(TCateg key, const TString& value) {
        const char* s = Pool.Append(value.data());

        ClassNamesHash.insert(std::make_pair(key, s));
    }

    typedef THashMap<TCateg, const char*, hash<TCateg> > TTempClassNamesHash;

    TTempClassNamesHash& GetHash() {
        return ClassNamesHash;
    }

private:
    TTempClassNamesHash ClassNamesHash;

    TMemoryPool Pool;
};

inline static void AddToHashAndPool(TClassNamesHashAndPool* hash, const char* s1, size_t l1, const char* s2, size_t l2) {
    TString s(s2, l2);
    hash->Add(FromString<TCateg>(s1, l1), s);
}

void TStaticClassNames::Load(const char* begin, const char* end, bool useRev) {
    {
        TClassNamesHashAndPool tempHash;
        ScanContainer(begin, end, &tempHash, AddToHashAndPool);

        SaveHashToBuffer(tempHash.GetHash(), ClassNamesSthashBuf);
    }

    ClassNamesSthashBuf.ShrinkToFit();

    C2NSthash = (const TClassNamesSthash*)(ClassNamesSthashBuf.data());

    if (useRev) {
        for (TClassNamesSthash::const_iterator it = C2NSthash->begin(); it != C2NSthash->end(); ++it) {
            N2CHash[it.Value()] = it.Key();
        }
    }
}

void TStaticClassNames::Save(TFixedBufferFileOutput& f) const {
    Y_VERIFY(C2NSthash, "C2NSthash is NULL");
    for (TClassNamesSthash::const_iterator it = C2NSthash->begin(); it != C2NSthash->end(); ++it) {
        f << it.Key() << "\t" << it.Value() << "\n";
    }
}

void TDynamicClassNames::Load(const char* begin, const char* end, bool use_rev) {
    // empiric formula for size calculation of grouping attributes
    // we approximately know how many records has in file with given size
    // plus 20% just in case
    const size_t c2nFileSize = 17 * 1024 * 1024;
    const size_t c2nRecords = 600 * 1024;
    size_t c2nSize = size_t((float)(end - begin) * c2nRecords / (c2nFileSize));
    c2nSize = size_t(c2nSize * 1.2);
    C2NMap.ClearAndResize(c2nSize);
    ScanContainer(begin, end, &C2NMap, AddToHash<TClassNames>);

    if (use_rev) {
        for (TClassNames::const_iterator it = C2NMap.begin(); it != C2NMap.end(); ++it) {
            N2CMap[it->second] = it->first;
        }
    }
}

void TDynamicClassNames::Save(TFixedBufferFileOutput& f) const {
    TMap<TClassNames::key_type, TClassNames::mapped_type> sorted(C2NMap.begin(), C2NMap.end());
    PrintContainer(f, sorted);
}

TCateg TDynamicClassNames::NewCateg(const TString& name) {
    TCateg c = CategCount() + 1;
    while (Categ2Name(c) != NONAME)
        ++c;
    C2NMap[(TSafeCateg)c] = name;
    N2CMap[name] = c;
    return c;
}

TCateg TDynamicClassNames::AddCateg(const TString& name) {
    TCateg c = Name2Categ(name);
    if (c != END_CATEG)
        return c;
    return NewCateg(name);
}

void TDynamicClassNames::AddCateg(const TString& name, TCateg categ, TClassNames& workStorage) {
    N2CMap[name] = categ;
    workStorage.Add((TSafeCateg)categ, name);
}


const char* const NONAME = "";

void ScanClassNames(const TString& filename, THashMap<TCateg, TString>& res) {
    TBlob blob = TBlob::FromFileContentSingleThreaded(filename);
    const char* begin = blob.AsCharPtr();
    const char* end = begin + blob.Length();
    ScanContainer(begin, end, &res, AddSimple< THashMap<TCateg, TString> >);
}

IMetaDb* TMetainfo::GetMetaDb(TDbName dbName) {
    switch (dbName) {
        case C2P:
            return dynamic_cast<TClassRelationsInfo*>(this);

        case C2L:
            return dynamic_cast<TC2LInfo*>(this);

        case C2N:
            return dynamic_cast<TClassNamesInfo*>(this);

        case C2Co:
            return dynamic_cast<TClassCoeffsInfo*>(this);

        default:
            ythrow yexception() << "Wrong dbName value: " <<  (long)dbName;
    }
}

void TMetainfo::Print(const char* filename, TDbName dbName) {
    TFixedBufferFileOutput f(filename);

    GetMetaDb(dbName)->Save(f);
}

void TMetainfo::ScanBlob(const TBlob& blob, TDbName dbName, bool useRev) {
    const char* begin = blob.AsCharPtr();
    const char* end = begin + blob.Length();

    GetMetaDb(dbName)->Load(begin, end, useRev);
}

void TMetainfo::Scan(const char* filename, TDbName dbName, bool use_rev) {
    if (filename != nullptr) {
        TBlob blob = TBlob::FromFileContentSingleThreaded(filename);
        ScanBlob(blob, dbName, use_rev);
    }
}

void TMetainfo::Scan(IInputStream& stream, TDbName dbName, bool use_rev) {
    TBlob blob = TBlob::FromStreamSingleThreaded(stream);
    ScanBlob(blob, dbName, use_rev);
}

void TMetainfo::ScanStroka(const TString& str, TDbName dbName, bool use_rev) {
    TBlob blob = TBlob::FromString(str);
    ScanBlob(blob, dbName, use_rev);
}

void TMetainfo::LoadMap(TDbName db, bool userev, const char* filename) {
    if (NFs::Exists(filename) && (TFile(filename, RdOnly).GetLength() > 0)) {
        Scan(filename, db, userev);
    }
}

void TMetainfo::LoadC2N(const TString& filename, const TString& attrname) {
    LoadMap(TMetainfo::C2N, attrname == "d", (filename + ".c2n").data());
}

void TMetainfo::LoadC2NOffroadWad(const TString& filename, const TString& attrname) {
    GetMetaDb(TMetainfo::C2N)->LoadFromPath(filename + ".c2n.wad", attrname == "d");
}

TCateg TMetainfo::CalcUpperBound() const {
    return Max(TClassNamesInfo::GetMaxKey(), TClassRelationsInfo::GetMaxKey());
}

bool HasValueInAttrPath(IIterator* attrs, const TMetainfo* pAttr, ui32 nAttr, TCateg value) {
    if (pAttr && attrs && (END_CATEG != value)) {
        attrs->MoveToAttr(nAttr);

        TCateg c(0);
        while (attrs->NextValue(&c)) {
            if (pAttr->HasValueInCategPath(value, c)) {
                return true;
            }
        }
    }

    return false;
}

}
