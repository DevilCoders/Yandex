#include "client.h"
#include <kernel/common_server/library/tvm_services/abstract/request/json.h>

namespace NCS {

    class TStaffRequest final : public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        const TStaffEntrySelector& Selector;
        const TStaffEntry::TStaffEntryFields& Fields;
        size_t Limit;
        TMaybe<ui64> FirstId;
    public:
        TStaffRequest(const TStaffEntrySelector& selector, const TStaffEntry::TStaffEntryFields& fields, size_t limit, TMaybe<ui64> firstId)
            : Selector(selector)
            , Fields(fields)
            , Limit(limit)
            , FirstId(firstId)
        {}

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            Selector.FillRequest(request, FirstId);
            request.AddCgiData("_fields", TStaffEntry(Fields).GetFieldsStr());
            if (Limit > 1) {
                request.AddCgiData("_sort", "id");
                request.AddCgiData("_limit", Limit);
            } else {
                request.AddCgiData("_one", "1");
            }
            return true;
        }

        class TResponse : public NExternalAPI::IHttpRequestWithJsonReport::TJsonResponse {
            CSA_READONLY_DEF(TStaffClient::TStaffEntries, Results);
            const TStaffEntry::TStaffEntryFields* Fields = nullptr;
            bool One = false;
        public:
            virtual void ConfigureFromRequest(const IServiceApiHttpRequest* request) override {
                const auto* req = dynamic_cast<const TStaffRequest*>(request);
                CHECK_WITH_LOG(req);
                Fields = &req->Fields;
                One = req->Limit <= 1;
            };

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& doc) override {
                Results.clear();
                if (GetCode() != 200) {
                    return true;
                }
                if (One) {
                    Results.emplace_back(*Fields);
                    if (!Results.back().DeserializeFromJson(doc)) {
                        return false;
                    }
                } else {
                    for (const auto& r : doc["result"].GetArray()) {
                        Results.emplace_back(*Fields);
                        if (!Results.back().DeserializeFromJson(r)) {
                            return false;
                        }
                    }
                }
                return true;
            }
            virtual bool IsReplyCodeSuccess(const i32 code) const override {
                return code == 200 || code == 404;
            }
        };
    };


    TStaffClient::TStaffClient(NExternalAPI::TSender::TPtr sender)
        : Sender(sender)
    {}

    bool TStaffClient::GetUserData(const TStaffEntrySelector& selector,
        TStaffEntries& results,
        const TStaffEntry::TStaffEntryFields& fields,
        size_t limit
    ) const {
        TMaybe<ui64> lastId;
        while (results.size() < limit) {
            const auto reqLimit = Max<size_t>(limit - results.size(), 50);
            const auto response = Sender->SendRequest<TStaffRequest>(selector, fields, reqLimit, lastId);
            if (!response.IsSuccess()) {
                return false;
            }
            if (response.GetResults().empty()) {
                break;
            }
            results.insert(results.end(), response.GetResults().cbegin(), response.GetResults().cend());
            lastId = results.back().GetId();
        }
        return true;
    }

}
