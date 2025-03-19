#pragma once

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/compressor/factory.h>

#include <util/generic/buffer.h>
#include <util/generic/string.h>

namespace NRichTreeProtocol {
    class TRichRequestNode;     // tree protobuf format
}


class TRichTreeSerializer {
public:
    static const TCompressorFactory::EFormat DEFAULT_COMPRESSOR = TCompressorFactory::EFormat::ZLIB_DEFAULT;

    TRichTreeSerializer();
    ~TRichTreeSerializer();

    void Serialize(const NSearchQuery::TRequest& request, TString& output, TCompressorFactory::EFormat format = DEFAULT_COMPRESSOR);
    void Serialize(const NSearchQuery::TRequest& request, TBuffer& output, TCompressorFactory::EFormat format = DEFAULT_COMPRESSOR);

    void SerializeBase64(const NSearchQuery::TRequest& request, TString& output, TCompressorFactory::EFormat format = DEFAULT_COMPRESSOR);

    void SerializeFromProto(const NRichTreeProtocol::TRichRequestNode& proto, bool base64, TString& tree, TCompressorFactory::EFormat format = DEFAULT_COMPRESSOR);

private:
    class TImpl;
    THolder<TImpl> Impl;
};


class TRichTreeDeserializer {
public:
    TRichTreeDeserializer();
    ~TRichTreeDeserializer();

    void Deserialize(const TStringBuf& binaryTree, NSearchQuery::TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE);
    void DeserializeBase64(const TStringBuf& base64Tree, NSearchQuery::TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE);

    void DeserializeToProto(const TStringBuf& tree, bool base64, NRichTreeProtocol::TRichRequestNode& proto, bool humanReadable = false);

private:
    class TImpl;
    THolder<TImpl> Impl;
};

