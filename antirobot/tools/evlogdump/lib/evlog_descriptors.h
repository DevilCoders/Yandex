#pragma once

#include <antirobot/idl/antirobot.ev.pb.h>

#include <antirobot/lib/ar_utils.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>


namespace NAntiRobot {


struct TEvlogEvent {
    explicit TEvlogEvent(TStringBuf eventBytes) {
        Y_ENSURE(
            eventBytes.size() >= sizeof(ui32) + sizeof(ui64) + sizeof(ui32),
            "Malformed event: incomplete header"
        );

        const char* eventPtr = eventBytes.data();

        Size = ReadLittle<ui32>(&eventPtr);
        Timestamp = ReadLittle<ui64>(&eventPtr);
        MessageId = ReadLittle<ui32>(&eventPtr);

        Y_ENSURE(
            eventBytes.size() == Size,
            "Malformed event: invalid size"
        );

        MessageBytes = {eventPtr, eventBytes.end()};
    }

    ui32 Size = 0;
    ui32 MessageId = 0;
    ui64 Timestamp = 0;
    TStringBuf MessageBytes;
};


const THashMap<TString, const NProtoBuf::Descriptor*>& GetEvlogTypeNameToDescriptor();


} // namespace NAntiRobot
