#include "qd_cgi.h"
#include "qd_docitems.h"

#include <kernel/querydata/idl/scheme/querydata_request.sc.h>

#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/builder.h>

#include <utility>

namespace NQueryData {

    void IRequestBuilder::FormRequestsJson(TVector<TString>& res, const NSc::TValue& data) const {
        THolder<TRequestRec> rec = TRequestRec::FromJson(data);
        FormRequests(res, *rec);
    }

    void IRequestBuilder::ParseFormedRequestJson(NSc::TValue& val, const TCgiParameters& cgi) const {
        TRequestRec rec;
        ParseFormedRequest(rec, cgi);
        val.MergeUpdate(rec.ToJson());
    }

    static ui32 GetNameLength(TStringBuf name, ui32 count) {
        return count ? name.size() + 3 : 0;
    }

    void TQuerySearchRequestBuilder::FormRequests(TVector<TString>& vs, const TRequestRec& data) const {
        vs.clear();
        TString req;

        SetCgiRoot(req, CGI_TEXT,        data.UserQuery);
        SetCgiRoot(req, CGI_REQ_ID,      data.ReqId);
        SetCgiRoot(req, CGI_YANDEX_TLD,  data.YandexTLD);
        SetCgiRoot(req, CGI_NOQTREE,     TString("1"));

    // START relev
        req.append("relev=");
        SetCgiSub(req, CGI_UI_LANG,     data.UILang);
        SetCgiSub(req, CGI_STRONG_NORM, data.StrongNorm);
        SetCgiSub(req, CGI_DOPPEL_NORM, data.DoppelNorm);
        SetCgiSub(req, CGI_DOPPEL_NORM_W, data.DoppelNormW);

        if (!Options.AlwaysUseRelev) {
            req.append("&");
            // END relev
            // START rearr
            req.append("rearr=");
        }

        SetCgiSub(req, CGI_FILTER_NAMESPACES, VectorToString(GetValues<TString>(data.FilterNamespaces), ','));
        SetCgiSub(req, CGI_FILTER_FILES,      VectorToString(GetValues<TString>(data.FilterFiles), ','));

        SetCgiSub(req, CGI_SKIP_NAMESPACES, VectorToString(GetValues<TString>(data.SkipNamespaces), ','));
        SetCgiSub(req, CGI_SKIP_FILES,      VectorToString(GetValues<TString>(data.SkipFiles), ','));

        if (data.IgnoreText) {
            SetCgiSub(req, CGI_IGNORE_TEXT, ToString((int)data.IgnoreText));
        }

        if (data.SerpType) {
            SetCgiSub(req, CGI_DEVICE, data.SerpType);
        }

        SetCgiSub(req, CGI_IS_MOBILE, ToString<int>(IsMobileSerpType(data.SerpType)));

        SetCgiSub(req, CGI_USER_ID,         data.UserId);
        SetCgiSub(req, CGI_USER_LOGIN,      data.UserLogin);
        SetCgiSub(req, CGI_USER_LOGIN_HASH, data.UserLoginHash);
        SetCgiSub(req, CGI_USER_REGIONS,             VectorToString(data.UserRegions, ','));
        SetCgiSub(req, CGI_USER_REGIONS_IPREG,       VectorToString(data.UserRegionsIpReg, ','));
        SetCgiSub(req, CGI_USER_REGIONS_IPREG_DEBUG, VectorToString(data.UserRegionsIpRegDebug, ','));
        SetCgiSub(req, CGI_USER_IP_MOBILE_OP,       data.UserIpMobileOp);
        SetCgiSub(req, CGI_USER_IP_MOBILE_OP_DEBUG, data.UserIpMobileOpDebug);

        if (!data.StructKeys.IsNull()) {
            SetCgiSub(req, CGI_STRUCT_KEYS,     JsonToString(data.StructKeys));
        }

        TMemoryPool memPool{1024};

        const auto& docItems = data.DocItems;

        const TStringBufs& docIds = Options.EnableUrls ? docItems.DocIds()
                                                       : docItems.AllDocIdsNoCopy(memPool, DIM_BANS);
        const TStringBufs& snipDocIds = Options.EnableUrls ? docItems.SnipDocIds()
                                                           : docItems.AllDocIdsNoCopy(memPool, DIM_SNIPS);
        const TStringBufs& categs = Options.EnableUrls ? docItems.Categs()
                                                       : docItems.AllCategsNoCopy(DIM_BANS);
        const TStringBufs& snipCategs = Options.EnableUrls ? docItems.SnipCategs()
                                                           : docItems.AllCategsNoCopy(DIM_SNIPS);
        TVector<TString> urls = Options.EnableUrls ? PairsToStrings(docItems.Urls())
                                              : TVector<TString>();
        TVector<TString> snipUrls = Options.EnableUrls ? PairsToStrings(docItems.SnipUrls())
                                                  : TVector<TString>();

        TRequestSplitMeasure measure;
        measure.SplittableCount = docIds.size()
                                + snipDocIds.size()
                                + categs.size()
                                + snipCategs.size()
                                + urls.size()
                                + snipUrls.size();

        measure.SharedSize = req.size() + GetNameLength(CGI_DOC_IDS, docIds.size())
                                        + GetNameLength(CGI_SNIP_DOC_IDS, snipDocIds.size())
                                        + GetNameLength(Options.EnableUrls ? CGI_CATEGS : CGI_URL_DEPRECATED, categs.size())
                                        + GetNameLength(CGI_SNIP_CATEGS, snipCategs.size())
                                        + GetNameLength(CGI_URLS, urls.size())
                                        + GetNameLength(CGI_SNIP_URLS, snipUrls.size());

        if (measure.SplittableCount) {
            measure.SplittableSize = CountSize(docIds)
                                   + CountSize(snipDocIds)
                                   + CountSize(categs)
                                   + CountSize(snipCategs)
                                   + CountSize(urls)
                                   + CountSize(snipUrls);

            measure.SharedSize = req.size();
            const ui32 nparts = CalculatePartsCount(measure, Limits);
            for (ui32 i = 0; i < nparts; ++i) {
                const TString& docIdsPack      = VectorToStringSplit(docIds, ',', i, nparts);
                const TString& snipDocIdsPack  = VectorToStringSplit(snipDocIds, ',', i, nparts);
                const TString& groupCategsPack = VectorToStringSplit(categs, ',', i, nparts);
                const TString& snipGroupCategsPack = VectorToStringSplit(snipCategs, ',', i, nparts);
                TString urlsPack = SplitDictStrings(urls, i, nparts);
                TString snipUrlsPack = SplitDictStrings(snipUrls, i, nparts);

                urlsPack = CompressCgi(urlsPack, Options.UrlsCompression);
                snipUrlsPack = CompressCgi(snipUrlsPack, Options.UrlsCompression);

                if (!docIdsPack && !snipDocIdsPack && !groupCategsPack && !snipGroupCategsPack && !urlsPack && !snipUrlsPack) {
                    continue;
                }

                vs.push_back(req);

                if (docIds) {
                    SetCgiSub(vs.back(), CGI_DOC_IDS, docIdsPack);
                }

                if (snipDocIds) {
                    SetCgiSub(vs.back(), CGI_SNIP_DOC_IDS, snipDocIdsPack);
                }

                if (categs) {
                    SetCgiSub(vs.back(), Options.EnableUrls ? CGI_CATEGS : CGI_URL_DEPRECATED, groupCategsPack);
                }

                if (snipCategs) {
                    SetCgiSub(vs.back(), CGI_SNIP_CATEGS, snipGroupCategsPack);
                }

                if (urls) {
                    SetCgiSub(vs.back(), FormCompressedCgiPropName(CGI_URLS, Options.UrlsCompression), urlsPack);
                }

                if (snipUrls) {
                    SetCgiSub(vs.back(), FormCompressedCgiPropName(CGI_SNIP_URLS, Options.UrlsCompression), snipUrlsPack);
                }

                vs.back().append('&');
                // END rearr
            }
        } else if (!Options.FormRequestsOnlyIfHasItems) {
            vs.resize(1, req);
        }
    }

    static void MergeJson(TRequestRec& req, TStringBuf json) {
        NSc::TValue reqJson;
        AdaptQueryDataRequest(reqJson, req);
        reqJson.MergeUpdateJson(json);
        AdaptQueryDataRequest(req, reqJson);
    }

    void TQuerySearchRequestBuilder::ParseFormedRequest(TRequestRec& req, const TCgiParameters& par) const {
        {
            TStringBuf device;
            bool isDesktop = false;
            bool isTablet = false;
            bool isTouch = false;
            bool isSmart = false;
            bool isMobile = false;

            for (const auto& transportCgi : std::initializer_list<TStringBuf>{"relev", "rearr"}) {
                for (const auto& rearrStr: par.Range(transportCgi)) {
                    TStringBuf rearr{rearrStr};
                    while (!!rearr) {
                        TStringBuf k, v;
                        v = rearr.NextTok(';');
                        k = v.NextTok('=');

                        if (CGI_IGNORE_TEXT == k) {
                            req.IgnoreText = IsTrue(v);
                        } else if (CGI_FILTER_NAMESPACES == k) {
                            InsertToSet(req.FilterNamespaces, Split<TStringBuf>(v, ','));
                        } else if (CGI_FILTER_FILES == k) {
                            InsertToSet(req.FilterFiles, Split<TStringBuf>(v, ','));
                        } else if (CGI_SKIP_NAMESPACES == k) {
                            InsertToSet(req.SkipNamespaces, Split<TStringBuf>(v, ','));
                        } else if (CGI_SKIP_FILES == k) {
                            InsertToSet(req.SkipFiles, Split<TStringBuf>(v, ','));
                        } else if (CGI_IS_DESKTOP == k) {
                            isDesktop = IsTrue(v);
                        } else if (CGI_IS_TABLET == k) {
                            isTablet = IsTrue(v);
                        } else if (CGI_IS_TOUCH == k) {
                            isTouch = IsTrue(v);
                        } else if (CGI_IS_SMART == k) {
                            isSmart = IsTrue(v);
                        } else if (CGI_IS_MOBILE == k) {
                            isMobile = IsTrue(v);
                        } else if (CGI_DEVICE == k) {
                            device = v;
                        } else if (CGI_USER_ID == k) {
                            req.UserId = v;
                        } else if (CGI_USER_LOGIN == k) {
                            req.UserLogin = v;
                        } else if (CGI_USER_LOGIN_HASH == k) {
                            req.UserLoginHash = v;
                        } else if (CGI_USER_IP_MOBILE_OP == k) {
                            req.UserIpMobileOp = v;
                        } else if (CGI_USER_IP_MOBILE_OP_DEBUG == k) {
                            req.UserIpMobileOpDebug = v;
                        } else if (CGI_USER_REGIONS == k) {
                            req.UserRegions = Split<TString>(v, ',');
                        } else if (CGI_USER_REGIONS_IPREG == k) {
                            req.UserRegionsIpReg = Split<TString>(v, ',');
                        } else if (CGI_USER_REGIONS_IP_COUNTRY == k) {
                            req.UserRegionsIpCountry = v;
                        } else if (CGI_USER_REGIONS_IPREG_DEBUG == k) {
                            req.UserRegionsIpRegDebug = Split<TString>(v, ',');
                        } else if (CGI_BEGEMOT_FAILED == k) {
                            req.BegemotFailed = IsTrue(v);
                        } else if (CGI_STRUCT_KEYS == k) {
                            req.StructKeys.MergeUpdateJson(v);
                        } else if (CGI_URL_DEPRECATED == k || CGI_CATEGS == k) {
                            req.DocItems.AddCategsAny(Split<TStringBuf>(v, ','), DIM_BANS);
                        } else if (CGI_SNIP_CATEGS == k) {
                            req.DocItems.AddCategsAny(Split<TStringBuf>(v, ','), DIM_SNIPS);
                        } else if (CGI_DOC_IDS == k) {
                            req.DocItems.AddDocIdsAny(Split<TStringBuf>(v, ','), DIM_BANS);
                        } else if (CGI_SNIP_DOC_IDS == k) {
                            req.DocItems.AddDocIdsAny(Split<TStringBuf>(v, ','), DIM_SNIPS);
                        } else if (k.StartsWith(CGI_URLS) || k.StartsWith(CGI_SNIP_URLS)) {
                            TStringBuf name;
                            ECgiCompression comprType;
                            if (ParseCompressedCgiPropName(name, comprType, k) != 1) {
                                continue;
                            }
                            TString tmp;
                            if (!DecompressCgi(tmp, v, comprType)) {
                                continue;
                            }
                            v = tmp;
                            auto json = NSc::TValue::FromJson(v);
                            req.DocItems.AddUrlsAny(json, k.StartsWith(CGI_SNIP_URLS) ? DIM_SNIPS : DIM_BANS);
                        } else if (CGI_QSREQ_JSON_ZLIB_1 == k || CGI_QSREQ_JSON_1 == k) {
                            MergeJson(req, DecompressCgi(v, CGI_QSREQ_JSON_ZLIB_1 == k ? CC_ZLIB : CC_PLAIN));
                        } else if (CGI_IS_RECOMMENDATIONS_REQUEST == k) {
                            req.IsRecommendationsRequest = true;
                        }
                    }
                }

                if (device) {
                    req.SerpType = device;
                } else if (isDesktop) {
                    req.SerpType = SERP_TYPE_DESKTOP;
                } else if (isTablet) {
                    req.SerpType = SERP_TYPE_TABLET;
                } else if (isTouch) {
                    req.SerpType = SERP_TYPE_TOUCH;
                } else if (isSmart) {
                    req.SerpType = SERP_TYPE_SMART;
                } else if (isMobile) {
                    req.SerpType = SERP_TYPE_MOBILE;
                }
            }
        }
        {
            for (const auto& relevStr: par.Range("relev")) {
                TStringBuf relev{relevStr};
                while (!!relev) {
                    TStringBuf k, v;
                    v = relev.NextTok(';');
                    k = v.NextTok('=');

                    if (CGI_DOPPEL_NORM == k) {
                        req.DoppelNorm = v;
                    } else if (CGI_DOPPEL_NORM_W == k) {
                        req.DoppelNormW = v;
                    } else if (CGI_STRONG_NORM == k) {
                        req.StrongNorm = v;
                    } else if (CGI_UI_LANG == k) {
                        req.UILang = v;
                    }
                }
            }
        }

        if (par.Has(CGI_IGNORE_TEXT)) {
            req.IgnoreText = IsTrue(par.Get(CGI_IGNORE_TEXT));
        }

        if (!GetCgiParameter(req.UserQuery, par, CGI_USER_REQUEST) && !req.IgnoreText) {
            GetCgiParameter(req.UserQuery, par, CGI_TEXT);
        }

        GetCgiParameter(req.YandexTLD, par, CGI_YANDEX_TLD);
        GetCgiParameter(req.ReqId, par, CGI_REQ_ID);
        GetCgiParameter(req.UILang, par, CGI_UI_LANG);

        for (ui32 i = 0, sz = par.NumOfValues(CGI_QSREQ_JSON_ZLIB_1); i < sz; ++i) {
            MergeJson(req, DecompressCgi(par.Get(CGI_QSREQ_JSON_ZLIB_1, i), CC_ZLIB));
        }

        for (ui32 i = 0, sz = par.NumOfValues(CGI_QSREQ_JSON_1); i < sz; ++i) {
            MergeJson(req, par.Get(CGI_QSREQ_JSON_1, i));
        }
    }

}
