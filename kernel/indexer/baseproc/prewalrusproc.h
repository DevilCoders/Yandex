#pragma once

#include "docprocessor.h"

#include <util/generic/list.h>

class TFreqCalculator;
class TGrammarProcessor;
class IDirectTextProcessor;
struct IYndexStorageFactory;
struct TIndexProcessorConfig;
struct TArchiveProcessorConfig;
class IOutputStream;
class TCompatiblePureContainer;

namespace NIndexerCore {

class TInvCreatorDTCallback;
class TDefaultDisamber;
class TTextArchiveAction;

class TPrewalrusProcessor : public TDocumentProcessor {
private:
    THolder<TFreqCalculator> FreqCalculator;
    THolder<TGrammarProcessor> GrammarProcessor;
    TList<TIntrusivePtr<IDirectTextProcessor> > DirectTextProcessors;

    THolder<TTextArchiveAction> TextArchiveAction;

public:
    TPrewalrusProcessor(TAutoPtr<TParsedDocStorage> docStorage, const TIndexProcessorConfig* cfg,
        IDisambDirectText* disamber, IDirectTextCallback4* invCreator, const TGroupProcessorConfig* grpCfg = nullptr,
        TDocumentProcessor::IEventsProcessor* eventsProc = nullptr, const TSimpleSharedPtr<TCompatiblePureContainer>* pure = nullptr);
    ~TPrewalrusProcessor();
    void SetArcCreator(const TArchiveProcessorConfig* cfg, IOutputStream& outArc);
};

}
