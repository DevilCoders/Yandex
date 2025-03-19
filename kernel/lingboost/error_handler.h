#pragma once

#include <util/memory/pool.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/deque.h>
#include <util/stream/output.h>

namespace NLingBoost {
    class TErrorHandler {
    public:
        enum class EFailMode {
            ThrowOnError,
            SkipOnError
        };

        struct TOptions {
            EFailMode FailMode = EFailMode::SkipOnError;

            TOptions() {}
            TOptions(EFailMode mode)
                : FailMode(mode)
            {}
        };

        class IContextPart;
        class TStringBufPart;
        template <typename IndexType>
        class TIndexedBufPart;

        using TContextPartPtr = IContextPart*; // should be trivially destructable

        struct TGuard;
        friend struct TGuard;

    public:
        TErrorHandler(const TOptions& opts = {})
            : Opts(opts)
            , Pool(256, TMemoryPool::TLinearGrow::Instance())
        {}

        template <typename TError>
        void Error(const TError& err);

        template <typename TError>
        void Check(bool cond, const TError& err) {
            if (!cond) {
                Error(err);
            }
        }

        bool IsInErrorState() const {
            return !Errors.empty();
        }
        void ClearErrorState() {
            Y_ASSERT(ErrorContext.empty());
            ErrorContext.clear();
            Errors.clear();
            Pool.Clear();
        }

        TString GetFullErrorMessage(const TStringBuf& prefix = {}) const;
        TString GetContextString() const;

        TContextPartPtr StringBufPart(TStringBuf text);

        template <typename IndexType>
        TContextPartPtr IndexedBufPart(TStringBuf text, IndexType index);

    private:
        void PushErrorContext(TContextPartPtr&& part);
        void PopErrorContext();

        struct TErrorInfo {
            TString Message;
            TString Context;
        };

        TOptions Opts;
        TDeque<TErrorInfo> Errors;
        TDeque<TContextPartPtr> ErrorContext;
        TMemoryPool Pool;
    };

    //
    //

    struct TErrorHandler::TGuard {
        TErrorHandler& Handler;

        template <size_t Size>
        TGuard(
            TErrorHandler& handler,
            const char (&text)[Size])
            : Handler(handler)
        {
            static_assert(Size > 0);
            Handler.PushErrorContext(Handler.StringBufPart(TStringBuf(text, Size - 1)));
        }
        template <size_t Size, typename IndexType>
        TGuard(
            TErrorHandler& handler,
            const char (&text)[Size],
            IndexType index)
            : Handler(handler)
        {
            static_assert(Size > 0);
            Handler.PushErrorContext(Handler.IndexedBufPart(TStringBuf(text, Size - 1), index));
        }
        ~TGuard() {
            Handler.PopErrorContext();
        }
    };

    template <typename TError>
    inline void TErrorHandler::Error(const TError& err) {
        if (EFailMode::ThrowOnError == Opts.FailMode) {
            throw err;
        } else {
            Y_ASSERT(EFailMode::SkipOnError == Opts.FailMode);
            Errors.push_back({TString(err.AsStrBuf()), GetContextString()});
        }
    }

    class TErrorHandler::IContextPart {
    public:
        virtual void Print(IOutputStream& out) const = 0;
    };

    class TErrorHandler::TStringBufPart
        : public IContextPart
    {
    public:
        explicit TStringBufPart(TStringBuf text)
            : Text(text)
        {}

        void Print(IOutputStream& out) const override {
            out << Text;
        }

    private:
        TStringBuf Text;
    };

    template <typename IndexType>
    class TErrorHandler::TIndexedBufPart
        : public IContextPart
    {
    public:
        explicit TIndexedBufPart(TStringBuf text, IndexType index)
            : Text(text)
            , Index(index)
        {}

        void Print(IOutputStream& out) const override {
            out << Text << "(" << Index << ")";
        }

    private:
        TStringBuf Text;
        IndexType Index{};
    };

    inline TErrorHandler::TContextPartPtr TErrorHandler::StringBufPart(
        TStringBuf text)
    {
        return Pool.New<TStringBufPart>(text);
    }

    template <typename IndexType>
    inline TErrorHandler::TContextPartPtr TErrorHandler::IndexedBufPart(
        TStringBuf text,
        IndexType index)
    {
        return Pool.New<TIndexedBufPart<IndexType>>(text, index);
    }

} // NLingBoost
