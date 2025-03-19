#include "pdstorage.h"
#include "recogn.h"
#include <kernel/recshell/recshell.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/zoneconf/attrextractor.h>
#include <library/cpp/html/zoneconf/ht_conf.h>
#include <kernel/indexer/face/docinfo.h>

namespace NIndexerCore {

class TParsedDocStorage::THtConfs {
public:
    THtConfs(const TString& parserConfig)
        : Current(nullptr)
    {
        const TString defName;
        THtConfigurator* htc = AddConf(defName, parserConfig);
        if (!parserConfig) {
            htc->LoadDefaultConf();
        }
        SetConf(defName);
    }
    ~THtConfs() {
        for (TConfs::iterator i = Confs.begin(); i != Confs.end(); ++i) {
            delete i->second;
        }
    }
    THtConfigurator* AddConf(const TString& name, const TString& confFile) {
        std::pair<TString, THtConfigurator*> value(name, nullptr);
        std::pair<TConfs::iterator, bool> ins = Confs.insert(value);
        if (ins.second) {
            ins.first->second = new THtConfigurator;
            if (!!confFile)
                ins.first->second->Configure(confFile.data());
        }
        return ins.first->second;
    }
    void SetConf(const TString& name) {
        TConfs::const_iterator i = Confs.find(name);
        if (i == Confs.end())
            ythrow yexception() << "Can't find a configuration with name " << name;
        Current = i->second;
    }
    const THtConfigurator* GetHtConf() const {
        Y_ASSERT(Current);
        return Current;
    }
private:
    typedef THashMap<TString, THtConfigurator*> TConfs;
    TConfs Confs;
    const THtConfigurator* Current;
};

TParsedDocStorage::TParsedDocStorage(const TParsedDocStorageConfig& cfg)
    : TSimpleParsedDocStorage()
{
    if (!!cfg.RecognizeLibraryFile)
        Recognizer.Reset(new TRecognizerShell(cfg.RecognizeLibraryFile));
    HtConfs.Reset(new THtConfs(cfg.ParserConfig));
    for (TMap<TString, TString>::const_iterator it = cfg.ParserConfigs.begin(); it != cfg.ParserConfigs.end(); ++it) {
        HtConfs->AddConf(it->first, it->second);
    }
}

TParsedDocStorage::~TParsedDocStorage() {
}

void TParsedDocStorage::SetConf(const TString& name) {
    HtConfs->SetConf(name);
}

void TParsedDocStorage::ParseDoc(IParsedDocProperties* docProps, const TDocInfoEx* docInfo) {
    if (docInfo->ConvSize != 0 && docInfo->ConvText != nullptr) {
        docInfo->DocHeader->Encoding = CODES_UTF8; // converted text is always UTF8
        DoParseHtml(docInfo->ConvText, docInfo->ConvSize, docProps);
    } else {
        if (docInfo->DocHeader->MimeType == MIME_TEXT) {
            TBuffer buf(docInfo->DocSize + 25);
            buf.Append("<plaintext>\n", 12);
            buf.Append(docInfo->DocText, docInfo->DocSize);
            buf.Append("</plaintext>\n", 13);
            DoParseHtml(buf.Data(), buf.Size(), docProps);
        } else if (docInfo->DocHeader->MimeType != MIME_HTML) {
            DoParseUnknownFormat(docProps, docInfo);
        } else {
            DoParseHtml(docInfo->DocText, docInfo->DocSize, docProps);
        }
    }
    OnAfterParseDoc(docProps, docInfo);
}

void TParsedDocStorage::DoParseHtml(const char* doc, size_t sz, IParsedDocProperties* docProps) {
    TSimpleParsedDocStorage::DoParseHtml(doc, sz, GetUrl(docProps));
}

void TParsedDocStorage::RecognizeDoc(IParsedDocProperties* docProps, const TDocInfoEx* docInfo) {
    ERecognRes res = PrepareEncodingAndLanguage(Storage.Begin(), Storage.End(), docProps, Recognizer.Get(), docInfo);
    if (res == RR_UNKNOWN) {
        ythrow yexception() <<  "Parse error in: url=" <<  docInfo->DocHeader->Url << ", DocId=" <<  docInfo->DocId << ", " << "unrecognized charset or language?";
    }
}

void TParsedDocStorage::NumerateDoc(INumeratorHandler& handler, IParsedDocProperties* docProps, const TDocInfoEx* docInfo) {
    if (docInfo->ConvSize != 0 && docInfo->ConvText != nullptr) {
        DoNumerateHtml(handler, docProps);
    } else {
        if (docInfo->DocHeader->MimeType != MIME_HTML && docInfo->DocHeader->MimeType != MIME_TEXT) {
            DoNumerateUnknownFormat((MimeTypes)docInfo->DocHeader->MimeType, handler, docProps);
        } else {
            DoNumerateHtml(handler, docProps);
        }
    }
}

void TParsedDocStorage::DoNumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps) {
    TAttributeExtractor extractor(HtConfs->GetHtConf());
    TSimpleParsedDocStorage::DoNumerateHtml(handler, docProps, &extractor);
}

const THtConfigurator* TParsedDocStorage::GetHtConf() const {
    return HtConfs->GetHtConf();
}

}

