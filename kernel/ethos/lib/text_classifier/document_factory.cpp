#include "document_factory.h"

#include <library/cpp/token/charfilter.h>

#include <util/charset/wide.h>
#include <util/digest/numeric.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <util/string/type.h>

namespace NEthos {

void TDocumentFactory::DocumentAndIdFromStringBuf(TStringBuf& stringBuf,
                                                  bool hasIdColumn,
                                                  bool hasWeight,
                                                  TDocument* document,
                                                  float* weight,
                                                  TString* id) const
{
    if (hasIdColumn) {
        *id = TString(stringBuf.NextTok('\t'));
    }
    *weight = 1.;
    if (hasWeight) {
        *weight = FromString(stringBuf.NextTok('\t'));
    }
    TStringBuf text = stringBuf.NextTok('\t');
    *document = DocumentFromString(text);
}

TBinaryLabelDocument TDocumentFactory::BinaryLabelDocumentFromPoolString(const ui32 index,
                                                                         const TString& poolString,
                                                                         const TString& targetLabel,
                                                                         bool hasIdColumn,
                                                                         bool hasWeight) const
{
    TStringBuf poolStringBuf(poolString);

    TString id;
    TDocument document;
    float weight;
    DocumentAndIdFromStringBuf(poolStringBuf, hasIdColumn, hasWeight, &document, &weight, &id);

    EBinaryClassLabel label = EBinaryClassLabel::BCL_NEGATIVE;

    bool hasLabel = true;
    while (hasLabel) {
        TStringBuf labelBuf;
        hasLabel = poolStringBuf.NextTok('\t', labelBuf);
        if (hasLabel && targetLabel == labelBuf) {
            label = EBinaryClassLabel::BCL_POSITIVE;
            break;
        }
    }

    return TBinaryLabelDocument(std::move(document), index, std::move(id), label, weight);
}

TMultiLabelDocument TDocumentFactory::MultiLabelDocumentFromPoolString(const ui32 index,
                                                                       const TString& poolString,
                                                                       THashSet<TString>* labelSet,
                                                                       bool hasIdColumn,
                                                                       bool hasWeight) const
{
    TStringBuf poolStringBuf(poolString);

    TString id;
    TDocument document;
    float weight;

    DocumentAndIdFromStringBuf(poolStringBuf, hasIdColumn, hasWeight, &document, &weight, &id);

    TVector<TString> labels;
    bool hasLabel = true;
    while (hasLabel) {
        TStringBuf label;
        hasLabel = poolStringBuf.NextTok('\t', label);
        if (hasLabel) {
            labels.push_back(ToString(label));
        }
    }

    if (!!labelSet) {
        labelSet->insert(labels.begin(), labels.end());
    }

    return TMultiLabelDocument(std::move(document), index, std::move(id), std::move(labels), weight);
}

TDocument TSimpleHashFactory::DocumentFromString(TStringBuf text) const {
    return DocumentFromWtring(UTF8ToWide<true>(text));
}

TDocument TSimpleHashFactory::DocumentFromWtring(const TUtf16String& wideText) const {
    return DocumentFromCleanWtring(NLemmasMerger::TLemmasMerger::GetCleanedText(wideText));
}

TDocument TSimpleHashFactory::DocumentFromCleanWtring(const TUtf16String& cleanWideText) const {
    TVector<ui64> unigramHashes;

    TWtringBuf buf(cleanWideText);

    for (;;) {
        TWtringBuf word;
        if (!buf.NextTok(' ', word)) {
            break;
        }

        size_t hash = ComputeHash(word);
        unigramHashes.push_back(hash);
    }

    return TDocument(std::move(unigramHashes));
}

namespace {
    template <typename TCharType>
    inline TDocument HashDocumentFromStrBuf(TBasicStringBuf<TCharType> strBuf) {
        TVector<ui64> unigramHashes;
        while (strBuf) {
            TBasicStringBuf<TCharType> next = strBuf.NextTok(' ');
            if (IsNumber(next)) {
                unigramHashes.push_back(FromString<ui64>(next));
            }
        }

        return TDocument(std::move(unigramHashes));
    }
}

TDocument THashFactory::DocumentFromString(TStringBuf text) const {
    return HashDocumentFromStrBuf<char>(text);
}

TDocument THashFactory::DocumentFromWtring(const TUtf16String& wideText) const {
    return DocumentFromCleanWtring(wideText);
}

TDocument THashFactory::DocumentFromCleanWtring(const TUtf16String& cleanWideText) const {
    return HashDocumentFromStrBuf<wchar16>(cleanWideText);
}

TDocument TLemmasMergerFactory::DocumentFromString(TStringBuf text) const {
    TUtf16String wideText = UTF8ToWide<true>(text);
    const size_t wideTextLengthUpperBound = 10000;
    if (wideText.length() > wideTextLengthUpperBound) {
        wideText.erase(wideTextLengthUpperBound);
    }

    return DocumentFromWtring(wideText);
}

TDocument TLemmasMergerFactory::DocumentFromWtring(const TUtf16String& wideText) const {
    return DocumentFromCleanWtring(LemmasMerger.GetCleanedText(wideText));
}

TDocument TLemmasMergerFactory::DocumentFromCleanWtring(const TUtf16String& cleanWideText) const {
    TDocument document;
    LemmasMerger.ReadHashesClean(document.UnigramHashes, cleanWideText);
    return document;
}

TDocument TDirectTextLemmerFactory::DocumentFromString(TStringBuf text) const {
    TUtf16String wideText = NormalizeUnicode(UTF8ToWide<true>(text));

    TVector<size_t> unigramHashes;

    size_t position = 0;
    while (position < wideText.size()) {
        while (position < wideText.size() && !IsAlpha(wideText[position]) && !IsDigit(wideText[position])) {
            ++position;
        }

        TUtf16String alphaWord;
        while (position < wideText.size() && IsAlpha(wideText[position])) {
            alphaWord += wideText[position];
            ++position;
        }
        TUtf16String digitWord;
        while (position < wideText.size() && IsDigit(wideText[position])) {
            digitWord += wideText[position];
            ++position;
        }

        if (digitWord) {
            while (digitWord.size() < 11) {
                digitWord = u"0" + digitWord;
            }
            unigramHashes.push_back(ComputeHash(digitWord));
        }
        if (alphaWord) {
            unigramHashes.push_back(ComputeHash(alphaWord));
        }
    }

    return TDocument(std::move(unigramHashes));
}

TDocument TDirectTextLemmerFactory::DocumentFromWtring(const TUtf16String& wideText) const {
    return DocumentFromString(WideToUTF8(wideText));
}

TDocument TDirectTextLemmerFactory::DocumentFromCleanWtring(const TUtf16String& cleanWideText) const {
    return DocumentFromString(WideToUTF8(cleanWideText));
}

void SaveDocumentFactory(IOutputStream* s, TDocumentFactory* documentFactory) {
    ::Save(s, TypeName(*documentFactory));
    documentFactory->Save(s);
}

template <typename TDocumentFactoryType>
bool HasDocumentFactoryType(const TString& typeName) {
    const TString documentFactoryTypeName = TypeName<TDocumentFactoryType>();
    return documentFactoryTypeName.find(typeName) != TString::npos ||
           typeName.find(documentFactoryTypeName) != TString::npos;
}

THolder<TDocumentFactory> LoadDocumentFactory(IInputStream* s) {
    TString typeName;
    ::Load(s, typeName);

    THolder<TDocumentFactory> documentFactory;
    if (HasDocumentFactoryType<TSimpleHashFactory>(typeName)) {
        documentFactory = MakeHolder<TSimpleHashFactory>();
    } else if (HasDocumentFactoryType<THashFactory>(typeName)) {
        documentFactory = MakeHolder<THashFactory>();
    } else if (HasDocumentFactoryType<TLemmasMergerFactory>(typeName)) {
        documentFactory = MakeHolder<TLemmasMergerFactory>();
    } else if (HasDocumentFactoryType<TDirectTextLemmerFactory>(typeName)) {
        documentFactory = MakeHolder<TDirectTextLemmerFactory>();
    } else {
        ythrow yexception() << "Unknown DocumentFactory type: " << typeName;
    }

    documentFactory->Load(s);

    return documentFactory;
}

template <typename TLabeledDocument, typename TDocumentFactoryMethod>
TVector<TLabeledDocument> ReadLabeledTextFromStream(IInputStream& in, const TDocumentFactoryMethod& documentFactoryMethod) {
    TVector<TLabeledDocument> documents;

    TString poolString;
    ui32 index = 0;
    while (in.ReadLine(poolString)) {
        documents.push_back(documentFactoryMethod(index++, poolString));

        // XXX: remove logging from the library
        if (documents.size() % 1000 == 0) {
            Cerr << documents.size() << " documents read...\n";
        }
    }

    Cerr << "read " << documents.size() << " documents!\n";

    return documents;
}

TBinaryLabelDocuments ReadBinaryLabelTextFromStream(IInputStream& in,
                                                    const TDocumentFactory& documentFactory,
                                                    const TString& targetLabel,
                                                    bool hasIdColumn,
                                                    bool hasWeight)
{
    return ReadLabeledTextFromStream<TBinaryLabelDocument>(in, [&] (const ui32 index, const TString& poolString) {
        return documentFactory.BinaryLabelDocumentFromPoolString(index, poolString, targetLabel, hasIdColumn, hasWeight);
    });
}

TBinaryLabelDocuments ReadBinaryLabelTextFromFile(const TString& fileName,
                                                  const TDocumentFactory& documentFactory,
                                                  const TString& targetLabel,
                                                  bool hasIdColumn,
                                                  bool hasWeight)
{
    TFileInput in(fileName);
    return ReadBinaryLabelTextFromStream(in, documentFactory, targetLabel, hasIdColumn, hasWeight);
}

TMultiLabelDocuments ReadMultiLabelTextFromStream(IInputStream& in,
                                                  const TDocumentFactory& documentFactory,
                                                  TVector<TString>* allLabels,
                                                  bool hasIdColumn,
                                                  bool hasWeight)
{
    THashSet<TString> labelSet;
    THashSet<TString>* labelSetPointer = !!allLabels ? &labelSet : nullptr;

    TMultiLabelDocuments documents = ReadLabeledTextFromStream<TMultiLabelDocument>(in, [&] (const ui32 index, const TString& poolString) {
        return documentFactory.MultiLabelDocumentFromPoolString(index, poolString, labelSetPointer, hasIdColumn, hasWeight);
    });

    if (!!allLabels) {
        allLabels->clear();
        allLabels->reserve(labelSet.size());

        for (auto& label : labelSet) {
            allLabels->push_back(label);
        }

        std::sort(allLabels->begin(), allLabels->end());
    }

    return documents;
}

TMultiLabelDocuments ReadMultiLabelTextFromFile(const TString& fileName,
                                                const TDocumentFactory& documentFactory,
                                                TVector<TString>* allLabels,
                                                bool hasIdColumn,
                                                bool hasWeight)
{
    TFileInput in(fileName);
    return ReadMultiLabelTextFromStream(in, documentFactory, allLabels, hasIdColumn, hasWeight);
}

TMultiLabelDocuments ReadMultiLabelHashesFromStream(IInputStream& in, TVector<TString>* allLabels) {
    TMultiLabelDocuments documents;
    Load(&in, documents);

    if (!!allLabels) {
        THashSet<TString> labelSet;
        for (const auto& document : documents) {
            labelSet.insert(document.Labels.begin(), document.Labels.end());
        }

        allLabels->clear();
        allLabels->reserve(labelSet.size());

        for (auto& label : labelSet) {
            allLabels->push_back(label);
        }

        std::sort(allLabels->begin(), allLabels->end());
    }

    return documents;
}

TBinaryLabelDocuments ReadBinaryLabelHashesFromStream(IInputStream& in, const TString& targetLabel) {
    TMultiLabelDocuments documents = ReadMultiLabelHashesFromStream(in, nullptr);

    TBinaryLabelDocuments result;
    result.reserve(documents.size());
    for (auto& document : documents) {
        result.push_back(TBinaryLabelDocument(std::move(document), targetLabel));
    }

    return result;
}

TMultiLabelDocuments ReadMultiLabelHashesFromFile(const TString& fileName, TVector<TString>* allLabels) {
    TFileInput in(fileName);
    return ReadMultiLabelHashesFromStream(in, allLabels);
}

TBinaryLabelDocuments ReadBinaryLabelHashesFromFile(const TString& fileName, const TString& targetLabel) {
    TFileInput in(fileName);
    return ReadBinaryLabelHashesFromStream(in, targetLabel);
}

}
