#include "qd_document_responses.h"

#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/urlid/url2docid.h>

#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/generic/maybe.h>
#include <library/cpp/string_utils/url/url.h>

#include <utility>

namespace NQueryData {

    static bool Contains(EKeyType keyType, EKeyType kt) {
        return keyType == kt;
    }

    static bool Contains(const std::initializer_list<EKeyType>& keyTypes, EKeyType kt) {
        return IsIn(keyTypes, kt);
    }

    static TStringBuf DoFixUrlKey(TStringBuf url) {
        return CutHttpPrefix(url, true);
    }

    static TStringBuf DoFixCategUrlKey(TStringBuf categUrl) {
        return DoFixUrlKey(categUrl.After(' '));
    }

    template <class TKeyTypes>
    static TMaybe<TStringBuf> DoGetSubkey(const TKeyTypes& keyTypes, const TSourceFactors& sf) {
        if (Contains(keyTypes, sf.GetSourceKeyType())) {
            return TStringBuf(sf.GetSourceKey());
        }

        for (const auto& sk : sf.GetSourceSubkeys()) {
            if (Contains(keyTypes, sk.GetType())) {
                return TStringBuf(sk.GetKey());
            }
        }

        return Nothing();
    }

    static auto DoGetOwner(const TSourceFactors& sf) {
        return DoGetSubkey(std::initializer_list<EKeyType>{KT_CATEG, KT_SNIPCATEG}, sf);
    }

    static TMaybe<TStringBuf> DoGetUrl(const TSourceFactors& sf) {
        if (auto url = DoGetSubkey(KT_URL, sf)) {
            return DoFixUrlKey(*url);
        } else if (auto categUrl = DoGetSubkey(std::initializer_list<EKeyType>{KT_CATEG_URL, KT_SNIPCATEG_URL}, sf)) {
            return DoFixCategUrlKey(*categUrl);
        } else {
            return Nothing();
        }
    }

    static auto DoGetDocId(const TSourceFactors& sf) {
        return DoGetSubkey(std::initializer_list<EKeyType>{KT_DOCID, KT_SNIPDOCID}, sf);
    }

    bool ResponseMatchesDocument(const TSourceFactors& sf, const TDocView& d) {
        if (d.Owner) {
            if (auto match = DoGetOwner(sf)) {
                return d.Owner == *match;
            }
        }
        if (d.Url) {
            if (auto match = DoGetUrl(sf)) {
                return DoFixUrlKey(d.Url) == *match;
            }
        }
        if (d.DocId) {
            if (auto match = DoGetDocId(sf)) {
                return d.DocId == *match;
            }
        }
        return false;
    }

    static void DoApplyProc(IDocumentResponseProcessor& proc, const TSourceFactors& sf) {
        if (auto owner = DoGetOwner(sf)) {
            if (*owner) {
                proc.OnOwner(*owner, sf);
            }
        } else if (auto url = DoGetUrl(sf)) {
            if (*url) {
                proc.OnUrl(*url, sf);
            }
        } else if (auto docId = DoGetDocId(sf)) {
            if (*docId) {
                proc.OnDocId(*docId, sf);
            }
        }
    }

    void ProcessDocumentResponses(IDocumentResponseProcessor& proc, const TQueryData& qd, bool needSorting) {
        if (needSorting) {
            TVector<std::pair<ui64, const TSourceFactors*>> buf;
            buf.reserve(qd.SourceFactorsSize());

            for (const auto& sf : qd.GetSourceFactors()) {
                buf.emplace_back(sf.GetVersion(), &sf);
            }

            Sort(buf);

            for (const auto& vsf : buf) {
                DoApplyProc(proc, *vsf.second);
            }
        } else {
            for (const auto& sf : qd.GetSourceFactors()) {
                DoApplyProc(proc, sf);
            }
        }
    }

    void TUrl2DocIdResponseProcessor::OnUrl(TStringBuf url, const TSourceFactors& sf) {
        OnDocId(Url2DocId(url, Url2DocIdBuf), sf);
    }

    enum EOnItemResponseType {
        OIRT_NONE = 0, OIRT_URL = 1, OIRT_DOC_ID = 2, OIRT_OWNER = 3
    };

    class TOnItemResponseWrapper : public TUrl2DocIdResponseProcessor {
        TOnItemResponse Proc;
        EOnItemResponseType ResponseType = OIRT_NONE;

    public:
        TOnItemResponseWrapper(TOnItemResponse proc, EOnItemResponseType rt)
            : Proc(proc)
            , ResponseType(rt)
        {}

        void OnUrl(TStringBuf url, const TSourceFactors& sf) override {
            if (OIRT_URL == ResponseType) {
                Proc(url, sf);
            } else if (OIRT_DOC_ID == ResponseType) {
                TUrl2DocIdResponseProcessor::OnUrl(url, sf);
            }
        }

        void OnDocId(TStringBuf docId, const TSourceFactors& sf) override {
            if (OIRT_DOC_ID == ResponseType) {
                Proc(docId, sf);
            }
        }

        void OnOwner(TStringBuf owner, const TSourceFactors& sf) override {
            if (OIRT_OWNER == ResponseType) {
                Proc(owner, sf);
            }
        }
    };

    void ProcessUrlResponses(TOnItemResponse onItemResponse, const TQueryData& qd, bool needSorting) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_URL);
        ProcessDocumentResponses(wr, qd, needSorting);
    }
    void ProcessUrlResponses(TOnItemResponse onItemResponse, const TSourceFactors& sf) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_URL);
        DoApplyProc(wr, sf);
    }

    void ProcessDocIdResponses(TOnItemResponse onItemResponse, const TQueryData& qd, bool needSorting) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_DOC_ID);
        ProcessDocumentResponses(wr, qd, needSorting);
    }
    void ProcessDocIdResponses(TOnItemResponse onItemResponse, const TSourceFactors& sf) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_DOC_ID);
        DoApplyProc(wr, sf);
    }

    void ProcessOwnerResponses(TOnItemResponse onItemResponse, const TQueryData& qd, bool needSorting) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_OWNER);
        ProcessDocumentResponses(wr, qd, needSorting);
    }
    void ProcessOwnerResponses(TOnItemResponse onItemResponse, const TSourceFactors& sf) {
        TOnItemResponseWrapper wr(onItemResponse, OIRT_OWNER);
        DoApplyProc(wr, sf);
    }

}
