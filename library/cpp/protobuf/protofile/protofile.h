#pragma once

// Reads and writes files with records comprised of ProtocolMessages.
#include <google/protobuf/message.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/store_policy.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/folder/path.h>
#include <library/cpp/logger/global/global.h>

namespace NFastTier {
    // Common interface for Proto readers
    class IGeneralProtoReader {
    public:
        virtual ~IGeneralProtoReader() = default;
        // Stream is not owned
        virtual void OpenWithMaxSize(IInputStream*, size_t maxItemSize) = 0;
        virtual void Open(IInputStream* s) {
            this->OpenWithMaxSize(s, Max<i32>()); //this is natural protobuf size limit
        }
        virtual void Open(const TString&) = 0;
        virtual bool GetNextMessage(::google::protobuf::Message& record) = 0;
    };

    // Common interface for type-restricted proto readers
    template <class T = ::google::protobuf::Message>
    class IProtoReader: public virtual IGeneralProtoReader {
    public:
        bool GetNext(T& record) {
            return this->GetNextMessage(record);
        }
    };

    // Common interface for proto writers
    class IGeneralProtoWriter {
    public:
        virtual ~IGeneralProtoWriter() = default;
        virtual void Open(IOutputStream*) = 0; // Stream is not owned
        virtual void Open(const TString&) = 0;
        virtual void WriteMessage(const ::google::protobuf::Message&) = 0;
        virtual void Finish() = 0;
    };

    // Common interface for type-restricted proto writer
    template <class T = ::google::protobuf::Message>
    class IProtoWriter: public virtual IGeneralProtoWriter {
    public:
        void Write(const T& message) {
            return this->WriteMessage(message);
        }
    };

    // Proto reader in binary format
    class TGeneralBinaryProtoReader: public virtual IGeneralProtoReader {
    private:
        THolder<IInputStream> StreamHolder;
        IInputStream* Stream = nullptr;
        size_t Position = 0;
        size_t MaxItemSize = 0;
        TBuffer Buffer;

        bool ReadDump(void* ptr, size_t size);
    public:
        using IGeneralProtoReader::Open;
        void OpenWithMaxSize(IInputStream*, size_t maxItemSize) override;
        void Open(const TString& fileName) override;
        size_t Tell() const {
            return Position;
        }
        void ForwardSeek(size_t newPosition);
        bool GetNextMessage(::google::protobuf::Message& record) override;
        TGeneralBinaryProtoReader() = default;
        TGeneralBinaryProtoReader(TGeneralBinaryProtoReader&&) = default;
        ~TGeneralBinaryProtoReader() override;
    };

    // Type-restricted proto reader in binary format
    template <class T = ::google::protobuf::Message>
    class TBinaryProtoReader: public IProtoReader<T>, public TGeneralBinaryProtoReader {
    };

    // Proto writer in binary format
    class TGeneralBinaryProtoWriter: public virtual IGeneralProtoWriter {
    private:
        THolder<IOutputStream> StreamHolder;
        IOutputStream* Stream = nullptr;
        size_t Position = 0;

        void WriteDump(void* ptr, size_t size);
    public:
        void Open(IOutputStream* stream) override;
        void Open(const TString& fileName) override;

        size_t Tell() const {
            return Position;
        }

        void WriteMessage(const ::google::protobuf::Message& record) override;
        void Finish() override;
        TGeneralBinaryProtoWriter() = default;
        TGeneralBinaryProtoWriter(TGeneralBinaryProtoWriter&&) = default;
        ~TGeneralBinaryProtoWriter() override;
    };

    // Type-restricted proto writer in binary format
    template <class T = ::google::protobuf::Message>
    class TBinaryProtoWriter: public IProtoWriter<T>, public TGeneralBinaryProtoWriter {
    };

    // Proto reader in text format
    class TGeneralTextProtoReader: public virtual IGeneralProtoReader {
    private:
        THolder<IInputStream> TextInputHolder;
        IInputStream* TextInput = nullptr;

    public:
        using IGeneralProtoReader::Open;
        void OpenWithMaxSize(IInputStream* stream, size_t) override;
        void Open(const TString& file) override;

        bool GetNextMessage(::google::protobuf::Message& record) override;
        TGeneralTextProtoReader() = default;
        TGeneralTextProtoReader(TGeneralTextProtoReader&&) = default;
        ~TGeneralTextProtoReader() override;
    };

    // Type-restricted proto reader in text format
    template <class T = ::google::protobuf::Message>
    class TTextProtoReader: public IProtoReader<T>, public TGeneralTextProtoReader {
    };

    // Proto writer in text format
    class TGeneralTextProtoWriter: public virtual IGeneralProtoWriter {
    private:
        THolder<IOutputStream> TextOutputHolder;
        IOutputStream* TextOutput = nullptr;

    public:
        void Open(IOutputStream* stream) override;
        void Open(const TString& file) override;

        void WriteMessage(const ::google::protobuf::Message& record) override;

        void Finish() override;
        TGeneralTextProtoWriter() = default;
        TGeneralTextProtoWriter(TGeneralTextProtoWriter&&) = default;
        ~TGeneralTextProtoWriter() override;
    };

    // Type-restricted proto writer in text format
    template <class T = ::google::protobuf::Message>
    class TTextProtoWriter: public IProtoWriter<T>, public TGeneralTextProtoWriter {
    };
}

namespace NDetail {
    class TProtoFileGuardBase {
    public:
        TProtoFileGuardBase(::google::protobuf::Message& proto, const TFsPath& path, bool forceOpen = false, bool allowUnknownFields = false);
        void Flush();
        ~TProtoFileGuardBase();

    private:
        TString OldContent;
        ::google::protobuf::Message& Proto;
        TFsPath Path;
        bool Deleted;

    protected:
        bool Changed;
    };
}

template <class TProto>
class TProtoFileGuard: private ::TEmbedPolicy<TProto>, public ::NDetail::TProtoFileGuardBase {
public:
    TProtoFileGuard(const TFsPath& path, bool forceOpen = false, bool allowUnknownFields = false)
        : TProtoFileGuardBase(*TEmbedPolicy<TProto>::Ptr(), path, forceOpen, allowUnknownFields)
    {
    }

    TProto* operator->() {
        Changed = true;
        return this->Ptr();
    }

    const TProto* operator->() const {
        return this->Ptr();
    }

    TProto& operator*() {
        Changed = true;
        return *this->Ptr();
    }

    const TProto& operator*() const {
        return *this->Ptr();
    }

    const TProto& GetProto() const {
        return *this->Ptr();
    }
};
