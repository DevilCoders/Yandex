#pragma once
#include "abstract.h"
#include <util/generic/cast.h>
#include "scheme.h"
#include <library/cpp/http/misc/httpcodes.h>
#include <util/generic/guid.h>
#include <kernel/common_server/common/url_matcher.h>
#include "fields.h"

namespace NCS {
    namespace NScheme {
        class THandlerRequestBody {
        private:
            using TContentByType = TMap<TString, TScheme>;
            CSA_READONLY_DEF(TContentByType, ContentByType);
            CSA_FLAG(THandlerRequestBody, Required, true);
            CSA_DEFAULT(THandlerRequestBody, TString, Description);
        public:
            TScheme* operator->() {
                return &Content();
            }

            TScheme& Content(const TString& contentType = "application/json") {
                auto it = ContentByType.find(contentType);
                if (it == ContentByType.end()) {
                    it = ContentByType.emplace(contentType, TScheme()).first;
                }
                return it->second;
            }
        };

        class THandlerResponse {
        private:
            CS_ACCESS(THandlerResponse, TString, Description, TGUID::Create().AsUuidString());
            using TContentByType = TMap<TString, TVector<TScheme>>;
            CSA_DEFAULT(THandlerResponse, TContentByType, ContentByType);
            using THeaders = TMap<TString, IElement::TPtr>;
            CSA_DEFAULT(THandlerResponse, THeaders, Headers);
        public:
            TScheme& AddContent(const TString& contentType = "*/*") {
                auto it = ContentByType.find(contentType);
                if (it == ContentByType.end()) {
                    it = ContentByType.emplace(contentType, TVector<TScheme>()).first;
                }
                it->second.emplace_back(TScheme());
                return it->second.back();
            }
            template <class T = TFSString>
            T& AddHeader(const TString& header, const TString& description = "") {
                auto it = Headers.find(header);
                if (it == Headers.end()) {
                    it = Headers.emplace(header, MakeAtomicShared<T>(header, description)).first;
                } else {
                    if (!dynamic_cast<T*>(&*it->second)) {
                        it->second = MakeAtomicShared<T>(header, description);
                    }
                }
                return *VerifyDynamicCast<T*>(&*it->second);
            }
        };

        class THandlerRequestParameters {
        private:
            CSA_DEFAULT(THandlerRequestParameters, TVector<IElement::TPtr>, Parameters);
            template <class T>
            T& Add(const TString& fieldName, const TString& originator, const TString& description) {
                Y_ASSERT(!!fieldName);
                Parameters.emplace_back(MakeAtomicShared<T>(fieldName, description));
                Parameters.back()->SetOriginator(originator);
                return *VerifyDynamicCast<T*>(&*Parameters.back());
            }
        public:
            template <class T>
            T& AddPath(const TString& fieldName, const TString& description = "") {
                return Add<T>(fieldName, "path", description);
            }

            template <class T>
            T& AddQuery(const TString& fieldName, const TString& description = "") {
                return Add<T>(fieldName, "query", description);
            }

        };

        class THandlerRequestMethod {
        private:
            CSA_DEFAULT(THandlerRequestMethod, THandlerRequestParameters, Parameters);
            CSA_MAYBE(THandlerRequestMethod, THandlerRequestBody, RequestBody);
            using TResponseByCode = TMap<TString, THandlerResponse>;
            CSA_DEFAULT(THandlerRequestMethod, TResponseByCode, Responses);
            CSA_DEFAULT(THandlerRequestMethod, TString, Description);
            CSA_DEFAULT(THandlerRequestMethod, TString, Summary);
            CSA_DEFAULT(THandlerRequestMethod, TString, OperationId);
        private:
            THandlerResponse& Response(const TString& code) {
                auto it = Responses.find(code);
                if (it == Responses.end()) {
                    it = Responses.emplace(code, THandlerResponse()).first;
                }
                return it->second;
            }
        public:
            void AddDefaultReply(const THandlerResponse& defaultResponse) {
                if (Responses.contains("default")) {
                    return;
                }
                Responses.emplace("default", defaultResponse);
            }
            THandlerResponse& Response(const HttpCodes code) {
                return Response(::ToString((ui32)code));
            }
            THandlerResponse& ResponseDefault() {
                return Response("default");
            }
            THandlerRequestBody& Body() {
                if (!RequestBody) {
                    RequestBody.ConstructInPlace();
                }
                return *RequestBody;
            }
        };

        class THandlerScheme {
        private:
            CSA_DEFAULT(THandlerScheme, TVector<TPathSegmentInfo>, Path);
            CSA_DEFAULT(THandlerScheme, THandlerRequestParameters, Parameters);
            using TRequestByMethod = TMap<ERequestMethod, THandlerRequestMethod>;
            CSA_READONLY_DEF(TRequestByMethod, RequestMethods);
        public:
            void AddDefaultReply(const THandlerResponse& defaultResponse) {
                for (auto&& [_, reqMethod] : RequestMethods) {
                    reqMethod.AddDefaultReply(defaultResponse);
                    for (auto&& [_, response] : reqMethod.MutableResponses()) {
                        response.AddHeader("X-YaRequestId", "Идентификатор запроса для трассировки логов");
                    }
                }
            }

            THandlerRequestMethod& Method(const ERequestMethod method) {
                auto it = RequestMethods.find(method);
                if (it == RequestMethods.end()) {
                    it = RequestMethods.emplace(method, THandlerRequestMethod()).first;
                }
                return it->second;
            }

            THandlerScheme() = default;
            THandlerScheme(const TVector<TPathSegmentInfo>& path)
                : Path(path) {
            }
        };

        class THandlerSchemasCollection {
        private:
            CSA_READONLY_DEF(TVector<THandlerScheme>, Schemas);
        public:
            THandlerSchemasCollection(TVector<THandlerScheme>&& schemas)
                : Schemas(std::move(schemas))
            {

            }

            NJson::TJsonValue SerializeToJson() const;
        };
    }
}
