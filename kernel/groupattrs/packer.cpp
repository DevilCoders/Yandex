#include "packer.h"

#include "attrname.h"
#include "mutdocattrs.h"

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

namespace NGroupingAttrs {

struct TDoc2Categ {
    ui32 DocId;
    TCateg Categ;

    TDoc2Categ()
        : DocId(Max<ui32>())
        , Categ(Max<TCateg>())
    {
    }

    void Reset(const TString& str) {
        size_t tab = str.rfind('\t');

        Y_ASSERT(tab != str.npos);

        DocId = FromString<ui32>(str.data(), tab);
        Categ = FromString<TCateg>(str.data() + tab + 1, str.size() - tab - 1);
    }
};

struct TInput {
    TString Name;
    TDoc2Categ Current;

    TSimpleSharedPtr<IInputStream> File;
    TString LastRead;

    TInput(const TString& name)
        : Name(name)
    {
    }

    bool Next() {
        bool hasNext = File->ReadLine(LastRead);

        if (hasNext) {
            Current.Reset(LastRead);
        }

        return hasNext;
    }
};

typedef TVector<TInput> TInputs;

struct TInputComparator {
    bool operator() (const TInput* one, const TInput* another) const {
        Y_ASSERT(one && another);
        return one->Current.DocId > another->Current.DocId;
    }
};

TString Filename(const TString& str) {
    size_t semicolon = str.rfind(':');
    if (semicolon == TString::npos) {
        return str;
    }

    return TString(str.data(), 0, semicolon);
}

static void OpenInputs(const TString& config, TInputs* result) {
    result->clear();

    TFileInput input(config);
    TString line;
    while (input.ReadLine(line)) {
        TString filename = Filename(line);
        result->push_back(TInput(Attrname(filename)));
        result->back().File.Reset(new TFileInput(filename, 1 << 20));
    }
}


void TPacker::CalcMaxValue(const TString& filename) {
    if (!NFs::Exists(filename)) {
        ythrow yexception() << "File " << filename << " doesn't exist";
    }

    TFileInput input(filename, 1 << 20);
    TString line;
    TCateg max = 0;
    while (input.ReadLine(line)) {
        size_t tab = line.rfind('\t');

        Y_ASSERT(tab != line.npos);

        TCateg value = FromString<TCateg>(line.data() + tab + 1, line.size() - tab - 1);
        max = (max < value ? value : max);
    }

    MaxValues[Attrname(filename)] = max;
}

void TPacker::CalcMaxValues(const TString& config) {
    TFileInput input(config);
    TString line;
    while (input.ReadLine(line)) {
        CalcMaxValue(Filename(line));
    }
}

void TPacker::InitConfig() {
    for (TMaxValues::const_iterator it = MaxValues.begin(); it != MaxValues.end(); ++it) {
        Config.AddAttr(it->first.data(), Config.GetType(it->second));
    }
}

void TPacker::Merge(const TString& config) {
    TInputs inputs;
    OpenInputs(config, &inputs);

    TVector<TInput*> pointers;
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i].Next()) {
            pointers.push_back(&inputs[i]);
        }
    }

    MakeHeap(pointers.begin(), pointers.end(), TInputComparator());

    ui32 last = 0;
    TMutableDocAttrs data(Config);
    while (!pointers.empty()) {
        TInput* input = pointers.front();

        PopHeap(pointers.begin(), pointers.end(), TInputComparator());
        pointers.pop_back();

        ui32 docid = input->Current.DocId;

        if (docid != last) {
            Writer->Write(data);
            data.Clear();

            for (ui32 placeholder = last + 1; placeholder < docid; ++placeholder) {
                Writer->WriteEmpty();
            }

            last = docid;
        }

        data.SetAttr(input->Name.data(), input->Current.Categ);

        if (input->Next()) {
            pointers.push_back(input);
            PushHeap(pointers.begin(), pointers.end(), TInputComparator());
        }
    }

    if (!data.IsEmpty()) {
        Writer->Write(data);
    }
}

void TPacker::Pack(const TString& config, TVersion version, const TString& output) {
    TFsPath tmpdir = GetDirName(output) != output ? GetDirName(output) : ".";

    PackToStream(config, version, output, tmpdir);

}

void TPacker::PackToStream(const TString& config, TVersion version, const TString& output, const TFsPath& tmpdir) {
    CalcMaxValues(config);
    InitConfig();

    if (version == IDocsAttrsWriter::WadVersion) {
        Writer.Reset(new TDocsAttrsWadWriter(Config, output));
    } else {
        Writer.Reset(new TDocsAttrsWriter(Config, version, TConfig::Search, tmpdir, output, nullptr));
    }

    Merge(config);
    Writer->Close();
}

}
