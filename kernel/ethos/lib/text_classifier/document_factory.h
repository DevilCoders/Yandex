#pragma once

#include "document.h"

#include <kernel/lemmas_merger/lemmas_merger.h>

#include <util/generic/ptr.h>
#include <util/system/type_name.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NEthos {

class TDocumentFactory {
public:
    virtual ~TDocumentFactory() {}

    virtual TDocument DocumentFromString(TStringBuf text) const = 0;
    virtual TDocument DocumentFromWtring(const TUtf16String& wideText) const = 0;
    virtual TDocument DocumentFromCleanWtring(const TUtf16String& cleanWideText) const = 0;

    TBinaryLabelDocument BinaryLabelDocumentFromPoolString(const ui32 index,
                                                           const TString& poolString,
                                                           const TString& targetLabel,
                                                           bool hasIdColumn = false,
                                                           bool hasWeightColumn = false) const;

    TMultiLabelDocument MultiLabelDocumentFromPoolString(const ui32 index,
                                                         const TString& poolString,
                                                         THashSet<TString>* labelSet,
                                                         bool hasIdColumn = false,
                                                         bool hasWeightColumn = false) const;

    virtual void Save(IOutputStream* s) const = 0;

    virtual void Load(IInputStream* s) = 0;

    virtual THolder<TDocumentFactory> Clone() = 0;

private:
    void DocumentAndIdFromStringBuf(TStringBuf& stringBuf,
                                    bool hasIdColumn,
                                    bool hasWeightColumn,
                                    TDocument* document,
                                    float* weight,
                                    TString* id) const;
};

class TSimpleHashFactory: public TDocumentFactory {
private:
    bool CleanText;

public:
    TSimpleHashFactory(bool cleanText = true)
        : CleanText(cleanText)
    {
    }

    TDocument DocumentFromString(TStringBuf text) const override;
    TDocument DocumentFromWtring(const TUtf16String& wideText) const override;
    TDocument DocumentFromCleanWtring(const TUtf16String& cleanWideText) const override;

    void Save(IOutputStream* s) const override {
        ::SaveMany(s, CleanText);
    }

    void Load(IInputStream* s) override {
        ::LoadMany(s, CleanText);
    }

    THolder<TDocumentFactory> Clone() override {
        return THolder<TDocumentFactory>(new TSimpleHashFactory(*this));
    }
};

class THashFactory: public TDocumentFactory {
public:
    TDocument DocumentFromString(TStringBuf text) const override;
    TDocument DocumentFromWtring(const TUtf16String& wideText) const override;
    TDocument DocumentFromCleanWtring(const TUtf16String& cleanWideText) const override;

    void Save(IOutputStream*) const override {
    }

    void Load(IInputStream*) override {
    }

    THolder<TDocumentFactory> Clone() override {
        return THolder<TDocumentFactory>(new THashFactory(*this));
    }
};

class TLemmasMergerFactory: public TDocumentFactory {
public:
    NLemmasMerger::TLemmasMerger LemmasMerger;

public:
    TLemmasMergerFactory() {}

    // XXX: add move constructor
    TLemmasMergerFactory(const NLemmasMerger::TLemmasMerger& lemmasMerger)
        : LemmasMerger(lemmasMerger)
    {
    }

    TLemmasMergerFactory(const TString& lmIndexFile) {
        LemmasMerger.LoadFromFile(lmIndexFile);
    }

    void LoadLemmasMerger(const TBlob data) {
        LemmasMerger.Init(data);
    }

    const NLemmasMerger::TLemmasMerger& GetLemmasMerger() const {
        return LemmasMerger;
    }

    void LoadLemmasMerger(IInputStream* in) {
        LemmasMerger.Load(in);
    }

    TDocument DocumentFromString(TStringBuf text) const override;
    TDocument DocumentFromWtring(const TUtf16String& wideText) const override;
    TDocument DocumentFromCleanWtring(const TUtf16String& cleanWideText) const override;

    void Save(IOutputStream* s) const override {
        ::SaveMany(s, LemmasMerger);
    }
    void Load(IInputStream* s) override {
        ::LoadMany(s, LemmasMerger);
    }

    THolder<TDocumentFactory> Clone() override {
        return THolder<TDocumentFactory>(new TLemmasMergerFactory(*this));
    }
};

class TDirectTextLemmerFactory: public TDocumentFactory {
public:
    struct TOptions {
       TString PureTriePath = "pure.trie";
       TString PureLangConfigPath = "pure.lang.config";
       TString DictPath = "dict.dict";

       Y_SAVELOAD_DEFINE(PureTriePath, PureLangConfigPath, DictPath);
    };

private:
    TOptions Options;
public:
    TDirectTextLemmerFactory()
    {
    }

    TDirectTextLemmerFactory(const TOptions& options)
        : Options(options)
    {
    }

    TDocument DocumentFromString(TStringBuf text) const override;
    TDocument DocumentFromWtring(const TUtf16String& wideText) const override;
    TDocument DocumentFromCleanWtring(const TUtf16String& cleanWideText) const override;

    void Save(IOutputStream* s) const override {
        ::SaveMany(s, Options);
    }

    void Load(IInputStream* s) override {
        ::LoadMany(s, Options);
    }

    THolder<TDocumentFactory> Clone() override {
        return THolder<TDocumentFactory>(new TDirectTextLemmerFactory(*this));
    }
};

void SaveDocumentFactory(IOutputStream* s, TDocumentFactory* documentFactory);

THolder<TDocumentFactory> LoadDocumentFactory(IInputStream* s);

TBinaryLabelDocuments ReadBinaryLabelTextFromStream(IInputStream& in,
                                                    const TDocumentFactory& documentFactory,
                                                    const TString& targetLabel,
                                                    bool hasIdColumn = false,
                                                    bool hasWeight = false);

TBinaryLabelDocuments ReadBinaryLabelTextFromFile(const TString& fileName,
                                                  const TDocumentFactory& documentFactory,
                                                  const TString& targetLabel,
                                                  bool hasIdColumn = false,
                                                  bool hasWeight = false);

TMultiLabelDocuments ReadMultiLabelTextFromStream(IInputStream& in,
                                                  const TDocumentFactory& documentFactory,
                                                  TVector<TString>* allLabels,
                                                  bool hasIdColumn = false,
                                                  bool hasWeight = false);

TMultiLabelDocuments ReadMultiLabelTextFromFile(const TString& fileName,
                                                const TDocumentFactory& documentFactory,
                                                TVector<TString>* allLabels,
                                                bool hasIdColumn = false,
                                                bool hasWeight = false);

TMultiLabelDocuments ReadMultiLabelHashesFromStream(IInputStream& in, TVector<TString>* allLabels);

TBinaryLabelDocuments ReadBinaryLabelHashesFromStream(IInputStream& in, const TString& targetLabel);

TMultiLabelDocuments ReadMultiLabelHashesFromFile(const TString& fileName, TVector<TString>* allLabels);

TBinaryLabelDocuments ReadBinaryLabelHashesFromFile(const TString& fileName, const TString& targetLabel);

}
