#include <library/cpp/digest/lower_case/hash_ops.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/str.h>
#include "catxml.h"

static inline bool LCCompare(const TStringBuf& f, const TStringBuf& s) noexcept {
    return TCIOps()(f, s);
}

/// Copy src to dst possibly skipping xml declaration
static void TransferWithoutDeclaration(IInputStream* src, IOutputStream* dst) {
    TString marker = TString::Uninitialized(5);

    if (src->Load(marker.begin(), 5) != 5)
        ythrow yexception() << "xml is too short";
    // omit declaration
    if (LCCompare(marker, TStringBuf("<?xml")))
        src->ReadLine();
    else
        *dst << marker;
    TransferData(src, dst);
}

static void ExtractDeclaration(IInputStream* src, TString& declaration, TString& rest) {
    TString marker = TString::Uninitialized(5);

    if (src->Load(marker.begin(), 5) != 5)
        ythrow yexception() << "xml is too short";

    if (LCCompare(marker, TStringBuf("<?xml"))) {
        TString line = src->ReadLine();
        size_t pos = line.find("?>");

        if (pos == TString::npos) {
            ythrow yexception() << "invalid xml declaration";
        } else {
            pos += 2;
        }

        declaration = marker + line.substr(0, pos);
        marker = line.substr(pos);
    } else {
        declaration.clear();
    }

    rest = marker + src->ReadAll();
}

void ConcatenateXmlStreams(const char* rootName, const TVector<IInputStream*>& streams, TString& result) {
    Y_ASSERT(!streams.empty());

    TStringOutput out(result);
    TString marker = TString::Uninitialized(5);

    for (size_t i = 0; i < streams.size(); ++i) {
        IInputStream* stream = streams[i];

        if (i == 0) {
            TString decl, text;
            ExtractDeclaration(stream, decl, text);
            out << decl << '<' << rootName << '>' << text;
        } else {
            TransferWithoutDeclaration(stream, &out);
        }
    }

    out << "</" << rootName << ">" << Endl;
}

void ConcatenateXmlStreams(const char* rootName,
                           const TStreamVector& spvec,
                           TString& result) {
    TVector<IInputStream*> vec;
    for (const auto& i : spvec) {
        vec.push_back(i.Get());
    }
    ConcatenateXmlStreams(rootName, vec, result);
}

static void ConcatenateXmlStreamsImp(TXmlConcatTask& task, IOutputStream& out,
                                     IInputStream* first) {
    if (!task.Streams.empty() && task.Streams[0].Get() == first) {
        IInputStream* s = task.Streams[0].Get();
        TString decl, text;
        ExtractDeclaration(s, decl, text);
        out << decl << '<' << task.Element << '>' << text;
        task.Streams.erase(task.Streams.begin());
    } else {
        out << '<' << task.Element << '>';
    }

    for (const auto& stream : task.Streams) {
        TransferWithoutDeclaration(stream.Get(), &out);
    }

    for (auto& i : task.Children) {
        ConcatenateXmlStreamsImp(i, out, first);
    }
    out << "</" << task.Element << '>';
}

static IInputStream* FindFirstStream(const TXmlConcatTask& task) {
    if (task.Streams.size())
        return task.Streams[0].Get();
    else if (task.Children.size()) {
        for (size_t i = 0; i < task.Children.size(); ++i) {
            IInputStream* ret = FindFirstStream(task.Children[i]);
            if (ret)
                return ret;
        }
    }
    return nullptr;
}

void ConcatenateXmlStreams(const TXmlConcatTask& task, TString& result) {
    TStringOutput out(result);
    TXmlConcatTask copy = task;
    IInputStream* first = FindFirstStream(task);
    ConcatenateXmlStreamsImp(copy, out, first);
}
