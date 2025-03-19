#pragma once

#include <ctime>
#include <cstdlib>

#include "dp.h"
#include "groupingmode.h"

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>
#include <util/system/types.h>

#include "relevance_type.h"
#include <kernel/search_types/search_types.h>

class IRelevance;
class IDocFactorSerializer;
class TBlob;
class TSelfFlushLogFrame;
class TBufferStream;
class TServerRequestData;
struct TMakePageContext;

namespace google::protobuf {
    class Message;
}


class IReqParams {
protected:
    virtual ~IReqParams() = default;
public:
    // Old simple case
    virtual void SetSearchQuery(const char *reqtext) = 0;

    virtual void SetExternalRelevance(IRelevance* extRelev) = 0;
};

class IIndexProperty {
public:
    virtual             ~IIndexProperty() = default;
public:
    virtual int         AttrCount() const = 0;
    virtual const char *Attr(int attrnum) const = 0;
    virtual const char *CategName(const char *attr, TCateg catid) const = 0;
    virtual TCateg      CategParent(const char *attr, TCateg catid) const = 0;
    virtual bool        CategIsLink(const char *attr, TCateg parent, TCateg link) const = 0;
    virtual time_t      IndexModTime() const = 0;
    virtual const char *ImagesUrl() const = 0;
    virtual size_t      IndProperty(const char *prop, char* buf, size_t size) const = 0;
    virtual const char *ConfigParam(const char* directive) const = 0;

    template <class T>
    inline void IndPropertySafe(const char* prop, T& t) const {
        if (IndProperty(prop, (char*)&t, sizeof(t)) != sizeof(t)) {
            abort();
        }
    }
};

class IReqResults {
protected:
    virtual             ~IReqResults() = default;
public:
    virtual const char *Request() = 0;
    virtual const char *ErrorText() = 0;
    virtual int         ErrorCode(unsigned sourcenum = 0) const = 0;
    virtual int         SerpErrorCode() const = 0;
    virtual const char *ReqId() const = 0;
    virtual unsigned    WizardRuleNameCount() const = 0;
    virtual const char *WizardRuleName(unsigned nRule) const = 0;
    virtual unsigned    WizardRulePropertyNameCount(const char* RuleName) const = 0;
    virtual const char *WizardRulePropertyName(const char* RuleName, unsigned nProperty) const = 0;
    virtual unsigned    WizardRulePropertyCount(const char* RuleName, const char* PropertyName) const = 0;
    virtual const char *WizardRuleProperty(const char* RuleName, const char* PropertyName, unsigned Index) const = 0;
};

class ICluster {
protected:
    virtual             ~ICluster() = default;
public:
    virtual TCateg      CurCateg(const TGroupingIndex& gi) const = 0;
    virtual TStringBuf  CurCategStr(const TGroupingIndex& gi) = 0;
    virtual int         GroupsOnPageCount(const TGroupingIndex& gi) const = 0;

    virtual int         DocsInGroupCount(const TGroupingIndex& gi) const = 0;

    virtual int         DocsInHeadGroupsCount(const TGroupingIndex& gi) const = 0;

    virtual ui64        TotalDocCount(int prior) = 0;

    virtual int         GroupingSize(const TGroupingIndex& gi) = 0;

    virtual ui64 GroupingDocCount(int prior, const TGroupingIndex& gi) = 0;
    virtual ui64 GroupingGroupCount(int prior, const TGroupingIndex& gi) = 0;

    virtual int         HitCount() = 0;

    virtual TCateg      GroupCateg(int nGroup, const TGroupingIndex& gi) = 0;

    virtual TStringBuf  GroupCategStr(int nGroup, const TGroupingIndex& gi) = 0;

    virtual ui64 GroupDocCount(int prior, int nGroup, const TGroupingIndex& gi) = 0;

    virtual TRelevance  GroupRelevance(int nGroup, const TGroupingIndex& gi) = 0;

    virtual int         GroupPriority(int nGroup, const TGroupingIndex& gi) = 0;

    virtual size_t      GroupSize(int nGroup, const TGroupingIndex& gi) = 0;

    virtual bool HasDocInfo(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual int DocPriority(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual int DocInternalPriority(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual TRelevance DocRelevance(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual TMaybe<TRelevPredict> DocRelevPredict(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual ui32        DocHitCount(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;

    virtual const char* PassageBreaks(int group, int doc, const TGroupingIndex& gi) = 0;

    virtual int         DocCategCount(int nGroup, int nDoc, const TGroupingIndex& gi, const char* facet) = 0;
    virtual TCateg      DocCategId(int nGroup, int nDoc, const TGroupingIndex& gi, const char* facet, unsigned index) = 0;

    virtual int         ReqAttrFunc(const char *func) const = 0;

    virtual unsigned    GetSearchPropertyCount(unsigned clientNum) const = 0;
    virtual TStringBuf  GetSearchPropertyName(unsigned clientNum, unsigned propNum) const = 0;
    virtual TStringBuf  GetSearchPropertyValue(unsigned clientNum, const char* name) const = 0;

    virtual unsigned    GetMainSearchPropertyCount() const = 0;
    virtual TStringBuf  GetMainSearchPropertyName(unsigned propNum) const = 0;
    virtual TStringBuf  GetMainSearchPropertyValue(const char* name) const = 0;
};

class IFactorSerializerFactory {
public:
    virtual ~IFactorSerializerFactory() = default;
    virtual IDocFactorSerializer* GetAttributeSerializer(const char* factorName) = 0;
};

class IAttributeWriter {
public:
    virtual void operator()(TStringBuf name, TStringBuf value) = 0;
    virtual void operator()(TStringBuf name, float value) = 0;
    virtual ~IAttributeWriter() = default;
};

class TDocHandle;
class TDocRoute;

class IArchiveDocInfo {
public:
    virtual ~IArchiveDocInfo() = default;
    virtual TString DocTitle() const = 0;
    virtual TString DocHeadline() const = 0;
    virtual int         DocPassageCount() const = 0;
    virtual TString DocPassage(int nPassage) const = 0;
    virtual bool DocPassageAttrs(int nPassage, TString* outValue) const = 0;

    virtual void SerializeFirstStageAttributes(TArrayRef<const char * const> attrNames, IAttributeWriter& write) const;
    virtual void SerializeAttributes(TArrayRef<const char * const> attrNames, IAttributeWriter& write) const;

    virtual TString     DocUrl(int nUrl) const = 0;
    virtual long        DocSize(int nUrl) const = 0;
    virtual TString DocCharset(int nUrl) const = 0;
    virtual time_t      DocMTime(int nUrl) const = 0;

    virtual TDocHandle  DocHandle() const = 0;
    virtual int         DocPropertyCount(const char *property) const = 0;
    virtual bool ReadDocProperty(TStringBuf property, ui32 index, TString* out_value) const = 0;
    virtual int         DocPropertyNameCount() const = 0;
    virtual TString DocPropertyName(unsigned index) const = 0;
    virtual TString DocServerDescr() const { return {}; }

    virtual ui32 DocIndexGeneration() const = 0;
    virtual ui32 DocSourceTimestamp() const = 0;
    virtual bool SerializeDocFactor(IDocFactorSerializer* serializer, IAttributeWriter& write) const = 0;
    virtual size_t GetAllFactors(float* /*output*/, size_t /*maxFactorCount*/) const { abort(); }
    virtual TString GetBinaryField(size_t /*field*/) const { abort(); }

protected:
    virtual void SerializeFirstStageAttribute(const char *attrName, IAttributeWriter& write) const;
    virtual void SerializeAttribute(const char *attrName, IAttributeWriter& write) const;
};

// Allow only read-only operations relative to client form field
class IClientRequestBaseAdjuster {
public:
    virtual             ~IClientRequestBaseAdjuster() = default;
    virtual bool        ClientFormFieldHas(TStringBuf, TStringBuf) = 0;
    virtual bool        IsClientEphemeral() const = 0;
    virtual const TString& ClientDescr() const = 0;
    virtual const TString& ClientFormField(TStringBuf, int) = 0;
    virtual int         ClientFormFieldCount(TStringBuf) = 0;
    virtual void        ClientDontSendRequest() = 0;
    virtual void        ClientEnableSendRequest() {}
    virtual void        ClientSetOnlyExplicitGroupings() = 0;
};

class IClientRequestAdjuster
    : public virtual IClientRequestBaseAdjuster
{
public:
    ~IClientRequestAdjuster() override = default;
    virtual void        ClientFormFieldInsert(TStringBuf, TStringBuf) = 0;
    virtual void        ClientFormFieldRemove(TStringBuf, int) = 0;
    virtual void        ClientFormFieldRemoveAll() = 0;
    virtual void        AskFactor(TStringBuf name) = 0;
};

namespace NReportStatistics {
    struct TReportStatistics {
        ui64 ReportByteSize = 0;
        ui64 ReportDocsCount = 0;
        ui64 TotalDocsCount = 0;
    };
}

class IReqEnv {
public:
    using TReportStatistics = NReportStatistics::TReportStatistics;
protected:
    virtual             ~IReqEnv()  = default;
public:
    // IHttpRequesterData
    virtual int         FormFieldTest(TStringBuf key, TStringBuf value) const = 0;
    virtual int         FormFieldCount(TStringBuf key) const = 0;
    virtual const char *FormField(TStringBuf key, int num) = 0;
    virtual void        FormFieldInsert(TStringBuf k, TStringBuf v) = 0;
    virtual void        FormFieldRemove(TStringBuf k, int n) = 0;
    virtual const char *Environment(const char* key) = 0;
    virtual const TString *HeaderIn(TStringBuf key) = 0;
    TStringBuf HeaderInOrEmpty(TStringBuf key) {
        const auto* ptr = HeaderIn(key);
        return ptr ? TStringBuf{*ptr} : TStringBuf{};
    }
    virtual size_t      HeadersCount() = 0;
    virtual const char *HeaderByIndex(size_t num) = 0;

    virtual IClientRequestAdjuster* GetClientRequestAdjuster(const char* clientDescr) = 0;
    virtual const char *QueryString() = 0;
    virtual const char *SearchUrl() = 0;
    virtual const char *SearchPageUrl(int npage, const char *attr) = 0;
    virtual const char *HighlightedDocUrl(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;
    virtual const char *HitReportUrl() = 0;
    virtual void        PrintS(const char *, size_t length) = 0;
    virtual void        Print(const google::protobuf::Message&) = 0;
    virtual const char* ConvertArchiveText(const char* rawtext, unsigned maxLen, int mode,
                   const char *Open1, const char *Open2, const char *Open3,
                   const char *Close1, const char *Close2, const char *Close3) = 0;

    virtual const TString& ClientDocServerDescr(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;
    virtual const char *ClientDocServerIcon(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;
    virtual const char *ClientStat() = 0;
    virtual int         ClientBadCount() = 0;
    virtual size_t      GetClientsCount() const = 0;
    virtual size_t      BaseSearchCount() const = 0;
    virtual size_t      BaseSearchNotRespondCount() const = 0;
    virtual size_t      NotRespondedSourcesCount() const = 0;
    virtual size_t      FailedPrimusCount() const = 0;
    virtual const char* NotRespondedSourceName(size_t idx) const = 0;
    virtual const char* FailedPrimus(size_t idx) const = 0;
    virtual int         NotRespondedClientsCount(const char* clientDescr) const = 0;
    virtual int         IncompleteClientsCount(const char* clientDescr) const = 0;
    virtual int         NotGatheredBaseSearchAnswers(const char* clientDescr) const = 0;
    virtual size_t      ChildrenSuperMindsCount() const = 0;
    virtual void        ChildSuperMind(size_t idx, TDocRoute& shard, float& multiplier, bool& isUnanswer) const = 0;

    // Best before next call to GetArchiveAccessor
    virtual const IArchiveDocInfo* GetArchiveAccessor(int nGroup, int nDoc, const TGroupingIndex& gi) = 0;
    virtual ui64 RequestBeginTime() const = 0;
    virtual IFactorSerializerFactory* GetFactorSerializerFactory() {
        return nullptr;
    }

    virtual TReportStatistics GetReportStats() const {
        return {};
    }
    virtual void SaveReportStats(const TReportStatistics&) {}
};

class IInfoContext {
    public:
        virtual ~IInfoContext() = default;
    public:
        virtual int CalcInfo() = 0;
        virtual TBufferStream& GetInfoDataStream() = 0;
};

class IPassageContext {
    public:
        using TReportStatistics = NReportStatistics::TReportStatistics;
        virtual ~IPassageContext() = default;
    public:
        virtual int FindPassages(const TMakePageContext&) = 0;

        virtual TReportStatistics GetReportStats() const {
            return {};
        }
        virtual void SaveReportStats(const TReportStatistics&) {}
};

class IFetchDocDataContext {
public:
    virtual ~IFetchDocDataContext() = default;

    virtual int FetchDocData(const TMakePageContext&) = 0;
};

class IFetchAttrsContext {
public:
    virtual ~IFetchAttrsContext() = default;
    virtual int FetchAttrs(const TMakePageContext&) = 0;
};

class ISearchContext {
protected:
    virtual ~ISearchContext() = default;
public:
    virtual const IIndexProperty*   IndexProperty() = 0;
    virtual IReqParams*             ReqParams() = 0;
    virtual IReqResults*            ReqResults() = 0;
    virtual ICluster*               Cluster() = 0;
    virtual IReqEnv*                ReqEnv() = 0;
};

struct TSimpleBlob {
    const char* data;
    size_t size;
};

class IRemoteRequestResult {
protected:
    virtual ~IRemoteRequestResult() = default;
public:
    virtual size_t StatusCode() const = 0;
    virtual size_t HeaderCount() const = 0;
    virtual const char* HeaderName(size_t index) const = 0;
    virtual const char* HeaderValueByIndex(size_t index) const = 0;
    virtual const char* HeaderValue(const char* headerName) const = 0;
    virtual const TSimpleBlob Content() const = 0;
        // yes, by value - it's basically a TString
};

class IRemoteRequester {
public:
    virtual ~IRemoteRequester() = default;
    virtual void SetReconnectTimeout(size_t timeout) = 0;
    virtual int AddRequestUrl(const char* url, size_t timeout) = 0;
    virtual int AddRequest(const char* host, ui16 port, const char* path, size_t timeout) = 0;
    virtual void AddRequestHeader(int index, const char* name, const char* value) = 0;
    virtual void SetRequestBody(int index, const TBlob& body) = 0;
    virtual const IRemoteRequestResult* GetRequestResult(int index) const = 0;
    virtual bool NeedWaitAll() const = 0;
    virtual void SetNeedWaitAll(bool val) = 0;
    virtual void Start() = 0;
    virtual void Join() = 0;
};
