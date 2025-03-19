#include "mergeattrs.h"

#include <kernel/groupattrs/mutdocattrs.h>

#include <ysite/yandex/common/mergemode.h>

#include <util/folder/dirut.h>

namespace NGroupingAttrs {

/* TInputAttrPortion */

static TString InputPortionFilename(const TString& filename) {
    if (filename.EndsWith(TDocsAttrs::GetFilenameSuffix())) {
        return TString(filename.data(), filename.size() - TDocsAttrs::GetFilenameSuffix().size());
    }

    return filename;
}

void TInputAttrPortion::Init(const TString& filename) {
    DocsAttrs.Reset(new TDocsAttrs(false, InputPortionFilename(filename).data()));
    if (DocsAttrs->Format() != 0) {
        ythrow yexception() << "Invalid portion format";
    }

    //Config_.Reset(new TConfig(&DocsAttrs->Config()));
    Data.Reset(new TDocAttrs(Config()));
}

bool TInputAttrPortion::Next() {
    if (Index >= DocsAttrs->DocCount()) {
        return false;
    }

    DocsAttrs->DocAttrs(Index, Data.Get());

    ++Index;
    return true;
}

ui32 TInputAttrPortion::DocId() const {
    const TCategSeries& categs = Data->Attrs(DocsAttrs->Config().IndexAuxAttrName());
    if (categs.size() == 0) {
        ythrow yexception() << "Can't get docid, invalid config mode";
    }

    return static_cast<ui32>(categs.GetCateg(0));
}

const TDocAttrs& TInputAttrPortion::Current() const {
    return *Data.Get();
}

struct TInputPortionComparator {
    bool operator() (TInputAttrPortion* one, TInputAttrPortion* another) {
        return one->DocId() > another->DocId();
    }
};

/* TOutputAttrPortion */
bool TOutputAttrPortion::Init(const TString& filename, const TInputPortions& inputs, const char* prefix, NGroupingAttrs::TVersion grattrVersion) {
    for (size_t i = 0; i < inputs.size(); ++i) {
        Config.MergeConfig(inputs[i].Config());
    }

    if (Config.AttrCount() == 0) {
        return false;
    }

    Out.Reset(new TFixedBufferFileOutput(filename));
    Writer.Reset(new TDocsAttrsWriter(Config, grattrVersion, Config.UsageMode(), GetDirName(filename), *Out.Get(), prefix));

    return true;
}

void TOutputAttrPortion::Write(TMutableDocAttrs& docAttrs) {
    Writer->Write(docAttrs);
}

void TOutputAttrPortion::WriteEmpty() {
    Writer->WriteEmpty();
}

void TOutputAttrPortion::Close() {
    Writer->Close();
}

/* Merge */
void MergeAttrsPortion(const TVector<TString>& newPortionsNames, const TString& oldPortionName, const TString& outName, const bitmap_2* addDelDocuments, TConfig::Mode mode, NGroupingAttrs::TVersion grattrVersion) {
    TInputPortions inputs;
    TOutputAttrPortion out(mode);

    bool hasOldPortion = !!oldPortionName && NFs::Exists((oldPortionName + TDocsAttrs::GetFilenameSuffix()));
    inputs.resize(newPortionsNames.size() + (hasOldPortion ? 1 : 0));

    if (hasOldPortion) {
        inputs[0].Init(oldPortionName + TDocsAttrs::GetFilenameSuffix());
    }

    for (size_t i = 0; i < newPortionsNames.size(); ++i) {
        inputs[i + (hasOldPortion ? 1 : 0)].Init(newPortionsNames[i]);
    }

    TString outNameCopy = outName; // we have to copy because GetFileNameComponent destroys its parameter
    const char* outFileName = GetFileNameComponent(outName.begin());
    if (!out.Init(outName, inputs, outFileName, grattrVersion)) {
        return;
    }

    TVector<TInputAttrPortion*> pointers;
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i].Next()) {
            pointers.push_back(&inputs[i]);
        }
    }

    ui32 last = static_cast<ui32>(-1);
    TMutableDocAttrs data(out.Config, 0);
    while (!pointers.empty()) {
        TInputAttrPortion* input = pointers.front();

        PopHeap(pointers.begin(), pointers.end(), TInputPortionComparator());
        pointers.pop_back();

        ui32 docid = input->DocId();
        if (docid != last) {
            if (!data.IsEmpty()) {
                out.Write(data);
            } else if (last != static_cast<ui32>(-1)) {
                out.WriteEmpty();
            }

            if (mode == TConfig::Search) {
                for (size_t i = 0; i < docid - last - 1; ++i) {
                    out.WriteEmpty();
                }
            }

            MERGE_MODE mm = addDelDocuments ? (MERGE_MODE)addDelDocuments->test(docid) : MM_DEFAULT;
            if (mm == MM_DELETE) {
                out.WriteEmpty();
                continue;
            }

            last = docid;
            data.Clear();
            if (mode == TConfig::Index) {
                data.ResetIndexAuxAttr(last);
            }
        }

        data.Merge(input->Current());
        if (input->Next()) {
            pointers.push_back(input);
            PushHeap(pointers.begin(), pointers.end(), TInputPortionComparator());
        }
    }

    out.Write(data);
    out.Close();
}

}
