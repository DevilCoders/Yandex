#pragma once

#include <google/protobuf/message.h>

#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>

namespace NProtoParser {
    using namespace NProtoBuf;

    // Base class for custom mergers
    class TBaseMerger {
    };

    template <class TSource>
    class TDefaultMerger;

    class TMessageParser {
    public:
        enum {
            SILENT = 1,                // ignore convert errors
            IGNORE_UNKNOWN_FIELDS = 2, // don't throw exception if message has no wanted field
            ATOMIC = 4,                // copies old optional submessage before merging it and recover it in case of error

            DEFAULT_MODE = IGNORE_UNKNOWN_FIELDS | ATOMIC,
        };

    public:
        TMessageParser(Message& msg, int mode = DEFAULT_MODE)
            : Mode(mode)
            , Msg(&msg)
        {
        }

        //
        // Merge field
        //

        void Merge(const FieldDescriptor* field, TStringBuf value);
        void Merge(const FieldDescriptor* field, TString value);
        void Merge(const FieldDescriptor* field, const char* value);
        void Merge(const FieldDescriptor* field, i64 value);
        void Merge(const FieldDescriptor* field, ui64 value);
        void Merge(const FieldDescriptor* field, i32 value);
        void Merge(const FieldDescriptor* field, ui32 value);
        void Merge(const FieldDescriptor* field, bool value);
        void Merge(const FieldDescriptor* field, float value);
        void Merge(const FieldDescriptor* field, double value);

        //
        // Field by name templates
        //

        template <class TSource>
        void Merge(const char* field, const TSource& src) {
            return Merge(FindFieldByName(TStringBuf(field)), src);
        }

        template <class TSource>
        void Merge(const TStringBuf& field, const TSource& src) {
            return Merge(FindFieldByName(field), src);
        }

        template <class TSource>
        void Merge(TString field, const TSource& src) {
            return Merge(FindFieldByName(field), src);
        }

        //
        // Custom merger
        //

        template <class TSource, class TMerger>
        void Merge(const TStringBuf& field, const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>* = NULL) {
            return Merge(FindFieldByName(field), src, merger);
        }

        template <class TSource, class TMerger>
        void Merge(TString field, const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>* = NULL) {
            return Merge(FindFieldByName(field), src, merger);
        }

        template <class TSource, class TMerger>
        void Merge(const FieldDescriptor* field, const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>* = nullptr);

        //
        // Custom type merger with automatically deduced template arguments
        //

        template <class TSource>
        void Merge(const FieldDescriptor* field, const TSource& src, typename TDefaultMerger<TSource>::TEnableDefaultMerger* = nullptr) {
            Merge(field, src, TDefaultMerger<TSource>());
        }

        //
        // Merge whole message
        //

        template <class TSource, class TMerger>
        void Merge(const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>* = nullptr);

        template <class TSource>
        void Merge(const TSource& src, typename TDefaultMerger<TSource>::TEnableDefaultMerger* = nullptr) {
            Merge(src, TDefaultMerger<TSource>());
        }

        const Message& GetMessage() const {
            return *Msg;
        }

        Message& GetMessage() {
            return *Msg;
        }

        const FieldDescriptor* FindFieldByName(const TString& field);    // throws if not IGNORE_UNKNOWN_FIELDS
        const FieldDescriptor* FindFieldByName(const TStringBuf& field); // don't call from different threads!!! // throws if not IGNORE_UNKNOWN_FIELDS

    private:
        //
        // Mode
        //

        bool IsSilent() const {
            return (Mode & SILENT) != 0;
        }

        bool IsIgnoreUnknown() const {
            return (Mode & IGNORE_UNKNOWN_FIELDS) != 0;
        }

        bool IsAtomic() const {
            return (Mode & ATOMIC) != 0;
        }

    private:
        class TSubMessageHolder {
        public:
            TSubMessageHolder(TMessageParser* parent, const FieldDescriptor* desc);
            ~TSubMessageHolder();

            void Release() {
                SubMsg = nullptr;
                Parent = nullptr;
            }

            Message* GetMessage() {
                return SubMsg;
            }

        private:
            TMessageParser* Parent;
            const FieldDescriptor* Desc;
            Message* SubMsg;
            THolder<Message> Copy;
        };

    private:
        // If field is submessage, returns it
        static bool IsSubMessage(const FieldDescriptor* field);

        template <class TValue>
        void MergeValue(const FieldDescriptor* desc, const TValue& value);

        void MergeWithDelimiter(const FieldDescriptor* desc, TStringBuf value, char delim);

    private:
        int Mode;
        Message* Msg;
        TString Buffer; // optimization for memory allocations
    };

    template <class TSource, class TMerger>
    void TMessageParser::Merge(const FieldDescriptor* desc, const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>*) {
        if (!desc) {
            if (IsSilent() || IsIgnoreUnknown())
                return;
            ythrow yexception() << "No field descriptor";
        }
        if (IsSubMessage(desc)) {
            TSubMessageHolder sub(this, desc);
            TMessageParser child(*sub.GetMessage(), Mode);
            child.Merge(src, merger);
            sub.Release();
        } else {
            try {
                merger(*this, desc, src);
            } catch (const TBadCastException&) {
                if (!IsSilent())
                    throw;
            }
        }
    }

    template <class TSource, class TMerger>
    void TMessageParser::Merge(const TSource& src, const TMerger& merger, std::enable_if_t<std::is_base_of<TBaseMerger, TMerger>::value, void>*) {
        try {
            merger(*this, src);
        } catch (const TBadCastException&) {
            if (!IsSilent())
                throw;
        }
    }

    template <class TSource>
    void MergeFrom(Message& msg, const TSource& src, int mode = TMessageParser::DEFAULT_MODE) {
        TMessageParser parser(msg, mode);
        parser.Merge(src);
    }

}
