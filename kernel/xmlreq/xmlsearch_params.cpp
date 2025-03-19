#include "xmlsearch_params.h"

#include <library/cpp/xml/sax/sax.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/stack.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace {
    enum ENode {
        EN_UNKNOWN,
        EN_ROOT,
        EN_QUERY,
        EN_PAGE,
        EN_SORTBY,
        EN_GROUPINGS,
        EN_GROUPBY,
        EN_MAX_PASSAGES,
        EN_MAX_PASSAGE_LENGTH,
        EN_MAX_HEADLINE_LENGTH,
        EN_MAX_TEXT_LENGTH,
        EN_MAX_TITLE_LENGTH,
        EN_REQID,
        EN_NOCACHE,
        EN_GROUP_ATTR,
        EN_GROUP_MODE,
        EN_GROUP_GROUPS_ON_PAGE,
        EN_GROUP_DOCS_IN_GROUP,
        EN_GROUP_CURCATEG,
        EN_GROUP_DEPTH,
        EN_GROUP_KILLDUP,
    };

    class TNodeMap {
    public:
        TNodeMap() {
            Map["query"] = EN_QUERY;
            Map["sortby"] = EN_SORTBY;
            Map["page"] = EN_PAGE;
            Map["groupings"] = EN_GROUPINGS;
            Map["groupby"] = EN_GROUPBY;
            Map["max-passages"] = EN_MAX_PASSAGES;
            Map["max-headline-length"] = EN_MAX_HEADLINE_LENGTH;
            Map["max-passage-length"] = EN_MAX_PASSAGE_LENGTH;
            Map["max-title-length"] = EN_MAX_TITLE_LENGTH;
            Map["max-text-length"] = EN_MAX_TEXT_LENGTH;
            Map["reqid"] = EN_REQID;
            Map["nocache"] = EN_NOCACHE;

            Map["attr"] = EN_GROUP_ATTR;
            Map["mode"] = EN_GROUP_MODE;
            Map["groups-on-page"] = EN_GROUP_GROUPS_ON_PAGE;
            Map["docs-in-group"] = EN_GROUP_DOCS_IN_GROUP;
            Map["curcateg"] = EN_GROUP_CURCATEG;
            Map["depth"] = EN_GROUP_DEPTH;
            Map["killdup"] = EN_GROUP_KILLDUP;
        }

        ENode GetNode(const TString& nodeString) const {
            TIterator  it = Map.find(nodeString);

            if (it != Map.end())
                return it->second;
            else
                return EN_UNKNOWN;
        }

        static inline TNodeMap* GetInstance() {
            return Singleton<TNodeMap>();
        }
    private:
        typedef THashMap<TCiString, ENode> TMapType;
        typedef TMapType::const_iterator TIterator;

        TMapType Map;
    };

    class TSaxHandler : public NXml::ISaxHandler {
    private:

        bool Valid;
        TXmlSearchRequest Result;
        TStack<ENode> NodeStack;

    private:
        static void FillGroupBy(const char** attrs, TXmlSearchRequest::TGroupBy& groupBy) {
            for (const char** p = attrs; *p; ) {
                TString attrName;
                TString attrValue;

                attrName = *p++;
                if (*p)
                    attrValue = *p;
                p++;

                switch (TNodeMap::GetInstance()->GetNode(attrName)) {
                    case EN_GROUP_ATTR:
                        groupBy.Attr = attrValue;
                        break;

                    case EN_GROUP_MODE:
                        groupBy.Mode = attrValue;
                        break;

                    case EN_GROUP_GROUPS_ON_PAGE:
                        groupBy.GroupsOnPage = attrValue;
                        break;

                    case EN_GROUP_DOCS_IN_GROUP:
                        groupBy.DocsInGroup = attrValue;
                        break;

                    case EN_GROUP_CURCATEG:
                        groupBy.CurCateg = attrValue;
                        break;

                    case EN_GROUP_DEPTH:
                        groupBy.Depth = attrValue;
                        break;

                    case EN_GROUP_KILLDUP:
                        groupBy.KillDup = attrValue;
                        break;

                    default:
                        break;
                }
            }
        }

    protected:
        void OnStartDocument() override {
            Valid = true;
            NodeStack.push(EN_ROOT);
        }

        void OnEndDocument() override {
            NodeStack.pop();
        }

        void OnStartElement(const char* name, const char** attrs) override {
            ENode node = TNodeMap::GetInstance()->GetNode(name);

            if (NodeStack.top() == EN_GROUPINGS && node == EN_GROUPBY) {
                Result.Groupings.push_back(TXmlSearchRequest::TGroupBy());
                TXmlSearchRequest::TGroupBy& grp = Result.Groupings.back();

                FillGroupBy(attrs, grp);
            }

            if (node == EN_NOCACHE)
                Result.NoCache = true;

            NodeStack.push(node);
        }

        void OnEndElement(const char* /*name*/) override {
            NodeStack.pop();
        }

        void OnText(const char* text, size_t len) override {
            switch (NodeStack.top()) {
                case EN_QUERY:
                    Result.Query += TString(text, len);
                    break;

                case EN_PAGE:
                    Result.Page += TString(text, len);
                    break;

                case EN_SORTBY:
                    Result.SortBy += TString(text, len);
                    break;

                case EN_MAX_PASSAGES:
                    Result.MaxPassages += TString(text, len);
                    break;

                case EN_MAX_PASSAGE_LENGTH:
                    Result.MaxPassageLength += TString(text, len);
                    break;

                case EN_MAX_TEXT_LENGTH:
                    Result.MaxTextLength += TString(text, len);
                    break;

                case EN_MAX_HEADLINE_LENGTH:
                    Result.MaxHeadlineLength += TString(text, len);
                    break;

                case EN_MAX_TITLE_LENGTH:
                    Result.MaxTitleLength += TString(text, len);
                    break;

                case EN_REQID:
                    Result.ReqId += TString(text, len);
                    break;

                default:
                    break;
            }
        }

        void OnWarning(const TErrorInfo& /*ei*/) override {}

        void OnError(const TErrorInfo& ei) override {
            Valid = false;
            ythrow yexception() << ei.Message;
        }

        void OnFatalError(const TErrorInfo& ei) override {
            Valid = false;
            throw yexception() << ei.Message;
        }

    public:
        TSaxHandler()
            : Valid(true)
        {
        }

        bool GetResult(TXmlSearchRequest& result) const {
            if (Valid) {
                result = Result;
                return true;
            } else
                return false;
        }

        bool IsValid() const {
            return Valid;
        }
    };
}

TXmlSearchRequest::TXmlSearchRequest()
    : NoCache(false)
{
}

TString TXmlSearchRequest::ToCgiString() const {
    TString result;
    TStringOutput output(result);

    TString queryEscaped = Query;
    CGIEscape(queryEscaped);
    output << "query=" << queryEscaped;
    if (!Page.empty())
        output << "&page=" << Page;

    if (!SortBy.empty())
        output << "&sortby=" << SortBy;

    if (!MaxPassages.empty())
        output << "&max-passages=" << MaxPassages;

    if (!MaxPassageLength.empty())
        output << "&max-passage-length=" << MaxPassageLength;

    if (!MaxTitleLength.empty())
        output << "&max-title-length=" << MaxTitleLength;

    if (!MaxHeadlineLength.empty())
        output << "&max-headline-length=" << MaxHeadlineLength;

    if (!MaxTextLength.empty())
        output << "&max-text-length=" << MaxTextLength;

    if (!ReqId.empty())
        output << "&reqid=" << ReqId;

    if (NoCache)
        output << "&nocache=";

    for (size_t i = 0; i < Groupings.size(); i++) {
        output << "&groupby=";
        const TXmlSearchRequest::TGroupBy& grp = Groupings[i];
        if (!grp.Attr.empty())
            output << "attr%3D" << grp.Attr << '.';

        if (!grp.Mode.empty())
            output << "mode%3D" << grp.Mode << '.';

        if (!grp.GroupsOnPage.empty())
            output << "groups-on-page%3D" << grp.GroupsOnPage << '.';

        if (!grp.DocsInGroup.empty())
            output << "docs-in-group%3D" << grp.DocsInGroup << '.';

        if (!grp.CurCateg.empty())
            output << "curcateg%3D" << grp.CurCateg << '.';

        if (!grp.Depth.empty())
            output << "depth%3D" << grp.Depth << '.';

        if (!grp.KillDup.empty())
            output << "killdup%3D" <<  grp.KillDup << '.';
    }

    return result;
}

bool ParseXmlSearch(IInputStream& input, TXmlSearchRequest & result) {
    TSaxHandler handler;
    try {
        NXml::ParseXml(&input, &handler);

        handler.GetResult(result);

        return handler.IsValid();
    } catch (yexception& ex) {
        Cerr << ex.what() << Endl;
        return false;
    }
}

