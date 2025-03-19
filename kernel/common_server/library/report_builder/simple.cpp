#include "simple.h"

#include <kernel/search_daemon_iface/groupingmode.h>

#include <search/session/compression/report.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/logger/global/global.h>

#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>

#include <util/system/hostname.h>
#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/digest/fnv.h>
#include <util/stream/buffer.h>

const int NUM_PRIORITIES = 3;

namespace {
    class TEventLogDumper: public ILogFrameEventVisitor {
    public:
        TEventLogDumper(TVector<TString>& container)
            : Container(container)
        {
        }
    private:
        void Visit(const TEvent& event) override {
            Container.push_back(event.ToString());
        }
    private:
        TVector<TString>& Container;
    };

    const TString DefaultGrouping = {};
    const TString DefaultCategory = {};
    const TString HiddenProperties = "hidden";
}

void TRTYSimpleProtoReportBuilder::ConsumeReport(NMetaProtocol::TReport& report, const TString& source) {
    for (ui32 grouping = 0; grouping < report.GroupingSize(); ++grouping) {
        NMetaProtocol::TGrouping* groupingObj = report.MutableGrouping(grouping);
        for (ui32 group = 0; group < groupingObj->GroupSize(); ++group) {
            NMetaProtocol::TGroup* groupObj = groupingObj->MutableGroup(group);
            for (ui32 doc = 0; doc < groupObj->DocumentSize(); ++doc) {
                NMetaProtocol::TDocument* docObj = groupObj->MutableDocument(doc);
                docObj->SetDocId(source + "-" + docObj->GetDocId());
                AddDocumentToGroup(*docObj, groupingObj->GetAttr(), groupObj->GetCategoryName());
            }
        }
    }
    if (report.SearcherPropSize()) {
        const ui32 id = Report.SearchPropertiesSize();
        if (auto searchProperties = Report.AddSearchProperties()) {
            searchProperties->SetId(id);
            searchProperties->SetName(source);
            searchProperties->MutableProperties()->Swap(report.MutableSearcherProp());
        }
    }

    if (report.EventLogSize()) {
        *Report.AddEventLog() = "START FRAME: " + source;
        for (auto&& i : report.GetEventLog()) {
            *Report.AddEventLog() = i;
        }
        *Report.AddEventLog() = "FINISH FRAME: " + source;
    }

    if (report.HasDebugInfo()) {
        const auto& debugInfo = report.GetDebugInfo();
        if (debugInfo.HasAnswerIsComplete()) {
            Complete &= debugInfo.GetAnswerIsComplete();
        }
    }

    if (report.HasErrorInfo()) {
        const auto& errorInfo = report.GetErrorInfo();
        if (errorInfo.HasText()) {
            const TString info = source + ": " + errorInfo.GetText();
            AddErrorMessage(info);
        }
        if (errorInfo.HasCode()) {
            Code = std::max<ui64>(Code, errorInfo.GetCode());
        }
    }

    if (report.SearchPropertiesSize()) {
        for (auto&& i : report.GetSearchProperties()) {
            if (i.GetName() == HiddenProperties) {
                Hidden.MutableProperties()->MergeFrom(i.GetProperties());
            } else {
                auto *sp = Report.AddSearchProperties();
                *sp = i;
                *sp->MutableName() = source + "-" + i.GetName();
            }
        }
    }
}

const IReportBuilderContext& TRTYSimpleProtoReportBuilder::GetContext() const {
    return Context;
}

ui64 TRTYSimpleProtoReportBuilder::GetCode() const {
    return Code;
}

ui32 TRTYSimpleProtoReportBuilder::GetDocumentsCount() const {
    return GetDocumentsCount(Groupings);
}

ui32 TRTYSimpleProtoReportBuilder::GetDocumentsCount(const TGroupings& groupings) const {
    ui32 count = 0;
    for (auto&& g : groupings) {
        count += GetDocumentsCount(g.second);
    }
    return count;
}

ui32 TRTYSimpleProtoReportBuilder::GetDocumentsCount(const TGrouping& grouping) const {
    ui32 count = 0;
    for (auto&& g : grouping) {
        count += GetDocumentsCount(g.second);
    }
    return count;
}

ui32 TRTYSimpleProtoReportBuilder::GetDocumentsCount(const TGroup& group) const {
    return group.size();
}

void TRTYSimpleProtoReportBuilder::AddReportProperty(const TString& propName, const TString& propValue) {
    if (auto propertie = Report.AddSearcherProp()) {
        propertie->SetKey(propName);
        propertie->SetValue(propValue);
    }
}

void TRTYSimpleProtoReportBuilder::AddHiddenProperty(const TString& propName, const TString& propValue) {
    NMetaProtocol::TSearchProperties* properties = nullptr;
    for (size_t i = 0; i < Report.SearchPropertiesSize(); ++i) {
        if (Report.GetSearchProperties(i).GetName() == HiddenProperties) {
            properties = Report.MutableSearchProperties(i);
            break;
        }
    }
    if (!properties) {
        const ui32 id = Report.SearchPropertiesSize();
        properties = Report.AddSearchProperties();
        Y_ENSURE(properties);
        properties->SetId(id);
        properties->SetName(HiddenProperties);
    }
    CHECK_WITH_LOG(properties);
    if (auto propertie = properties->AddProperties()) {
        propertie->SetKey(propName);
        propertie->SetValue(propValue);
    }
}

void TRTYSimpleProtoReportBuilder::AddDocument(NMetaProtocol::TDocument& doc) {
    AddDocumentToGroup(doc, DefaultGrouping, DefaultCategory);
}

void TRTYSimpleProtoReportBuilder::AddDocumentToGroup(NMetaProtocol::TDocument& doc, const TString& grouping, const TString& category) {
    Groupings[grouping][category].emplace_back();
    Groupings[grouping][category].back().Swap(&doc);
}

void TRTYSimpleProtoReportBuilder::AddErrorMessage(const TString& msg) {
    DEBUG_LOG << "AddErrorMessage: " << msg << Endl;
    if (!!Errors)
        Errors += "\n";
    Errors += msg;
}

void TRTYSimpleProtoReportBuilder::MarkIncomplete(bool value) {
    Complete = !value;
}

void TRTYSimpleProtoReportBuilder::ScanReport(IScanner& scanner) const {
    for (auto&& grouping : Groupings) {
        const TString& name = grouping.first;
        for (auto&& group : grouping.second) {
            const TString& category = group.first;
            for (auto&& doc : group.second) {
                if (!doc.HasUrl() && !doc.HasArchiveInfo() && doc.FirstStageAttributeSize() == 0) {
                    continue;
                }
                scanner(doc, name, category);
            }
        }
    }
    for (auto&& grouping : Report.GetGrouping()) {
        const TString& name = grouping.GetAttr();
        for (auto&& group : grouping.GetGroup()) {
            const TString& category = group.GetCategoryName();
            for (auto&& doc : group.GetDocument()) {
                scanner(doc, name, category);
            }
        }
    }
    for (auto&& propertie : Report.GetSearcherProp()) {
        scanner(propertie.GetKey(), propertie.GetValue());
    }
    for (auto&& propertie : Hidden.GetProperties()) {
        scanner(propertie.GetKey(), propertie.GetValue());
    }
    for (auto&& sourceInfo : Report.GetSearchProperties()) {
        for (auto&& propertie : sourceInfo.GetProperties()) {
            const TString& name = Report.SearchPropertiesSize() > 1 ? (sourceInfo.GetName() + "." + propertie.GetKey()) : propertie.GetKey();
            scanner(name, propertie.GetValue());
        }
    }
}

void TRTYSimpleProtoReportBuilder::ConsumeLog(const TSelfFlushLogFramePtr eventLog) {
    if (eventLog) {
        TEventLogDumper dumper(Events);
        eventLog->VisitEvents(dumper, NEvClass::Factory());
    }
}

void TRTYSimpleProtoReportBuilder::InsertErrors(NMetaProtocol::TReport& report) const {
    if (!!Errors)
        report.MutableErrorInfo()->SetText(Errors);
}

void TRTYSimpleProtoReportBuilder::InsertEventLog(NMetaProtocol::TReport& report) const {
    for (auto&& ev : Events) {
        report.AddEventLog(ev);
    }
}

void TRTYSimpleProtoReportBuilder::FinishImpl(NMetaProtocol::TReport& report, const ui32 httpCode) {
    InsertErrors(report);
    InsertEventLog(report);

    const TCgiParameters& cgi = Context.GetCgiParameters();

    NMetaProtocol::ECompressionMethod compression = NMetaProtocol::CM_COMPRESSION_NONE;
    for (size_t i = 0; i < Context.GetCgiParameters().NumOfValues("pron"); ++i) {
        const TString& comp = Context.GetCgiParameters().Get("pron", i);
        if (comp.StartsWith("pc")) {
            TStringBuf compType(comp.data() + 2, comp.size() - 2);
            TryFromString<NMetaProtocol::ECompressionMethod>(compType, compression);
        }
    }

    auto balancingInfo = report.MutableBalancingInfo();
    balancingInfo->SetElapsed((Now() - Context.GetRequestStartTime()).MicroSeconds());

    TBuffer buf;
    TBufferOutput bufOutput(buf);

    const TString& humanReadableType = Context.GetCgiParameters().Get("hr");
    if (humanReadableType == "json") {
        NProtobufJson::Proto2Json(report, bufOutput);
        Context.AddReplyInfo("Content-Type", TString("application/json; charset=") + NameByCharset(CODES_UTF8));
    } else if (IsTrue(humanReadableType)) {
        google::protobuf::io::TCopyingOutputStreamAdaptor adapter(&bufOutput);
        Y_ENSURE(google::protobuf::TextFormat::Print(report, &adapter), "cannot serialize text protobuf");
    } else {
        if (compression != NMetaProtocol::CM_COMPRESSION_NONE) {
            NMetaProtocol::Compress(report, compression);
        }
        report.SerializeToArcadiaStream(&bufOutput);
    }

    const auto mslevel = FromStringWithDefault<ui64>(cgi.Get("mslevel"), 0);
    const ui32 code = mslevel > 0 ? HTTP_OK : httpCode;
    Context.MakeSimpleReply(buf, code);
}

void TRTYSimpleProtoReportBuilder::InsertMeta(NMetaProtocol::TReport& report, const ui32 httpCode) const {
    if (auto head = report.MutableHead()) {
        head->SetVersion(1);
        head->SetSegmentId(GetHostName());
        head->SetIndexGeneration(0);
    }
    if (auto debugInfo = report.MutableDebugInfo()) {
        debugInfo->SetBaseSearchCount(1);
        debugInfo->SetBaseSearchNotRespondCount(0);
        debugInfo->SetAnswerIsComplete(Complete && !IsServerError(httpCode));
    }
    if (auto errorInfo = report.MutableErrorInfo()) {
        errorInfo->SetGotError(Errors.empty() ? NMetaProtocol::TErrorInfo::NO : NMetaProtocol::TErrorInfo::YES);
        errorInfo->SetCode(httpCode);
    }
}

void TRTYSimpleProtoReportBuilder::InsertDocuments(NMetaProtocol::TReport& report) {
    const ui32 docsCount = GetDocumentsCount();
    for (ui32 i = 0; i < NUM_PRIORITIES; ++i) {
        report.AddTotalDocCount(docsCount);
    }

    for (auto&& grg : Groupings) {
        const TString& attribute = grg.first;
        TGrouping& groups = grg.second;

        const EGroupingMode mode = attribute.empty() ? GM_FLAT : GM_DEEP;
        const NMetaProtocol::TGrouping::TIsFlat isFlat = (mode == GM_FLAT) ? NMetaProtocol::TGrouping::YES : NMetaProtocol::TGrouping::NO;

        NMetaProtocol::TGrouping* grouping = report.AddGrouping();
        grouping->SetMode(mode);
        grouping->SetIsFlat(isFlat);
        if (mode == GM_FLAT) {
            auto& docs = groups.at(DefaultCategory);
            const ui32 numGroups = GetDocumentsCount(docs);
            for (ui32 i = 0; i < NUM_PRIORITIES; ++i) {
                grouping->AddNumGroups(numGroups);
            }
            for (auto&& doc : docs) {
                grouping->AddGroup()->AddDocument()->Swap(&doc);
            }
        } else {
            grouping->SetAttr(attribute);
            for (ui32 i = 0; i < NUM_PRIORITIES; ++i) {
                grouping->AddNumGroups(groups.size());
            }
            for (auto&& g : groups) {
                const TString& category = g.first;
                TGroup& docs = g.second;

                auto group = grouping->AddGroup();
                group->SetCategoryName(category);
                group->SetRelevance(docs.size() ? docs[0].GetRelevance() : 0);
                for (ui32 i = 0; i < NUM_PRIORITIES; ++i) {
                    group->AddRelevStat(docs.size());
                }
                for (auto&& doc : docs) {
                    group->AddDocument()->Swap(&doc);
                }
            }
        }
    }
}

void TRTYSimpleProtoReportBuilder::Finish(const ui32 httpCode) {
    InsertMeta(Report, httpCode);
    InsertDocuments(Report);
    FinishImpl(Report, httpCode);
}

void TRTYSimpleProtoReportBuilder::Finish(NMetaProtocol::TReport&& report, const ui32 httpCode) {
    Report = std::forward<NMetaProtocol::TReport>(report);
    FinishImpl(Report, httpCode);
}
