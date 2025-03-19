#include "docprocessor.h"
#include "directtextaction.h"
#include "segmentaction.h"
#include "archproc.h"
#include "indexconf.h"
#include "groupproc.h"

#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/faceproc/docattrinserter.h>
#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/indexer/directindex/extratext.h>
#include <kernel/indexer/parseddoc/pdstorage.h>

#include <kernel/tarc/iface/tarcface.h>
#include <ysite/directtext/textarchive/createarc.h>

#include <yweb/robot/dbscheeme/urlflags.h>
#include <kernel/indexer_iface/yandind.h>

namespace NIndexerCore {

TDocumentProcessor::TDocumentProcessor(TAutoPtr<TParsedDocStorage> docStorage, const TIndexProcessorConfig* cfg, const TGroupProcessorConfig* grpCfg, IEventsProcessor* eventsProc, const TSimpleSharedPtr<TCompatiblePureContainer>* pure)
    : ParsedDocStorage(docStorage)
    , PlainArchiveCreator(nullptr)
    , Disamber(nullptr)
    , InvCreator(nullptr)
    , EventsProcessor(eventsProc)
{
    TDTCreatorConfig dtcCfg;
    dtcCfg.PureTrieFile = cfg->PureTrieFile;
    dtcCfg.PureLangConfigFile = cfg->PureLangConfigFile;
    if (cfg->NoMorphology) {
        NLemmer::TAnalyzeWordOpt option;
        option.AcceptDictionary = TLangMask();
        option.AcceptBastard = TLangMask();
        option.AcceptSob = option.AcceptBastard;
        option.ReturnFoundlingAnyway = true;
        DTCreator.Reset(new TDirectTextCreator(dtcCfg, TLangMask(cfg->DefaultLangMask), LANG_UNK, &option, 1, pure));
    } else {
        DTCreator.Reset(new TDirectTextCreator(dtcCfg, TLangMask(cfg->DefaultLangMask), LANG_UNK, &NLemmer::TAnalyzeWordOpt::IndexerOpt(), 1, pure));
    }
    DirectTextAction.Reset(new TDirectTextAction(*DTCreator));
    AddAction(DirectTextAction.Get());
    if (cfg->StoreSegmentatorData) {
        SegmentAction.Reset(new TSegmentAction(*DTCreator, cfg->Dom2Path));
        AddAction(SegmentAction.Get());
    }

    if (grpCfg)
        GroupPr.Reset(new TGroupProcessor(grpCfg));

    if (EventsProcessor)
        EventsProcessor->OnCreate(this);
}

TDocumentProcessor::~TDocumentProcessor() {
}

void TDocumentProcessor::AddAction(IDocumentAction* action) {
    DocumentActions.AddAction(action);
}

void TDocumentProcessor::SetDisamber(IDisambDirectText* obj) {
    Y_ASSERT(obj);
    Disamber = obj;
}

void TDocumentProcessor::SetInvCreator(IDirectTextCallback4* obj) {
    Y_ASSERT(obj);
    InvCreator = obj;
}

void TDocumentProcessor::AddInvCreator(IDirectTextCallback4* obj) {
    Y_ASSERT(obj);
    if (obj) {
        for (auto&& i : AdditionalInvCreators) {
            Y_VERIFY(i != obj, "Incorrect InvCreator");
        }
        AdditionalInvCreators.push_back(obj);
    }
}

void TDocumentProcessor::SetArcCreator(TArchiveCreator* obj) {
    Y_ASSERT(obj);
    PlainArchiveCreator = obj;
}

void TDocumentProcessor::AddDirectTextCallback(IDirectTextCallback2* obj) {
    Y_ASSERT(obj);
    Callbacks.push_back(obj);
}

void TDocumentProcessor::ProcessOneDoc(const TDocInfoEx* docInfo, TFullDocAttrs* docAttrs, const NIndexerCore::TExtraTextZones* extraText) {
    if (docInfo->UrlFlags & UrlFlags::NOINDEX && docInfo->UrlFlags & UrlFlags::NOFOLLOW)
        ythrow yexception() << "Both NOINDEX and NOFOLLOW are set for " <<  docInfo->DocHeader->Url;
    bool isFull = (docInfo->UrlFlags & UrlFlags::NOINDEX) == 0;
    if (!isFull && docInfo->DocHeader->MimeType != MIME_HTML)
        return;

    THolder<IParsedDocProperties> props(CreateParsedDocProperties());
    for (TFullDocAttrs::TConstIterator i = docAttrs->Begin(); i != docAttrs->End(); ++i) {
        if (i->Type & TFullDocAttrs::AttrAuxPars) {
            props->SetProperty(i->Name.data(), i->Value.data());
        }
    }

    const ECharset origEnc = (ECharset)docInfo->DocHeader->Encoding;
    ParsedDocStorage->ParseDoc(props.Get(), docInfo);
    ParsedDocStorage->RecognizeDoc(props.Get(), docInfo);

    DTCreator->AddDoc((isFull ? docInfo->DocId : YX_NEWDOCID), (ELanguage)docInfo->DocHeader->Language);

    INumeratorHandler* numHandler = DocumentActions.OnDocProcessingStart(docInfo, isFull);
    ParsedDocStorage->NumerateDoc(*numHandler, props.Get(), docInfo);

    if (extraText && extraText->size())
        AppendExtraText(*DTCreator, *extraText);

    TDocAttrInserter inserter(docAttrs);
    DocumentActions.OnDocProcessingStop(props.Get(), docInfo, &inserter, isFull);

    DTCreator->CommitDoc();

    if (Disamber) {
        DTCreator->CreateDisambMasks(*Disamber);
    }

    for (size_t i = 0; i < Callbacks.size(); i++) {
        IDirectTextCallback2* cb = Callbacks[i];
        cb->SetCurrentDoc(*docInfo);
        DTCreator->ProcessDirectText(*cb, &inserter);
    }
    inserter.Flush();

    if (EventsProcessor) {
        EventsProcessor->OnDocAttrsCreated(props.Get(), docInfo->DocId, *docAttrs);
    }

    if (PlainArchiveCreator) {
        DTCreator->ProcessDirectText(*PlainArchiveCreator, docInfo, docAttrs);
    }
    if (InvCreator) {
        DTCreator->CreateInvertIndex(*InvCreator, docInfo, docAttrs);
    }
    for (auto&& i : AdditionalInvCreators) {
        DTCreator->CreateInvertIndex(*i, docInfo, docAttrs);
    }
    if (!!GroupPr) {
        GroupPr->CommitDoc(props.Get(), docInfo->DocId, *docAttrs);
    }

    DTCreator->ClearDirectText();

    // restore encoding of the original document before storing full archive:
    // if there is ConvText then all processing including encoding/language recognition
    // is performed with chunks generated from ConvText, so origEnc is restored if it is not
    // equal to CODES_UNKNOWN
    if (origEnc != CODES_UNKNOWN)
        docInfo->DocHeader->Encoding = origEnc;
}


void TDocumentProcessor::SetCurrentUrl(const TString& url) {
    if (PlainArchiveCreator)
        PlainArchiveCreator->SetCurrentUrl(url);
}

void TDocumentProcessor::SetParserConf(const TString& name) {
    ParsedDocStorage->SetConf(name);
}

void TDocumentProcessor::Term() {
    DocumentActions.Term();
    for (size_t i = 0; i < Callbacks.size(); i++) {
        IDirectTextCallback2* cb = Callbacks[i];
        cb->Finish();
    }
    Callbacks.clear();

    PlainArchiveCreator = nullptr;
    Disamber = nullptr;
    InvCreator = nullptr;
    AdditionalInvCreators.clear();
    if (!!GroupPr) {
        GroupPr->Term();
        GroupPr.Destroy();
    }
    if (EventsProcessor)
        EventsProcessor->OnTerm();
}

}
