#include <util/folder/dirut.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/parseddoc/pdstorage.h>

#include "TextAndTitleNumerator.h"
#include "HtmlParser.h"

using namespace NIndexerCore;

void NumerateHtml(const TString& html, INumeratorHandler& handler) {
    TParsedDocStorageConfig storageConfig;
    TString dictFile("dict.dict");
    if (NFs::Exists(dictFile)) {
        storageConfig.RecognizeLibraryFile = dictFile;
    }
    TString parserConfigFile("htparser.ini");
    if (NFs::Exists(parserConfigFile)) {
        storageConfig.ParserConfig = parserConfigFile;
    }
    TParsedDocStorage parsedDocStorage(storageConfig);

    THolder<IParsedDocProperties> props(CreateParsedDocProperties());
    props->SetProperty(PP_DEFCHARSET, "UTF-8");

    TFullArchiveDocHeader docHeader;
    docHeader.MimeType = MIME_HTML;
    docHeader.Language = LANG_UNK;
    docHeader.Encoding = CODES_UNKNOWN;

    TDocInfoEx docInfo;
    docInfo.Clear();
    docInfo.DocId = (ui32) -1; //YX_NEWDOCID
    docInfo.DocText = html.c_str();
    docInfo.DocSize = html.length();
    docInfo.DocHeader = &docHeader;

    parsedDocStorage.ParseDoc(props.Get(), &docInfo);
    parsedDocStorage.RecognizeDoc(props.Get(), &docInfo);
    parsedDocStorage.NumerateDoc(handler, props.Get(), &docInfo);
}

TSimpleSharedPtr<TTextAndTitleSentences> Html2Text(const TString& html) {
    TTextAndTitleNumerator textAndTitleNumerator;
    NumerateHtml(html, textAndTitleNumerator);
    return textAndTitleNumerator.GetSentences();
}
