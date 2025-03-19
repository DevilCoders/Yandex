#pragma once
#include <util/generic/strbuf.h>
#include <util/generic/array_ref.h>
#include <util/stream/output.h>


namespace NDoom {

class TStringBufSerializer {
public:
    static void Deserialize(const TArrayRef<const char>& serialized, TStringBuf* data) {
        *data = TStringBuf(serialized.data(), serialized.size());
    }
    static ui32 Serialize(const TStringBuf& data, IOutputStream* stream) {
        stream->Write(data.data(), data.size());
        return data.size();
    }

    enum {
        DataSize = -1
    };
};

class TDataRegionSerializer {
public:
    static void Deserialize(const TArrayRef<const char>& serialized, TArrayRef<const char>* data) {
        *data = TArrayRef<const char>(serialized.data(), serialized.size());
    }
    static ui32 Serialize(const TArrayRef<const char>& data, IOutputStream* stream) {
        stream->Write(data.data(), data.size());
        return data.size();
    }

    enum {
        DataSize = -1
    };
};

template<class T>
class TPodSerializer {
public:
    static void Deserialize(const TArrayRef<const char>& serialized, T* data) {
        *data = ReadUnaligned<T>(serialized.data());
    }
    static ui32 Serialize(const T& data, IOutputStream* stream) {
        stream->Write(&data, sizeof(data));
        return sizeof(data);
    }

    enum {
        DataSize = sizeof(T)
    };
};


template<class THit>
class TErfSerializer {
public:
    using TErfType = std::remove_pointer_t<THit>;

    void static Deserialize(const TArrayRef<const char>& serialized, THit* data) {
          *data = reinterpret_cast<THit>(serialized.data());
    }
    ui32 static Serialize(const THit& data, IOutputStream* stream) {
        stream->Write(data, sizeof(TErfType));
        return sizeof(TErfType);
    }

    enum {
        DataSize = sizeof(TErfType)
    };
};

}
