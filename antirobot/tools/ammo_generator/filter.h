#pragma once

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/regex/pcre/regexp.h>

#include <functional>

typedef std::function<bool(TStringBuf response, const TVector<TString>& requestChunks)> TAmmoFilter;

namespace {
    unsigned short GetHttpResponseCode(TStringBuf response) {
        const TStringBuf prefix = "HTTP/1.1 ";
        if (!response.StartsWith(prefix)) {
            return 0;
        }

        response = response.Skip(prefix.length());

        unsigned int responseCode = 0;
        for (char c : response) {
            if ('0' <= c && c <= '9') {
                responseCode = responseCode * 10 + (c - '0');
            } else {
                break;
            }
        }

        return static_cast<unsigned short>(responseCode);
    }

    size_t GetRequestLength(const TVector<TString>& requestChunks) {
        size_t requestLength = 0;
        for (const TString& chunk : requestChunks) {
            requestLength += chunk.size();
        }
        return requestLength;
    }

    TAmmoFilter CreateResponseCodeFilter(unsigned short httpCode) {
        return [httpCode](TStringBuf response, const TVector<TString>&) {
            return httpCode == 0 || GetHttpResponseCode(response) == httpCode;
        };
    }

    TAmmoFilter CreateRequestSizeFilter(size_t minimumRequestLength) {
        return [minimumRequestLength](TStringBuf, const TVector<TString>& requestChunks) {
            return minimumRequestLength == 0 || GetRequestLength(requestChunks) >= minimumRequestLength;
        };
    }

    TAmmoFilter CreateRequestRegExpFilter(const TString& requestRegExp) {
        if (!requestRegExp) {
            return [](TStringBuf, const TVector<TString>&) {
                return true;
            };
        }
        TRegExMatch re(requestRegExp);
        return [re](TStringBuf, const TVector<TString>& requestChunks) {
            TString fullRequest;
            for (const auto& chunk : requestChunks) {
                fullRequest += chunk;
            }
            return re.Match(fullRequest.c_str());
        };
    }
}

TMaybe<TAmmoFilter> CreateAmmoFilter(unsigned short httpCode, size_t minimumRequestSize, const TString& requestRegExp) {
    if (httpCode == 0 && minimumRequestSize == 0 && !requestRegExp) {
        return Nothing();
    }

    auto requestSizeFilter = CreateRequestSizeFilter(minimumRequestSize);
    auto responseCodeFilter = CreateResponseCodeFilter(httpCode);
    auto regExpFilter = CreateRequestRegExpFilter(requestRegExp);

    return [=](TStringBuf response, const TVector<TString>& requestChunks) {
        return responseCodeFilter(response, requestChunks)
            && requestSizeFilter(response, requestChunks)
            && regExpFilter(response, requestChunks);
    };
}
