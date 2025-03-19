#include "norm_query.h"
#include "norm_structkey.h"
#include "qd_genrequests.h"
#include "qd_subkeys_util.h"

#include <kernel/querydata/cgi/qd_cgi_utils.h>
#include <kernel/querydata/cgi/qd_docitems.h>
#include <kernel/querydata/idl/scheme/querydata_request.sc.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <kernel/urlid/url2docid.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NQueryData {

    static void Assign(TStringBufs& dst, const TVector<TString>& src) {
        dst.clear();
        for (const auto& s : src) {
            if (Y_LIKELY(s)) {
                dst.push_back(s);
            }
        }
    }


    bool FillSubkeysCache(TSubkeysCache& cache, TKeyTypes& keyTypes, const TRequestRec& request, TStringBuf nspace) {
        cache.ClearFakeSubkeys();

        if (keyTypes.empty()) {
            return false;
        }

        for (ui32 tnum = 0, sz = keyTypes.size(); tnum < sz; ++tnum) {
            int type = keyTypes[tnum];

            if (type <= FAKE_KT_COUNT || type >= KT_COUNT) {
                cache.ClearFakeSubkeys();
                return false;
            }

            if (KT_STRUCTKEY == type) {
                // get keys and intended usage
                type = keyTypes[tnum] = GetStructKeys(cache.GetSubkeysMutable(type), cache.StringPool(),
                                                             request.StructKeys, nspace);
            }

            TStringBufs& subkeys = cache.GetSubkeysMutable(type);

            if (subkeys.empty()) {
                switch (type) {
                case FAKE_KT_SOURCE_NAME: {
                    subkeys.push_back(nspace);
                    break;
                }

                case KT_BINARYKEY: {
                    Assign(subkeys, request.BinaryKeys);
                    break;
                }

                case KT_QUERY_EXACT: {
                    AssignNotEmpty<TStringBuf>(subkeys, request.UserQuery);
                    break;
                }

                case KT_QUERY_LOWERCASE: {
                    PushBackPooled(subkeys, cache.StringPool(), LowerCaseNormalization(request.UserQuery));
                    break;
                }

                case KT_QUERY_SIMPLE: {
                    PushBackPooled(subkeys, cache.StringPool(), SimpleNormalization(request.UserQuery));
                    break;
                }

                case KT_QUERY_STRONG: {
                    GetOrMakeNormalization(cache, KT_QUERY_STRONG, request.StrongNorm, NormalizeRequestUTF8Wrapper,
                                           request.UserQuery);
                    break;
                }

                case KT_QUERY_DOPPEL:
                case KT_QUERY_DOPPEL_TOKEN:
                case KT_QUERY_DOPPEL_PAIR: {
                    TStringBufs& doppel = GetOrMakeNormalization(cache, KT_QUERY_DOPPEL, request.DoppelNorm,
                                                                 DoppelNormalization, request.UserQuery);

                    if (!doppel.empty()) {
                        switch (type) {
                        default:
                            break;
                        case KT_QUERY_DOPPEL_TOKEN:
                            Tokenize(subkeys, doppel.back());
                            break;
                        case KT_QUERY_DOPPEL_PAIR:
                            GeneratePairs(subkeys, cache.StringPool(), doppel.back());
                            break;
                        }
                    }

                    break;
                }

                case KT_SERP_DEVICE: {
                    FillSerpDeviceTypeHierarchy(subkeys, request.SerpType);
                    break;
                }

                case KT_USER_IP_TYPE: {
                    FillUserIpOperatorTypeHierarchy(subkeys, request.GetUserIpMobileOp());
                    break;
                }

                case KT_SERP_TLD: {
                    AssignNotEmpty(subkeys, request.YandexTLD);
                    EnsureHierarchy(subkeys, GENERIC_ANY);
                    break;
                }

                case KT_SERP_UIL: {
                    AssignNotEmpty(subkeys, request.UILang);
                    EnsureHierarchy(subkeys, GENERIC_ANY);
                    break;
                }

                case KT_CATEG: {
                    subkeys = request.DocItems.AllCategsNoCopy(DIM_ALL);
                    break;
                }

                case KT_SNIPCATEG: {
                    subkeys = request.DocItems.AllCategsNoCopy(DIM_SNIPS);
                    break;
                }

                case KT_DOCID: {
                    subkeys = request.DocItems.AllDocIdsNoCopy(cache.MemoryPool(), DIM_ALL);
                    break;
                }

                case KT_SNIPDOCID: {
                    subkeys = request.DocItems.AllDocIdsNoCopy(cache.MemoryPool(), DIM_SNIPS);
                    break;
                }

                case KT_URL: {
                    subkeys = request.DocItems.AllUrlsNoCopy();
                    break;
                }

                case KT_CATEG_URL: {
                    subkeys = request.DocItems.AllCategUrlsNoCopy(cache.MemoryPool(), DIM_ALL);
                    break;
                }

                case KT_SNIPCATEG_URL: {
                    subkeys = request.DocItems.AllCategUrlsNoCopy(cache.MemoryPool(), DIM_SNIPS);
                    break;
                }

                case FAKE_KT_STRUCTKEY_ANY:
                case FAKE_KT_STRUCTKEY_ORDERED: {
                    // nothing to do here
                    break;
                }

                case KT_USER_REGION:
                case KT_USER_REGION_IPREG: {
                    if (KT_USER_REGION_IPREG == type) {
                        AssignNotEmpty(subkeys, request.GetUserRegionsIpReg());
                    } else {
                        AssignNotEmpty(subkeys, request.UserRegions);
                    }

                    EnsureHierarchy(subkeys, REGION_ANY);
                    break;
                }

                case KT_USER_ID: {
                    AssignNotEmpty(subkeys, request.UserId);
                    break;
                }

                case KT_USER_LOGIN: {
                    AssignNotEmpty(subkeys, request.UserLogin);
                    break;
                }

                case KT_USER_LOGIN_HASH: {
                    AssignNotEmpty(subkeys, request.UserLoginHash);
                    break;
                }
                }
            }
        }

        return true;
    }

    bool NormalizeQuery(TRawKeys& normquery, const TRequestRec& request, const TStringBufs& types, TStringBuf nspace) {
        normquery.KeyTypes.clear();
        normquery.KeyTypes.reserve(types.size());
        for (const auto& name : types) {
            normquery.KeyTypes.push_back(GetNormalizationTypeFromName(name));
        }
        return NormalizeQuery(normquery, request, nspace);
    }

    bool NormalizeQuery(TRawKeys& k, const TRequestRec& r, TStringBuf nspace) {
        bool res = FillSubkeysCache(k, k.KeyTypes, r, nspace);
        k.GenerateTuples(false);
        return res;
    }
}
