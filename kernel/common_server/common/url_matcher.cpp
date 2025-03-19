#include "url_matcher.h"
#include <util/string/split.h>
#include <kernel/common_server/util/types/string_normal.h>

namespace NCS {
    bool TPathHandlerInfo::DeserializeFromString(const TString& pathPattern) {
        PathPattern = NCS::TStringNormalizer::TruncRet(pathPattern, '/');
        bool slashSeqStarted = false;
        for (auto&& c : pathPattern) {
            if (c == '/') {
                if (!slashSeqStarted && PathPatternCorrected.size()) {
                    slashSeqStarted = true;
                } else {
                    continue;
                }
            } else {
                slashSeqStarted = false;
            }
            PathPatternCorrected += c;
        }
        return true;
    }

    bool TPathHandlerInfo::Match(const TStringBuf req, ui32& templates, ui32& deep, TMap<TString, TString>& params) const {
        TStringBuf pCurrent;
        TStringBuf pFurther(PathPatternCorrected.data(), PathPatternCorrected.size());

        TStringBuf rCurrent;
        TStringBuf rFurther = req;
        NCS::TStringNormalizer::Trunc(rFurther, '/');
        deep = 0;
        templates = 0;
        while (true) {
            ++deep;
            TStringBuf fLocal;
            if (!pFurther.TrySplit('/', pCurrent, fLocal)) {
                pCurrent = pFurther;
                pFurther = TStringBuf();
            } else {
                pFurther = fLocal;
            }
            if (pCurrent.StartsWith("*")) {
                ++templates;
            } else if (pCurrent.StartsWith("$")) {
                TStringBuf l;
                TStringBuf r;
                if (pCurrent.TrySplit('=', l, r)) {
                    l.Skip(1);
                    params.emplace(l, r);
                    pCurrent = r;
                } else {
                    ++templates;
                    if (!pFurther) {
                        pCurrent.Skip(1);
                        params.emplace(pCurrent, rFurther);
                        for (auto&& c : rFurther) {
                            if (c == '/') {
                                ++templates;
                            }
                        }
                        return true;
                    }
                }
            }

            if (!rFurther.TrySplit('/', rCurrent, fLocal)) {
                rCurrent = rFurther;
                rFurther = TStringBuf();
            } else {
                rFurther = fLocal;
            }

            if (!pCurrent && !rCurrent) {
                return true;
            } else if (!pCurrent && !!rCurrent) {
                return false;
            } else if (!!pCurrent && !rCurrent) {
                return pCurrent == "*";
            } else {
                if (pCurrent == "*") {
                    return true;
                } else if (pCurrent.StartsWith("$")) {
                    params.emplace(pCurrent.substr(1), rCurrent);
                } else if (pCurrent != rCurrent) {
                    return false;
                }
            }
        }
        return true;
    }

    TVector<TPathSegmentInfo> TPathHandlerInfo::GetPatternInfo() const {
        TVector<NCS::TPathSegmentInfo> result;
        TVector<TString> pathSegments = StringSplitter(PathPatternCorrected).Split('/').SkipEmpty().ToList<TString>();
        for (auto&& i : pathSegments) {
            result.emplace_back(i);
        }
        return result;
    }

    TPathSegmentInfo::TPathSegmentInfo(const TString& pathSegment) {
        TStringBuf sb(pathSegment.data(), pathSegment.size());
        if (!sb.StartsWith('$')) {
            Value = pathSegment;
        } else {
            sb.Skip(1);
            TStringBuf l;
            TStringBuf r;
            if (!sb.TrySplit('=', l, r)) {
                Variable = sb;
            } else {
                Variable = l;
                Value = r;
            }
        }
    }

}
