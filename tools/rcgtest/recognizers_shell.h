#pragma once

#include <util/generic/ptr.h>

#include <kernel/recshell/recshell.h>
#include <dict/recognize/docrec/stat_enc.h>

class TRecognizersShell {
public:
    typedef TRecognizerShell::THints THints;

private:
    THolder<TRecognizerShell> DictHolder;

public:
    class TResult;

    enum ERecognizer {
        UndefinedRec,
        DictRec
    };

    ERecognizer Recognizer;

    bool PrintResults;
    bool UseParser;
    bool UseUCS2;
    bool AllLanguages;
    bool LineByLine;

    size_t BufferSize;

    TString DefaultUrl;

public:
    TRecognizersShell();

    void Load(const TString& path);

    void ProcessFile(const TString& path, const THints& hints = THints()) const;
    void Process(IInputStream& input, const THints& hints = THints()) const;

    void ShowInfo() const;

private:
    TRecognizerShell& GetDictRec() const;

    void ProcessEachLine(IInputStream& input) const;

    void Recognize(IInputStream& input, TResult& result, const THints& hints) const;

    void WriteResults(const TResult& result) const;
};

void ProcessDirectory(const TString& dirpath, const TRecognizersShell& recognizers);

void ProcessListOfFiles(const TString& path, const TRecognizersShell& recognizers);
void ProcessListOfFiles(IInputStream& input, const TRecognizersShell& recognizers);
