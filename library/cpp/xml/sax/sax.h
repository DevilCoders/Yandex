#pragma once

#include <util/generic/string.h>
#include <util/generic/yexception.h>

class IInputStream;

namespace NXml {
    struct IParserContext {
        virtual void GetContext(size_t& line, size_t& column, TString& textContext) = 0;
    };

    class ISaxHandler {
    public:
        struct TErrorInfo {
            TString Message;
            size_t Line;
            size_t Column;
            TString Context;
        };

        class TError: public yexception {
        public:
            inline TError() {
            }

            inline TError(const TString& msg) {
                *this << msg;
            }
        };

        class TParseError: public TError {
        public:
            inline TParseError(const TErrorInfo& err)
                : TError(err.Message)
                , Err_(err)
            {
            }

            inline const TErrorInfo& ErrorInfo() const noexcept {
                return Err_;
            }

        private:
            const TErrorInfo Err_;
        };

        ISaxHandler() noexcept;
        virtual ~ISaxHandler();

        virtual void OnStartDocument();
        virtual void OnEndDocument();

        virtual void OnStartElement(const char* name, const char** attrs) = 0;
        virtual void OnEndElement(const char* name) = 0;

        virtual void OnText(const char* text, size_t len) = 0;
        virtual bool GetEntity(const TStringBuf& entityName, TString& entityExpansion);

        virtual void OnWarning(const TErrorInfo& ei);
        virtual void OnError(const TErrorInfo& ei);
        virtual void OnFatalError(const TErrorInfo& ei);

        IParserContext* GetParserContext();
        void SetParserContext(IParserContext*);

    private:
        IParserContext* Context = nullptr;
    };

    void ParseXml(IInputStream* in, ISaxHandler* parser);
}
