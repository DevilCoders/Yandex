#pragma once

#include "docactionface.h"
#include <kernel/indexer/directindex/extratext.h>
#include <kernel/indexer/face/docinfo.h>

class TArchiveCreator;
struct TIndexProcessorConfig;
class TFullDocAttrs;
class TGroupProcessor;
struct TGroupProcessorConfig;
class TCompatiblePureContainer;

namespace NIndexerCore {

class TDirectTextCreator;
class TDirectTextAction;
class IDirectTextCallback2;
class IDirectTextCallback4;
class TInvCreatorDTCallback;
class TSegmentAction;
class TParsedDocStorage;
class IDisambDirectText;
struct TExtraText;

class TDocumentProcessor {
public:
    class IEventsProcessor {
    public:
        virtual ~IEventsProcessor() {
        }
        virtual void OnCreate(TDocumentProcessor* /*processor*/) {
        }
        virtual void OnDocAttrsCreated(const IParsedDocProperties* /*ps*/, ui32 /*docId*/, TFullDocAttrs& /*extAttrs*/) {
        }
        virtual void OnTerm() {
        }
    };
private:
    THolder<TParsedDocStorage> ParsedDocStorage;
    THolder<TDirectTextCreator> DTCreator;
    THolder<TDirectTextAction> DirectTextAction;
    THolder<TSegmentAction> SegmentAction;
    TDocumentActions DocumentActions;
    TVector<IDirectTextCallback2*> Callbacks;
    TArchiveCreator* PlainArchiveCreator;
    IDisambDirectText* Disamber;
    TVector<IDirectTextCallback4*> AdditionalInvCreators;
    IDirectTextCallback4* InvCreator;
    THolder<TGroupProcessor> GroupPr;
    TDocumentProcessor::IEventsProcessor* EventsProcessor;
public:
    TDocumentProcessor(TAutoPtr<TParsedDocStorage> docStorage, const TIndexProcessorConfig* cfg, const TGroupProcessorConfig* grpCfg = nullptr, IEventsProcessor* eventsProc = nullptr, const TSimpleSharedPtr<TCompatiblePureContainer>* pure = nullptr);
    ~TDocumentProcessor();

    void AddAction(IDocumentAction* action);
    void SetDisamber(IDisambDirectText* obj);
    void SetInvCreator(IDirectTextCallback4* obj);
    void AddInvCreator(IDirectTextCallback4* obj);
    void SetArcCreator(TArchiveCreator* obj);
    void AddDirectTextCallback(IDirectTextCallback2* obj);

    void SetCurrentUrl(const TString& url);
    void SetParserConf(const TString& name);
    void ProcessOneDoc(const TDocInfoEx*, TFullDocAttrs*, const NIndexerCore::TExtraTextZones* = nullptr);
    void Term();
};

}

