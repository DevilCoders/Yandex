#pragma once

#include "config.h"
#include "docsattrsdata.h"
#include "metainfos.h"

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/system/sys_alloc.h>

class TMemoryCategPair {
private:
    ui32 AttrNum;
    TCateg Categ;
public:
    TMemoryCategPair(ui32 attrNum = NGroupingAttrs::TConfig::NotFound, TCateg categ = END_CATEG)
        : AttrNum(attrNum)
        , Categ(categ)
    { }
    friend bool operator< (const TMemoryCategPair& p1, const TMemoryCategPair& p2) noexcept {
        return p1.AttrNum < p2.AttrNum || p1.AttrNum == p2.AttrNum && p1.Categ < p2.Categ;
    }
    ui32 GetAttrNum() const {
        return AttrNum;
    }
    TCateg GetCateg() const {
        return Categ;
    }
};

namespace NMemoryAttrMap {

class TDeallocate {
public:
    template <class T>
    static inline void Destroy(T* t) noexcept {
        y_deallocate(t);
    }
};

class TOneDocAttrs {
public:
    typedef TSimpleSharedPtr<void, TDeallocate> TDataPtr;

private:
    TDataPtr Data;

    class TReadCateg {
    private:
        const char*& Data;
        TCateg* Categ;
    public:
        TReadCateg(const char*& data, TCateg* categ)
            : Data(data)
            , Categ(categ)
        { }
        template<typename T>
        void Process() {
            *Categ = *reinterpret_cast<const T*>(Data);
            Data += sizeof(T);
        }
    };

    class TReadCategs {
    private:
        const char* Begin;
        const char* End;
        TCategSeries* Categs;
    public:
        TReadCategs(const char* begin, const char* end, TCategSeries* categs)
            : Begin(begin)
            , End(end)
            , Categs(categs)
        { }
        template<typename T>
        void Process() {
            const T* begin = reinterpret_cast<const T*>(Begin);
            const T* end = reinterpret_cast<const T*>(End);
            while (begin < end)
                Categs->AddCateg(static_cast<TCateg>(*begin++));
        }
    };

    template<typename TWorker>
    static void SwitchType(NGroupingAttrs::TConfig::Type type, TWorker worker) {
        switch (type) {
            case NGroupingAttrs::TConfig::I32:
                worker.template Process<i32>();
                return;
            case NGroupingAttrs::TConfig::I16:
                worker.template Process<i16>();
                return;
            case NGroupingAttrs::TConfig::I64:
            default:
                worker.template Process<i64>();
                return;
        }
    }

    ui32 GetAttrCount() const {
        const ui32* offset = (const ui32*)Data.Get();
        if (offset)
            return *offset / sizeof(ui32) - 1;
        return 0;
    }

    const char* GetBegin(ui32 attr) const {
        const ui32* offset = (const ui32*)Data.Get();
        if (offset) {
            return (char*)offset + offset[Min<ui32>(attr, GetAttrCount())];
        }
        return nullptr;
    }

public:
    TOneDocAttrs() {
    }
    TOneDocAttrs(const NGroupingAttrs::TConfig& config, const TVector<TMemoryCategPair>& docAttrs) {
        Reset(config, docAttrs);
    }
    void Clear() {
        Data = nullptr;
    }
    // Assuming that docAttrs are sorted.
    TDataPtr Reset(const NGroupingAttrs::TConfig& config, const TVector<TMemoryCategPair>& docAttrs, bool update = false);
    void SetBeginEnd(ui32 attr, const char*& begin, const char*& end) const {
        begin = GetBegin(attr);
        end = GetBegin(attr + 1);
    }
    static void NextCateg(NGroupingAttrs::TConfig::Type type, const char*& begin, TCateg* categ) {
        SwitchType(type, TReadCateg(begin, categ));
    }
    static void GetCateg(NGroupingAttrs::TConfig::Type type, const char* begin, const char* /*end*/, TCateg* categ) {
        NextCateg(type, begin, categ);
    }
    static void GetCategs(NGroupingAttrs::TConfig::Type type, const char* begin, const char* end, TCategSeries* categs) {
        SwitchType(type, TReadCategs(begin, end, categs));
    }
};

class TMemoryConfigWrapper : public NGroupingAttrs::TMetainfos {
protected:
    NGroupingAttrs::TConfig AttrsConfig;

    ui32 AddAttrIfAbsent(const char* name, NGroupingAttrs::TConfig::Type type);
public:
    TMemoryConfigWrapper();

    const NGroupingAttrs::TConfig& Config() const {
        return AttrsConfig;
    }
    NGroupingAttrs::TMetainfo& AddMetainfo(const char* name, ui32& attrNum, NGroupingAttrs::TConfig::Type type = NGroupingAttrs::TConfig::I64) {
        attrNum = AddAttrIfAbsent(name, type);
        return ProvideMetainfo(attrNum);
    }

    TMemoryCategPair GetDocAttrCategPair(ui32 attrNum, const TString& value) {
        NGroupingAttrs::TMetainfo& metaInfo = ProvideMetainfo(attrNum);
        TCateg categ = NGroupingAttrs::GetDynamicClassNames(metaInfo)->AddCateg(value);
        return TMemoryCategPair(attrNum, categ);
    }
    TMemoryCategPair GetDocAttrNameCategPair(const char* attrName, const TString& value, NGroupingAttrs::TConfig::Type type) {
        ui32 attrNum = AddAttrIfAbsent(attrName, type);
        return GetDocAttrCategPair(attrNum, value);
    }
    TMemoryCategPair GetDocAttrIntCategPair(const char* attrName, TCateg categ, NGroupingAttrs::TConfig::Type type) {
        ui32 attrNum = NGroupingAttrs::TConfig::NotFound;
        AddMetainfo(attrName, attrNum, type);
        return TMemoryCategPair(attrNum, categ);
    }
};

} // namespace NMemoryAttrMap

class TMemoryAttrMap : public NMemoryAttrMap::TMemoryConfigWrapper
                     , public NGroupingAttrs::IDocsAttrsData {
private:
    typedef NMemoryAttrMap::TOneDocAttrs TOneDocAttrs;
    typedef TVector<TOneDocAttrs> TDocAttrs; // DocId -> (AttrNum -> Categs)

    TDocAttrs DocAttrs;

    const TString& ConfigDir;

    ui32 DocCount_;

    template<class TCategFunc, class TCategs>
    bool GetDocCategCommon(ui32 docId, ui32 attrNum, TCategFunc method, TCategs* categs) const {
        Y_VERIFY(docId < DocAttrs.size(), "Docid is out of range");
        if (attrNum == NGroupingAttrs::TConfig::NotFound) {
            return false;
        }
        const TOneDocAttrs& doc = DocAttrs[docId];
        const char* begin;
        const char* end;
        doc.SetBeginEnd(attrNum, begin, end);
        if (begin == end) {
            return false;
        }
        method(AttrsConfig.AttrType(attrNum), begin, end, categs);
        return true;
    }
public:
    class TIterator : private NGroupingAttrs::IIterator {
    private:
        const TMemoryAttrMap* Owner;
        const TOneDocAttrs* Attrs;

        const char* Begin;
        const char* End;

        NGroupingAttrs::TConfig::Type Type;

    public:
        TIterator(const TMemoryAttrMap* attrmap, ui32 docid)
            : Owner(attrmap)
            , Attrs(nullptr)
            , Begin(nullptr)
            , End(nullptr)
            , Type(NGroupingAttrs::TConfig::BAD)
        {
            if (attrmap) {
                Y_ASSERT(docid < Owner->DocAttrs.size());
                Attrs = &Owner->DocAttrs[docid];
            }
        }

        void MoveToAttr(const char* attrName) override {
            Y_ASSERT(Owner);
            MoveToAttr(Owner->Config().AttrNum(attrName));
        }

        void MoveToAttr(ui32 attrNum) override {
            Y_ASSERT(Owner);
            Y_ASSERT(attrNum != NGroupingAttrs::TConfig::NotFound && attrNum < Owner->Config().AttrCount());
            Attrs->SetBeginEnd(attrNum, Begin, End);
            Type = Owner->Config().AttrType(attrNum);
        }

        bool NextValue(TCateg* result) override {
            if (Begin >= End)
                return false;

            TOneDocAttrs::NextCateg(Type, Begin, result);
            return true;
        }

        friend class TMemoryAttrMap;
    };

    TMemoryAttrMap(const TString& configDir, size_t size);

    // Read only interface
    inline ui32 GetDocCount() const {
        return DocCount_;
    }
    bool GetDocCateg(ui32 docId, ui32 attrNum, TCateg* result) const {
        return GetDocCategCommon(docId, attrNum, &TOneDocAttrs::GetCateg, result);
    }
    inline bool GetDocCategs(ui32 docId, ui32 attrNum, TCategSeries* result) const {
        result->Clear();
        return GetDocCategCommon(docId, attrNum, &TOneDocAttrs::GetCategs, result);
    }

    // Write interface
    void SetDocCount(ui32 docCount) {
        DocCount_ = docCount;
    }
    void LoadC2P(const TString& attrList);

    // update == true means that only setted attributes will be changed. In other case all attributes
    // will be cleared and set new values. In update case END_CATEG value in TMemoryCategPair::Categ
    // means that all value of attribute must be erased.
    TOneDocAttrs::TDataPtr SetAttrs(ui32 docId, TVector<TMemoryCategPair>& docAttrs, bool update = false) {
        Y_VERIFY(docId < DocAttrs.size(), "oops");
        TOneDocAttrs& oneDocAttrs = DocAttrs[docId];
        Sort(docAttrs.begin(), docAttrs.end());
        TOneDocAttrs::TDataPtr result = oneDocAttrs.Reset(AttrsConfig, docAttrs, update);
        docAttrs.clear();
        return result;
    }
    void Clear(ui32 docId) {
        Y_VERIFY(docId < DocAttrs.size(), "oops");
        DocAttrs[docId].Clear();
    }

    // IDocsAttrsData implementation
    ui32 DocCount() const override;
    bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* externalRelevance) const override;
    void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const override;
    NGroupingAttrs::TVersion Version() const override;
    NGroupingAttrs::TFormat Format() const override;
    const NGroupingAttrs::TConfig& Config() const override;
    const NGroupingAttrs::TMetainfos& Metainfos() const override;
    NGroupingAttrs::TConfig& MutableConfig() override;
    NGroupingAttrs::TMetainfos& MutableMetainfos() override;
    TAutoPtr<NGroupingAttrs::IIterator> CreateIterator(ui32 docId) const override;
    const void* GetRawDocumentData(ui32 docid) const override;
    bool SetDocCateg(ui32 docid, ui32 attrnum, ui64 value) override;
};
