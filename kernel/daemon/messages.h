#pragma once

#include "info.h"

#include <library/cpp/mediator/messenger.h>

#include <util/generic/map.h>

class TCollectDaemonInfoMessage: public IMessage {
private:
    TString CType;
    TString Service;
public:

    bool Initialized() const {
        return CType || Service;
    }

    TCollectDaemonInfoMessage& SetCType(const TString& value) {
        CType = value;
        return *this;
    }
    TCollectDaemonInfoMessage& SetService(const TString& value) {
        Service = value;
        return *this;
    }

    const TString& GetCType() const {
        return CType;
    }
    const TString& GetService() const {
        return Service;
    }
};

class TCollectServerInfo: public IMessage {
public:
    virtual void Fill(TServerInfo& info) {
        for (const auto& field : Fields)
            info.InsertValue(field.first, field.second);
    }

    TMap<TString, NJson::TJsonValue> Fields;
};

class TMessageReopenLogs: public IMessage {
public:
    TMessageReopenLogs() = default;
};

struct TMessageUpdateUnistatSignals : public IMessage {
};
