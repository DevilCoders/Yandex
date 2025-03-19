#pragma once

#include <library/cpp/charset/doccodes.h>

#include <yweb/robot/dbscheeme/mergecfg.h>
#include <ysite/directtext/linker/hosttable.h>

#include <kernel/indexer/face/docinfo.h>
#include "docactionface.h"
#include "rsshandler.h"


struct TRssProcessorConfig {
    TString DatHome;
    int Cluster;
};

class TRssProcessor : public NIndexerCore::IDocumentAction {
private:
    THolder<TRssNumeratorHandler> Handler;
    TOutDatFile<TRssLinkRec> OutputLinks;
    TDatSorterMemo<TRssDateRec, ByUid> OutputDates;
    TRssProcessorConfig Config;
    THostsTable* HostTable;

    bool IsRss(const TDocInfoEx* docInfo) {
        return docInfo->DocHeader->MimeType == MIME_RSS;
    }

public:
    TRssProcessor(const TRssProcessorConfig* config, THostsTable* hosts)
        : OutputLinks("rsslinks", dbcfg::fbufsize, 0)
        , OutputDates("rsslinksdate", dbcfg::small_sorter_size, dbcfg::pg_url, dbcfg::fbufsize, 0)
        , Config(*config)
        , HostTable(hosts)
    {
        TString outputLinksPathTemplate = TString::Join(~Config.DatHome, dbcfg::fname_clsrsslinks);
        OutputLinks.Open(~Sprintf(~outputLinksPathTemplate, Config.Cluster));
        OutputDates.Open(TString::Join(~Config.DatHome, dbcfg::fname_temp));
    }

    void Term() override {
        TString outputDatesPathTemplate = TString::Join(~Config.DatHome, dbcfg::fname_clsrsslinksdate);
        OutputDates.SortToFile(~Sprintf(~outputDatesPathTemplate, Config.Cluster));
        OutputLinks.Close();
        OutputDates.Close();
    }

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override {
        if (isFullProcessing && IsRss(docInfo)) {
            Handler.Reset(new TRssNumeratorHandler(docInfo->HostId, HostTable, docInfo->DocHeader->Url));
            return Handler.Get();
        }
        return nullptr;
    }

    void OnDocProcessingStop(const IParsedDocProperties* , const TDocInfoEx* , IDocumentDataInserter*, bool ) override {
        if (!!Handler) {
            Handler->StoreResult(OutputLinks, OutputDates);
            Handler.Destroy();
        }
    }
};
