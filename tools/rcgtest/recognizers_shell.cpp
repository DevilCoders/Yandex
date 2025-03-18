#include <util/folder/dirut.h>
#include <util/folder/iterator.h>
#include <util/generic/utility.h>
#include <util/stream/length.h>
#include <util/stream/output.h>
#include <util/string/vector.h>
#include <util/string/strip.h>

#include "recognizers_shell.h"
#include <util/string/split.h>

using namespace NRecognizer;

class TRecognizersShell::TResult {
public:
    ECharset Encoding;
    TRecognizer::TLanguages Languages;

public:
    TResult();
};

TRecognizersShell::TResult::TResult()
    : Encoding(CODES_UNKNOWN)
{}

// TRecognizersShell

TRecognizersShell::TRecognizersShell()
    : Recognizer(DictRec)
    , PrintResults(true)
    , UseParser(false)
    , UseUCS2(false)
    , AllLanguages(false)
    , LineByLine(false)
    , BufferSize(TRecognizerShell::DefaultBufSize)
{}

void TRecognizersShell::Load(const TString& path) {
    if (Recognizer == DictRec) {
        DictHolder.Reset(new TRecognizerShell(path, BufferSize));
    } else {
        ythrow yexception() << "recognizer is not specified";
    }
}

void TRecognizersShell::ShowInfo() const {
    if (DictHolder.Get()) {
        const TVector<ECharset>& encodings = DictHolder->GetRecognizer()->GetSupportedEncodings();
        const TVector<ELanguage>& languages = DictHolder->GetRecognizer()->GetSupportedLanguages();

        for (size_t i = 0; i < encodings.size(); ++i) {
            Cout << NameByCharset(encodings[i]) << "\t";
        }
        Cout << Endl;

        for (size_t i = 0; i < languages.size(); ++i) {
            Cout << NameByLanguage(languages[i]) << "\t";
        }
        Cout << Endl;

        Cout << DictHolder->GetRecognizer()->GetDictionary().EncodingsSize() << Endl;
    }
}

TRecognizerShell& TRecognizersShell::GetDictRec() const {
    if (DictHolder.Get() == nullptr) {
        ythrow yexception() << "recognizer is not initialized";
    }
    return *DictHolder;
}

void TRecognizersShell::ProcessFile(const TString& path, const THints& hints) const {
    if ((PrintResults || Singleton<TDumper>()->RequirePath()) && ! LineByLine) {
        Cout << path << "\t";
    }

    TFileInput file(path);
    Process(file, hints);
}

void TRecognizersShell::Process(IInputStream& input, const THints& hints) const {
    if (LineByLine) {
        ProcessEachLine(input);
    } else {
        TResult result;
        Recognize(input, result, hints);

        WriteResults(result);
    }
}

void TRecognizersShell::Recognize(IInputStream& input, TResult& result, const THints& hints) const {
    if (hints.Url == nullptr && !DefaultUrl.empty()) {
        THints temp(hints);
        temp.Url = DefaultUrl.c_str();
        Recognize(input, result, temp);
        return;
    }

    if (Recognizer != DictRec)
        ythrow yexception() << "recognizer is not specified";

    if (UseParser) {
        TBuffer buf;
        {
            TBufferOutput out(buf);
            TransferData(&input, &out);
        }
        GetDictRec().RecognizeHtml(buf.Data(), buf.Size(), result.Encoding, result.Languages, hints);
    } else if (UseUCS2) {
        // Cout << "reading ucs2" << Endl;
        TString buffer = input.ReadAll();
        size_t len = buffer.size() / 2;
        const TChar* buf32 = (TChar*)buffer.data();

        const TDictionary& dict = GetDictRec().GetRecognizer()->GetDictionary();
        TResultCounterEncoding counter(dict);
        counter.RecognizeBuffer(buf32, buf32 + len);
        result.Encoding = counter.GetBestEncoding(hints);
    } else {
        TString buffer = input.ReadAll();
        size_t len = buffer.size();

        result.Encoding = GetDictRec().GetRecognizer()->RecognizeEncoding(buffer.c_str(), len);
        GetDictRec().GetRecognizer()->RecognizeLanguage(buffer.c_str(), len, result.Encoding, result.Languages, hints);
    }
}

void TRecognizersShell::WriteResults(const TResult& result) const {
    if (PrintResults) {
        Cout << NameByCharset(result.Encoding) << "\t" << result.Languages << Endl;
    }

    if (Singleton<TDumper>()->RequirePath())
        Cout << Endl;
}

void TRecognizersShell::ProcessEachLine(IInputStream& input) const {
    ECharset encoding = CODES_UTF8;

    size_t count = 0;
    TString line;

    while (input.ReadLine(line)) {
        ++count;
        TResult result;

        if (Recognizer == DictRec) {
            GetDictRec().GetRecognizer()->RecognizeLanguage(line.c_str(), line.size(), encoding, result.Languages);
        }
        if (PrintResults) {
            Cout << count << "\t" << result.Languages << Endl;
        }
    }
}

// Free functions

void ProcessDirectory(const TString& dirpath, const TRecognizersShell& recognizers) {
    TDirIterator directory(dirpath);

    for (auto i = directory.begin(), mi = directory.end(); i != mi; ++i) {
        if (i->fts_info == FTS_F) {
            recognizers.ProcessFile(i->fts_path);
        }
    }
}

void ProcessListOfFiles(const TString& path, const TRecognizersShell& recognizers) {
    TFileInput file(path);
    ProcessListOfFiles(file, recognizers);
}

void ProcessListOfFiles(IInputStream& input, const TRecognizersShell& recognizers) {
    TString line;

    while (input.ReadLine(line)) {
        StripInPlace(line);
        if (line.empty())
            continue;

        TVector<TString> temp;
        StringSplitter(line).Split('\t').Collect(&temp);

        TString path = temp.at(0);
        TRecognizersShell::THints hints;
        hints.HttpCodepage = EncodingHintByName((temp.size() > 1) ? temp.at(1).c_str() : nullptr);

        if (temp.size() > 2) {
            size_t index = 2;
            if (temp.size() > 3) { // 3-d position is result
                index = 3;
            }

            hints.Url = temp[index].c_str();
        }

        if (temp.size() > 4) {
            hints.HttpLanguage = temp[4].c_str();
        }

        recognizers.ProcessFile(path, hints);
    }
}
