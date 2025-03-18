#include "threadsafe_reqstat.h"

#include <library/cpp/stat-handle/proto/stat.pb.h>
#include <library/cpp/protobuf/json/proto2json.h>

namespace NStat {
    TString TThreadSafeReqStat::ToJson() const {
        // This method is copy-pasted from library/cpp/stat-handle/stat.h to ensure its thread-safety.
        // If you irrecoverably fix this method there, fix it also here, please.
        // Note that here we cannot just lock the mutex and call TStatBase::ToJson,
        // because 'ToProto' non-recursively locks the same mutex.
        TStatProto proto;
        ToProto(proto); //< locks this->Mutex
        TString out;
        NProtobufJson::Proto2Json(proto, out);
        return out;
    }
}
