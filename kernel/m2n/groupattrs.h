#pragma once

#include <util/folder/dirut.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <kernel/groupattrs/docsattrs.h>
#include <kernel/groupattrs/docsattrswriter.h>
#include <kernel/groupattrs/mutdocattrs.h>

#include "remaptable.h"

namespace NM2N {

typedef ui16 TClusterId; // Should be the same type as TFromToRec::FromCl/ToCl.

class IOutputStreamsBuilder {
public:
    virtual IOutputStream* BuildStream(const TString& filename) = 0;
};

class TAttrValues {
public:
    struct TValue {
        ui32 DocId;
        TCateg Value;

        inline TValue(ui32 docId, TCateg value)
            : DocId(docId)
            , Value(value)
        {
        }
    };

    typedef TVector<TValue> TValues;

private:
    TValues Values;

    static bool ValueLessByDocId(const TValue& v1, const TValue& v2) {
        return v1.DocId < v2.DocId;
    }

public:
    inline void AddValue(ui32 docId, TCateg value) {
        Values.push_back(TValue(docId, value));
    }

    TCateg GetMaxValue() const noexcept {
        TCateg max = 0;
        for (TVector<TValue>::const_iterator it = Values.begin(); it != Values.end(); ++it) {
            if (it->Value > max) {
                max = it->Value;
            }
        }

        return max;
    }

    void Sort() {
        std::sort(Values.begin(), Values.end(), ValueLessByDocId);
    }

    const TValues& GetValues() const noexcept {
        return Values;
    }
};

class TValuesIterator {
private:
    TAttrValues::TValues::const_iterator It;
    TAttrValues::TValues::const_iterator End;

public:
    TValuesIterator(const TAttrValues* values)
        : It(values->GetValues().begin())
        , End(values->GetValues().end())
    {}

    inline bool IsValid() const {
        return It != End;
    }

    inline ui32 GetCurrDocId() noexcept {
        return It->DocId;
    }

    inline TCateg GetDocAttr() noexcept {
        return It->Value;
    }

    bool Next() {
        if (!IsValid())
            return false;
        ++It;
        return IsValid();
    }
};
class TAttrNames {
    typedef THashMap<const char*, TCateg> TNamesHash;
    TNamesHash Names;

public:
    inline TCateg AddName(const char* name) { //name is stored by corresponding NGroupingAttrs::TMetainfo
        TNamesHash::const_iterator it = Names.find(name);
        if (it != Names.end()) {
            return it->second;
        }

        // min "h" attr values is 1, not 0
        TCateg newValue = Names.size() + 1;
        Names.insert(TNamesHash::value_type(name, newValue));

        return newValue;
    }

    void WriteC2N(IOutputStream& out) {
        THashMap<TCateg, const char*> c2n;
        for (TNamesHash::const_iterator it = Names.begin(); it != Names.end(); ++it) {
            c2n.insert(THashMap<TCateg, const char*>::value_type(it->second, it->first));
        }

        for (TCateg i = 1; i <= static_cast<TCateg>(c2n.size()); ++i) {
            out << i << "\t" << c2n[i] << "\n";
        }
    }
};

class TAttrFiles {
private:
    TString Prefix;

public:
    TAttrFiles(const TString& prefix)
        : Prefix(prefix)
    {
    }

    inline TString GetAttrsFile() const {
        if (Prefix.EndsWith(NGroupingAttrs::TDocsAttrs::GetFilenameSuffix()))
            return Prefix.substr(0, Prefix.size() - NGroupingAttrs::TDocsAttrs::GetFilenameSuffix().size());
        return Sprintf("%s/index", Prefix.data());
    }

    inline TString GetResultFile() const {
        if (Prefix.EndsWith(NGroupingAttrs::TDocsAttrs::GetFilenameSuffix()))
            return Prefix;
        return Sprintf("%s/index%s", Prefix.data(), NGroupingAttrs::TDocsAttrs::GetFilenameSuffix().data());
    }

    inline TString GetC2NFile(const TString& attr) const {
        return Sprintf("%s/%s.c2n", Prefix.data(), attr.data());
    }
};

enum EAttrType {
    AT_GLOBAL,
    AT_NAMES
};

struct TAttr {
public:
    TString Name;
    EAttrType Type;
    bool Unique;
    NGroupingAttrs::TConfig::Type OriginalType;

    TAttr()
        : Type(AT_GLOBAL)
    {
    }

    TAttr(TString name, EAttrType type, bool unique, NGroupingAttrs::TConfig::Type originalType)
        : Name(name)
        , Type(type)
        , Unique(unique)
        , OriginalType(originalType)
    {
    }
};

typedef TVector<TAttr> TAttrs;
typedef TVector<TAttrValues> TAttrsValues;
typedef TVector< TSimpleSharedPtr<NGroupingAttrs::TMetainfo> > TAttrsNames;
typedef TVector<TAttrNames> TDstAttrsNames;

class TAttrsRemap {
private:
    const TCompactMultipleRemapTable* RemapTable;

    TVector<TVector<TVector<size_t> > > AttrsRemap; // AttrsRemap[fromCl][fromAttrIdx][toCl] = toAttrIdx;
    static const size_t NoRemap;

    TVector<TAttrs> SrcAttrs;
    TVector<TAttrFiles> SrcFiles;
    TVector<TAttrsNames> SrcNames;

    TVector<TAttrs> DstAttrs;
    TVector<TAttrFiles> DstFiles;
    TVector<TAttrsValues> DstValues;
    TVector<TDstAttrsNames> DstNames;

    bool IgnoreNoAttrs;
    bool RemapEmptyAttrs;
    NGroupingAttrs::TVersion WriteVersion;

    IOutputStreamsBuilder* StreamsBuilder;

    void ReadAttrs(TAttrFiles attrFiles, TAttrs* result) {
        TAttrs res;

        NGroupingAttrs::TDocsAttrs da(false, attrFiles.GetAttrsFile().data(), false, true, IgnoreNoAttrs);
        res.reserve(da.Config().AttrCount());

        for (size_t attrnum = 0; attrnum < da.Config().AttrCount(); ++attrnum) {
            const TString name(da.Config().AttrName(attrnum));
            EAttrType type = AT_GLOBAL;
            if (NFs::Exists(attrFiles.GetC2NFile(name))) {
                type = AT_NAMES;
            }
            bool unique = da.Config().IsAttrUnique(attrnum);
            res.push_back(TAttr(name, type, unique, da.Config().AttrType(attrnum)));
        }

        result->swap(res);
    }

    inline void AddResultCategGlobal(TClusterId oldCl, size_t oldAttrIndex, TClusterId newCl, ui32 newDoc, TCateg value) {
        size_t newAttrIndex = GetNewAttrIndex(oldCl, oldAttrIndex, newCl);
        DstValues[newCl][newAttrIndex].AddValue(newDoc, value);
    }

    inline void AddResultCategName(TClusterId oldCl, size_t oldAttrIndex, TClusterId newCl, ui32 newDoc, const char* value) {
        size_t newAttrIndex = GetNewAttrIndex(oldCl, oldAttrIndex, newCl);
        TCateg categ = DstNames[newCl][newAttrIndex].AddName(value);
        DstValues[newCl][newAttrIndex].AddValue(newDoc, categ);
    }

    inline size_t GetNewAttrIndex(TClusterId oldCl, size_t oldAttrIndex, TClusterId newCl) noexcept {
        size_t& newAttrIndex = AttrsRemap[oldCl][oldAttrIndex][newCl];
        if (newAttrIndex != NoRemap) {
            return newAttrIndex;
        }

        const TString attrName = SrcAttrs[oldCl][oldAttrIndex].Name;

        const size_t dstAttrsCounts = DstAttrs[newCl].size();
        for (size_t i = 0; i < dstAttrsCounts; ++i) {
            if (DstAttrs[newCl][i].Name == attrName) {
                newAttrIndex = i;
                return newAttrIndex;
            }
        }

        newAttrIndex = dstAttrsCounts;
        DstValues[newCl].resize(newAttrIndex + 1);
        DstNames[newCl].resize(newAttrIndex + 1);
        DstAttrs[newCl].resize(newAttrIndex + 1);
        DstAttrs[newCl][newAttrIndex] = SrcAttrs[oldCl][oldAttrIndex];
        return newAttrIndex;
    }

    void ReadConfig(const char** inputFiles, const char** outputFiles) {
        TClusterId srcClsNum = static_cast<TClusterId>(RemapTable->SrcClustersCount());
        TClusterId dstClsNum = static_cast<TClusterId>(RemapTable->DstClustersCount());

        SrcAttrs.resize(srcClsNum);
        SrcNames.resize(srcClsNum);
        AttrsRemap.resize(srcClsNum);

        for (TClusterId i = 0; i < srcClsNum; ++i) {
            TAttrFiles attrFiles(inputFiles[i]);

            ReadAttrs(attrFiles, &SrcAttrs[i]);
            const TAttrs& attrs = SrcAttrs[i];

            SrcFiles.push_back(attrFiles);

            AttrsRemap[i].resize(attrs.size());
            SrcNames[i].resize(attrs.size());

            for (size_t j = 0; j < attrs.size(); ++j) {
                AttrsRemap[i][j].resize(dstClsNum, NoRemap);

                if (attrs[j].Type == AT_NAMES) {
                    SrcNames[i][j].Reset(new NGroupingAttrs::TMetainfo(false));
                    SrcNames[i][j]->Scan(attrFiles.GetC2NFile(attrs[j].Name).data(), NGroupingAttrs::TMetainfo::C2N);
                }
            }
        }

        DstValues.resize(dstClsNum);
        DstNames.resize(dstClsNum);
        DstAttrs.resize(dstClsNum);

        for (TClusterId i = 0; i < dstClsNum; ++i) {
            DstFiles.push_back(TAttrFiles(outputFiles[i]));
        }
    }

    void Validate() {
        THashMap<TString, EAttrType> types;

        for (TVector<TAttrs>::const_iterator itAttrs = SrcAttrs.begin(); itAttrs != SrcAttrs.end(); ++itAttrs) {
            for (TAttrs::const_iterator itAttr = itAttrs->begin(); itAttr != itAttrs->end(); ++itAttr) {
                THashMap<TString, EAttrType>::const_iterator it = types.find(itAttr->Name);

                if (it == types.end()) {
                    types[itAttr->Name] = itAttr->Type;
                } else {
                    if (it->second != itAttr->Type) {
                        ythrow yexception() << "Attribute " <<  itAttr->Name.data() << " has different types";
                    }
                }
            }
        }
    }

    void WriteAttrs(NGroupingAttrs::TConfig config, THashMap<TString, TValuesIterator>& its, ui32 dstMaxDocId, const TString& filename) {
        Y_ASSERT(config.AttrCount() == its.size());

        THolder<IOutputStream> out;
        if (StreamsBuilder) {
            out.Reset(StreamsBuilder->BuildStream(filename));
        } else {
            out.Reset(new TFixedBufferFileOutput(filename));
        }

        NGroupingAttrs::TDocsAttrsWriter writer(config, WriteVersion, NGroupingAttrs::TConfig::Search, GetDirName(filename), *out, "");

        ui32 docid = 0;
        for (bool hasNotEndedIters = true;hasNotEndedIters; ++docid) {
            bool isEmpty = true;
            hasNotEndedIters = false;
            NGroupingAttrs::TMutableDocAttrs da(config);
            for (size_t attrnum = 0; attrnum < config.AttrCount(); ++attrnum) {
                TValuesIterator& iter = its.find(config.AttrName(attrnum))->second;
                for (; iter.IsValid() && (iter.GetCurrDocId() <= docid); iter.Next()) {
                    if (iter.GetCurrDocId() == docid) {
                        da.SetAttr(attrnum, iter.GetDocAttr());
                        isEmpty = false;
                    }
                }
                hasNotEndedIters |= iter.IsValid();
            }

            if (!isEmpty) {
                writer.Write(da);
            } else {
                writer.WriteEmpty();
            }
        }

        while (docid <= dstMaxDocId) {
            writer.WriteEmpty();
            ++docid;
        }

        writer.Close();
    }

public:
    //! @note numbers of input/output files are defined by numbers of input/output clusters of remapTable
    TAttrsRemap(const char** inputFiles, const char** outputFiles, const TCompactMultipleRemapTable* remapTable, bool ignoreNoAttrs = false,
                NGroupingAttrs::TVersion writeVersion = NGroupingAttrs::TDocsAttrsWriter::DefaultVersion, bool remapEmptyAttrs = false)
        : RemapTable(remapTable)
        , IgnoreNoAttrs(ignoreNoAttrs)
        , RemapEmptyAttrs(remapEmptyAttrs)
        , WriteVersion(writeVersion)
        , StreamsBuilder(nullptr)
    {
        ReadConfig(inputFiles, outputFiles);
        Validate();
    }

    void SetOutputStreamsBuilder(IOutputStreamsBuilder* builder) {
        StreamsBuilder = builder;
    }

    void Remap() {
        TVector<NM2N::TDocCl> dsts;

        for (TClusterId cl = 0; cl < RemapTable->SrcClustersCount(); ++cl) {
            NGroupingAttrs::TDocsAttrs da(false, SrcFiles[cl].GetAttrsFile().data(), false, true, IgnoreNoAttrs);

            if (RemapEmptyAttrs) {
                for (size_t attrnum = 0; attrnum < da.Config().AttrCount(); ++attrnum) {
                    for (TClusterId destCl = 0; destCl < RemapTable->DstClustersCount(); ++destCl) {
                        GetNewAttrIndex(cl, attrnum, destCl);
                    }
                }
            }

            size_t numDocs = RemapTable->GetSrcMaxDocId(cl) + 1;

            for (size_t doc = 0; doc < numDocs; ++doc) {
                if (!RemapTable->GetMultipleDst(doc, static_cast<ui16>(cl), &dsts))
                    continue;

                for (size_t i = 0; i < dsts.size(); ++i) {
                    for (size_t attrnum = 0; attrnum < da.Config().AttrCount(); ++attrnum) {
                        TCategSeries series;
                        da.DocCategs(doc, attrnum, series);

                        EAttrType type = SrcAttrs[cl][attrnum].Type;

                        if (type == AT_GLOBAL) {
                            for (size_t c = 0; c < series.size(); ++c) {
                                AddResultCategGlobal(cl, attrnum, dsts[i].ClId, dsts[i].DocId, series.GetCateg(c));
                            }

                        } else if (type == AT_NAMES) {
                            for (size_t c = 0; c < series.size(); ++c) {
                                const char* name = SrcNames[cl][attrnum]->Categ2Name(series.GetCateg(c));
                                AddResultCategName(cl, attrnum, dsts[i].ClId, dsts[i].DocId, name);
                            }
                        }
                    }
                }
            }
        }
    }

    void WriteResult() {
        TClusterId numDst = static_cast<size_t>(RemapTable->DstClustersCount());

        for (TClusterId cl = 0; cl < numDst; ++cl) {
            NGroupingAttrs::TConfig result;
            for (size_t i = 0; i < DstAttrs[cl].size(); ++i) {
                const TCateg maxValue = DstValues[cl][i].GetMaxValue();
                bool unique = DstAttrs[cl][i].Unique;
                const NGroupingAttrs::TConfig::Type type = unique ? DstAttrs[cl][i].OriginalType : NGroupingAttrs::TConfig::GetType(maxValue);
                result.AddAttr(DstAttrs[cl][i].Name.data(), type, false, unique);
            }

            THashMap<TString, TValuesIterator> its;
            for (size_t i = 0; i < DstValues[cl].size(); ++i) {
                TAttrValues& av = DstValues[cl][i];
                av.Sort();
                its.insert(std::pair<TString, TValuesIterator>(DstAttrs[cl][i].Name, TValuesIterator(&av)));
            }

            if (!result.AttrCount())
                continue; // don't write results to file since there are no any documents with grouping attributes

            WriteAttrs(result, its, RemapTable->GetDstMaxDocId(cl), DstFiles[cl].GetResultFile());

            for (size_t i = 0; i < DstAttrs[cl].size(); ++i) {
                if (DstAttrs[cl][i].Type == AT_NAMES) {
                    TFixedBufferFileOutput c2nfile(DstFiles[cl].GetC2NFile(DstAttrs[cl][i].Name));
                    DstNames[cl][i].WriteC2N(c2nfile);
                }
            }
        }
    }
};

static inline bool IsEqual(const THashSet<TString>& s1, const THashSet<TString>& s2) {
    if (s1.size() != s2.size()) {
        return false;
    }

    for (THashSet<TString>::const_iterator it = s1.begin(); it != s1.end(); ++it) {
        if (!s2.contains(*it)) {
            return false;
        }
    }

    return true;
}

} // namespace NM2N
