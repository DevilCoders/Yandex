#include "trusted_users.h"

#include <antirobot/idl/factors.pb.h>

#include <google/protobuf/messagext.h>

namespace NAntiRobot {

void TTrustedUsers::Load(IInputStream& in) {
    NFeaturesProto::TUidRecord record;
    NFeaturesProto::THeader header;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TUidRecord",
            "LoadError TTrustedUsers invalid header");
    size_t num = header.GetNum();

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
        Users.insert(record.GetKey());
        --num;
    }

    Y_ENSURE(num == 0, "LoadError TEntityDictionary num mismatch");
}

} // namespace NAntiRobot
