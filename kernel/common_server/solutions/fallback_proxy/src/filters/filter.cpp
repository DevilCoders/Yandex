#include "filter.h"

namespace NCS {
    namespace NFallbackProxy {

        TPQMessagesFakeFilter::TFactory::TRegistrator<TPQMessagesFakeFilter> TPQMessagesFakeFilter::Registrator(TPQMessagesFakeFilter::GetTypeName());

        bool IPQMessagesFilter::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            try {
                return DoDeserializeFromJson(jsonInfo);
            } catch (...) {
                TFLEventLog::Error("couldn't deserialize pq message filter");
                return false;
            }
        }

        NJson::TJsonValue IPQMessagesFilter::SerializeToJson() const {
            try {
                return DoSerializeToJson();
            } catch (...) {
                TFLEventLog::Error("couldn't serialize pq message filter");
                return NJson::JSON_MAP;
            }
        }

        NFrontend::TScheme IPQMessagesFilter::GetScheme(const IBaseServer& server) const {
            return DoGetScheme(server);
        }

        bool IPQMessagesFilter::Accept(const IPQMessage::TPtr message) const {
            try {
                return DoAccept(message);
            } catch (...) {
                TFLEventLog::Error("could not filter message");
                return false;
            }
        }

        bool TPQMessagesFakeFilter::DoDeserializeFromJson(const NJson::TJsonValue& /* jsonInfo */) {
            return true;
        }

        NJson::TJsonValue TPQMessagesFakeFilter::DoSerializeToJson() const {
            return NJson::JSON_MAP;
        }

        bool TPQMessagesFakeFilter::DoAccept(const IPQMessage::TPtr /* message */) const {
            return true;
        }

        NFrontend::TScheme TPQMessagesFakeFilter::DoGetScheme(const IBaseServer& /* server */) const {
            NFrontend::TScheme result;
            return result;
        }
    }
}
