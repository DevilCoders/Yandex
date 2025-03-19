#pragma once

#include <kernel/tarc/docdescr/docdescr.h>
#include <library/cpp/numerator/numerate.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/spec/attrs.h>
#include <library/cpp/html/spec/lextype.h>
#include <library/cpp/html/face/parsface.h>
#include <library/cpp/microbdb/safeopen.h>
#include <yweb/robot/dbscheeme/baserecords.h>
#include <ysite/directtext/linker/hosttable.h>
#include <util/datetime/parser.h>

class TRssNumeratorHandler : public INumeratorHandler {
    enum ERssNumeratorState {
        STATE_NORMAL = 0,
        STATE_LI,
        STATE_H4,
        STATE_ANCHOR,
        STATE_A_END,
        STATE_H4_END,
        STATE_DATE,
        STATE_DIV_DATE_END,
        STATE_ITEMCONTENT,
        STATE_DIV_ITEMCONTENT_END,
        STATE_AUTHOR,
        STATE_DIV_AUTHOR_END
    };

    typedef void (TRssNumeratorHandler::*TStateHandler)(const THtmlChunk&);

private:
    TString Link;
    TString Description;
    TString Date;
    TString Anchor;
    TString Author;
    TString Source;
    ui32 HostId;
    THostsTable* HostTable;
    ERssNumeratorState State;
    TVector<TRssLinkRec> Records;
    TVector<TRssLinkRec::TExtInfo> ExtRecords;
    TVector<TRssDateRec> DateRecords;
    TVector<TStateHandler> Handlers;
    i32 DivCount;

    void Clear();
    void Store();
    bool GetHostIdUrlIdByUrl(const TString& url, urlid_t& result);

    bool CheckAttr(const NHtml::TAttribute* attr, const char* attrName, const char* attrValue, const char* text);

    bool CheckTag(const THtmlChunk& e, HT_TAG tag, bool isClose = false);

    void SetFromAttr(TString& target, const NHtml::TAttribute* attr, const char* text);

    bool CheckLink(const THtmlChunk& e);

    bool CheckText(const THtmlChunk& e, TString* target);

    bool Check(const THtmlChunk& e, HT_TAG tag, const char* attrName, const char* attrValue);

    bool IsTrash(const THtmlChunk& e);

    void ProcessNormal(const THtmlChunk& e);
    void ProcessLi(const THtmlChunk& e);
    void ProcessH4(const THtmlChunk& e);
    void ProcessAnchor(const THtmlChunk& e);
    void ProcessAEnd(const THtmlChunk& e);
    void ProcessH4End(const THtmlChunk& e);
    void ProcessDate(const THtmlChunk& e);
    void ProcessDivDateEnd(const THtmlChunk& e);
    void ProcessItemContent(const THtmlChunk& e);
    void ProcessDivItemContentEnd(const THtmlChunk& e);
    void ProcessAuthor(const THtmlChunk& e);
    void ProcessDivAuthorEnd(const THtmlChunk& e);

public:
    TRssNumeratorHandler(ui32 hostId, THostsTable* hosts, const TString& source);

    void OnAddEvent(const THtmlChunk& e) override;

    void StoreResult(TOutDatFile<TRssLinkRec>& outputLinks, TDatSorterMemo<TRssDateRec, ByUid>& outputDates) const;
    void StoreResult(TOutDatFile<TRssLinkRec>& links, TOutDatFile<TRssDateRec>& dates) const;
};
