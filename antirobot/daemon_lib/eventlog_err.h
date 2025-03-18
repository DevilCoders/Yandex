#pragma once

#include "config_global.h"
#include "request_params.h"
#include "uid.h"


namespace NAntirobotEvClass {
    class THeader;
}

namespace NAntiRobot {
    class TRequest;

    enum EEvlogLevel {
        EVLOG_ERROR = 0,
        EVLOG_WARNING,
        EVLOG_MAXLEVEL = EVLOG_WARNING
    };

    class TEvlogMessage {
    public:
        TEvlogMessage(const char* file, int line)
            : Request(nullptr)
            , Header(nullptr)
            , Level(EVLOG_WARNING)
        {
            Buf << file << ':' << line << ' ';
        }

        ~TEvlogMessage();

        template <class T>
        inline TEvlogMessage& operator<< (const T& t) {
            if ((ui32)Level <= ANTIROBOT_DAEMON_CONFIG.LogLevel)
                Buf << t;
            return *this;
        }

        inline TEvlogMessage& operator<< (const TRequest& req) {
            Request = &req;
            return *this;
        }

        inline TEvlogMessage& operator<< (const NAntirobotEvClass::THeader& header) {
            Header = &header;
            return *this;
        }

        inline TEvlogMessage& operator<< (const EEvlogLevel level) {
            Level = (ui32)level;
            return *this;
        }
    private:
        class TSafeTempBufOutput: public IOutputStream, public TTempBuf {
            public:
                void DoWrite(const void* data, size_t len) override {
                    Append(data, Min(len, Left()));
                }
        };

        const TRequest* Request;
        const NAntirobotEvClass::THeader* Header;
        ui32 Level;
        TSafeTempBufOutput Buf;
    };

    NAntirobotEvClass::THeader MakeEvlogHeader(const TStringBuf& reqid, const TAddr& userAddr, const TUid& uid,
            const TStringBuf& yandexuid, const TAddr& partnerAddr, const TString& uniqueKey);

    NAntirobotEvClass::THeader MakeEvlogHeader(const TAddr& userAddr);
}

#define EVLOG_MSG TEvlogMessage(__FILE__, __LINE__)
