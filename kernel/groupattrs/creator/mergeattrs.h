#pragma once

#include <kernel/groupattrs/docattrs.h>
#include <kernel/groupattrs/docsattrs.h>
#include <kernel/groupattrs/docsattrswriter.h>

#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/defaults.h>

#include <library/cpp/deprecated/mbitmap/mbitmap.h>

namespace NGroupingAttrs {

class TInputAttrPortion {
private:
    TSimpleSharedPtr<TDocsAttrs> DocsAttrs;

    ui32 Index;
    TSimpleSharedPtr<TDocAttrs> Data;

public:
    TInputAttrPortion()
        : Index(0)
    {
    }

    void Init(const TString& filename);

    const TConfig& Config() const {
        return DocsAttrs->Config();
    }

    bool Next();
    ui32 DocId() const;
    const TDocAttrs& Current() const;
};

typedef TVector<TInputAttrPortion> TInputPortions;

class TOutputAttrPortion {
private:
    THolder<TFixedBufferFileOutput> Out;
    THolder<TDocsAttrsWriter> Writer;

public:
    TConfig Config;

public:
    TOutputAttrPortion(TConfig::Mode mode)
        : Config(mode)
    {
    }

    bool Init(const TString& filename, const TInputPortions& input, const char* prefix, NGroupingAttrs::TVersion grattrVersion);
    void Write(TMutableDocAttrs& docAttrs);
    void WriteEmpty();
    void Close();
};

void MergeAttrsPortion(const TVector<TString>& newNames, const TString& oldName, const TString& outName, const bitmap_2* addDelDocuments, TConfig::Mode mode, NGroupingAttrs::TVersion grattrVersion = TDocsAttrsWriter::DefaultVersion);

}
