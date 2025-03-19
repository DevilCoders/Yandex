#include "errors.h"

#include <yp/cpp/yp/client.h>
#include <util/string/split.h>


namespace NServiceMonitor {
    NJson::TJsonValue SerializeToJson(std::exception_ptr exception) noexcept {
        NJson::TJsonValue result;
        try {
            std::rethrow_exception(exception);

        } catch (const NYP::NClient::TResponseError& ypResponseError) {
            result["type"] = "NYP::NClient::TResponseError";
            result["code"] = ypResponseError.Code();
            result["message"] = ypResponseError.Message();
            result["details"] = ypResponseError.Details();
            result["request_id"] = ypResponseError.RequestId();
            result["error"] = ToString(ypResponseError.Error());

        } catch (yexception& ex) {
            result["type"] = "yexception";
            result["message"] = ex.what();
            auto& backtraceArray = result["backtrace"];
            const TBackTrace* backTrace = ex.BackTrace();
            if (backTrace) {
                TVector<TStringBuf> btLines;
                TString backTraceString = backTrace->PrintToString();
                Split(backTraceString, "\n", btLines);
                for (auto&& line : btLines) {
                    backtraceArray.AppendValue(line);
                }
            }

        } catch (const std::exception& ex) {
            result["type"] = "std::exception";
            result["message"] = ex.what();

        } catch (...) {
            result["type"] = "unknown";
        }

        return result;
    }
}
