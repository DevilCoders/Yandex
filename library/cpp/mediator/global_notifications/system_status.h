#pragma once

#include <library/cpp/mediator/messenger.h>

#include <util/stream/output.h>
#include <util/string/printf.h>

class TSystemStatusMessage: public IMessage {
public:
    enum class ESystemStatus {
        ssOK,
        ssUnknownError,
        ssIncorrectConfig,
        ssIncorrectData,
        ssSemiOK
    };

private:
    TString Message;
    ESystemStatus Status;

public:
    TSystemStatusMessage(const TString& message, const ESystemStatus& status)
        : Message(message)
        , Status(status)
    {
    }

    const TString& GetMessage() const {
        return Message;
    }

    ESystemStatus GetStatus() const {
        return Status;
    }
};

[[noreturn]] void AbortFromCorruptedIndex(const TString& message);
[[noreturn]] void AbortFromCorruptedConfig(const TString& message);

inline void AbortFromCorruptedIndex(const char* message) {
    AbortFromCorruptedIndex(TString(message));
}
inline void AbortFromCorruptedConfig(const char* message) {
    AbortFromCorruptedConfig(TString(message));
}
template <class... TArgs>
inline void AbortFromCorruptedIndex(const char* msg, TArgs... args) {
    AbortFromCorruptedIndex(Sprintf(msg, args...));
}
template <class... TArgs>
inline void AbortFromCorruptedConfig(const char* msg, TArgs... args) {
    AbortFromCorruptedConfig(Sprintf(msg, args...));
}

template <class... TArgs>
Y_FORCE_INLINE void AssertCorrectIndex(bool condition, TArgs... args) {
    if (Y_UNLIKELY(!condition)) {
        AbortFromCorruptedIndex(args...);
    }
}

template <class... TArgs>
Y_FORCE_INLINE void AssertCorrectConfig(bool condition, TArgs... args) {
    if (Y_UNLIKELY(!condition)) {
        AbortFromCorruptedConfig(args...);
    }
}
