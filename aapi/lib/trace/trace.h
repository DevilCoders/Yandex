#pragma once

#include <aapi/lib/trace/events.ev.pb.h>

#include <google/protobuf/message.h>

namespace NAapi {

using TMessage = ::google::protobuf::Message;

struct TProxyEvent {
    size_t ID;
    const TMessage* Msg;

    template <class T>
    inline TProxyEvent(const T& ev)
        : ID(ev.ID)
        , Msg(&ev)
    {
    }

    inline operator const TMessage&() const noexcept {
        return *Msg;
    }
};

void InitTrace(const TString& path, bool debugOutput = false);

void Trace(const TProxyEvent& ev);

}  // namespace NAapi
