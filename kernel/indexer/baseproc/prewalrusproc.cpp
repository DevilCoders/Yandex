#include "prewalrusproc.h"
#include "directtextaction.h"
#include "archproc.h"
#include "archproc.h"
#include "indexconf.h"

#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/posindex/invcreator.h>
#include <kernel/indexer/parseddoc/pdstorage.h>

#include <kernel/keyinv/indexfile/indexstoragefactory.h>

#include <ysite/directtext/fio/actual_fio_processor.h>
#include <ysite/directtext/dater/dater.h>
#include <ysite/directtext/segment_trigrams/segment_trigrams.h>
#include <ysite/directtext/freqs/freqs.h>
#include <ysite/directtext/telextractor/phone_processor.h>
#include <ysite/directtext/kcparser/kcparser.h>
#include <ysite/directtext/grammar/grammar.h>
#include <ysite/directtext/url_segments/chooser.h>
#include <ysite/directtext/measure/measureprocessor.h>

#include <util/folder/path.h>

using NFioExtractor::TActualFioProcessor;

namespace NIndexerCore {

TPrewalrusProcessor::TPrewalrusProcessor(TAutoPtr<TParsedDocStorage> docStorage, const TIndexProcessorConfig* cfg, IDisambDirectText* disamber, IDirectTextCallback4* invCreator, const TGroupProcessorConfig* grpCfg, TDocumentProcessor::IEventsProcessor* eventsProc, const TSimpleSharedPtr<TCompatiblePureContainer>* pure)
    : TDocumentProcessor(docStorage, cfg, grpCfg, eventsProc, pure)
    , FreqCalculator(new TFreqCalculator(cfg->UseFreqAnalize, cfg->NeededFreqKeys))
    , GrammarProcessor(new TGrammarProcessor(cfg->PornoWeightsFile))
{
    AddDirectTextCallback(FreqCalculator.Get());
    AddDirectTextCallback(GrammarProcessor.Get());
    if (cfg->UseExtProcs) {
        DirectTextProcessors.push_back(new TPhoneProcessor);
        DirectTextProcessors.push_back(new TActualFioProcessor(TFsPath(cfg->NameExtractorDataPath) / "name_extractor.cfg"));
        if (cfg->UseKCParser)
            DirectTextProcessors.push_back(new TKCParser);
        if (cfg->UseDater)
            DirectTextProcessors.push_back(new TDater(false, cfg->SCluster, cfg->WorkDir));
        DirectTextProcessors.push_back(new TSegmentTrigrams);
        DirectTextProcessors.push_back(new TUrlSegmenter());
        if (!cfg->NumberExtractorCfgPath.empty()) {
            DirectTextProcessors.push_back(new NMeasure::TMeasureProcessor(cfg->NumberExtractorCfgPath));
        }
        for (TList<TIntrusivePtr<IDirectTextProcessor> >::iterator itProc = DirectTextProcessors.begin(); itProc != DirectTextProcessors.end(); ++itProc) {
            (*itProc)->SetProcessorDir(cfg->DatHome.data(), cfg->SCluster.data());
            AddDirectTextCallback((*itProc).Get());
        }
    }
    if (disamber)
        SetDisamber(disamber);
    if (invCreator)
        SetInvCreator(invCreator);
}

TPrewalrusProcessor::~TPrewalrusProcessor() {
}

void TPrewalrusProcessor::SetArcCreator(const TArchiveProcessorConfig* cfg, IOutputStream& outArc) {
    TextArchiveAction.Reset(new TTextArchiveAction(cfg, outArc));
    TDocumentProcessor::SetArcCreator(TextArchiveAction->GetArchiveCreator());
    AddAction(TextArchiveAction.Get());
}

}
