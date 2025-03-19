#pragma once
#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/generic/buffer.h>
#include <util/generic/string.h>
#include <library/cpp/logger/global/global.h>
#include <contrib/libs/protobuf/src/google/protobuf/message_lite.h>

template <class THeader>
class TBlobWithHeader {
private:
    THeader Header;
    TBlob Data;
public:

    const THeader& GetHeader() const {
        return Header;
    }

    const TBlob& GetData() const {
        return Data;
    }

    TBlobWithHeader() {
    }

    TBlobWithHeader(const THeader& h, TBlob&& data)
        : Header(h)
        , Data(data) {
    }

    bool Load(const TBlob& data) {
        TMemoryInput input(data.AsCharPtr(), data.Size());
        ui32 size;
        if (input.Read(&size, sizeof(size)) != sizeof(size)) {
            return false;
        }
        if (size > data.Size()) {
            return false;
        }
        TBuffer metaBuf(size);
        input.Read(metaBuf.Data(), size);
        if (!Header.ParseFromArray(metaBuf.Data(), size)) {
            return false;
        }

        Data = TBlob::FromStream(input);
        return true;
    }

    TBlob Save() const {
        TBuffer bufResult;

        TString dataMetaSerialization;
        Y_PROTOBUF_SUPPRESS_NODISCARD Header.SerializeToString(&dataMetaSerialization);
        ui32 size = dataMetaSerialization.size();
        bufResult.Append((const char*)&size, sizeof(size));
        bufResult.Append(dataMetaSerialization.data(), dataMetaSerialization.size());

        bufResult.Append(Data.AsCharPtr(), Data.Size());

        return TBlob::FromBuffer(bufResult);
    }
};

template <class TProto, class TBaseClass>
class IProtoSerializable: public TBaseClass {
protected:
    virtual void SerializeToProto(TProto& proto) const = 0;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const TProto& proto) = 0;
public:

    using TBaseClass::TBaseClass;

    virtual TBlob Serialize() const noexcept override final {
        TProto proto;
        SerializeToProto(proto);
        return TBlob::FromString(proto.SerializeAsString());
    }

    Y_WARN_UNUSED_RESULT virtual bool Deserialize(const TBlob& data) noexcept override final {
        TProto proto;
        if (!proto.ParseFromArray(data.AsCharPtr(), data.Size())) {
            ERROR_LOG << "Cannot parse data from proto in regular process" << Endl;
            return false;
        }
        return DeserializeFromProto(proto);
    }

};

