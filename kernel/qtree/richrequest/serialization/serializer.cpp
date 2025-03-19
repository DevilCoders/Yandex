#include "serializer.h"

#include <library/cpp/streams/lz/lz.h>

#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>

#include <util/stream/buffer.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>


using  NSearchQuery::TRequest;

class TRichTreeProto {
public:
    TRichTreeProto()
        : IsCleared(true)
    {
    }

    void Clear() {
        if (!IsCleared) {
            // clearing is quite heavy operation for for big protobuf tree,
            // so try avoiding it for the fist serialization (which is also the last one in most cases)
            Tree.Clear();
            IsCleared = true;
        }
    }

    void LoadFrom(const TRequest& request, bool humanReadable = false) {
        Clear();
        Tree.MutableBase()->SetSoftness(request.Softness);
        ::Serialize(*request.Root, Tree, humanReadable);
        IsCleared = false;
    }

    void SerializeToString(TString& str) const {
        if (Y_UNLIKELY(!Tree.SerializeToString(&str)))
            ythrow yexception() << "Rich tree serialization failed";
    }

    void DeserializeFromString(const TString& input) {
        if (Y_UNLIKELY(!Tree.ParseFromString(input))) {
            ythrow yexception() << "Rich tree deserialization failed";
        }
        IsCleared = false;
    }

    void DeserializeFromStream(IInputStream* input) {
        DeserializeFromString(input->ReadAll());
    }

    void SaveTo(TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE) const {
        request.Softness = Tree.GetBase().GetSoftness();
        request.Root = ::Deserialize(Tree, mode);
    }

    void CopyTo(NRichTreeProtocol::TRichRequestNode& p) const {
        p.CopyFrom(Tree);
    }

    void ConvertToHumanReadable() {
        // deserialize/serialize via TRequest, in fact
        TRequest request;
        SaveTo(request);
        LoadFrom(request, true);
    }

private:
    NRichTreeProtocol::TRichRequestNode Tree;
    bool IsCleared;
};

// Serializer with own tmp buffers for intermediate data
class TRichTreeSerializer::TImpl {
public:
    void SerializeProto(const TRequest& request, TString& output) {
        Proto.LoadFrom(request);
        Proto.SerializeToString(output);
    }

    void Serialize(const TRequest& request, IOutputStream* output, TCompressorFactory::EFormat format) {
        SerializeProto(request, SerializedProto);
        TCompressorFactory::Compress(output, SerializedProto, format);
    }

    void Serialize(const TRequest& request, TBuffer& output, TCompressorFactory::EFormat format) {
        output.Clear();
        TBufferOutput tmp(output);
        Serialize(request, &tmp, format);
    }

    void Serialize(const TRequest& request, TString& output, TCompressorFactory::EFormat format) {
        output.clear();
        TStringOutput tmp(output);
        Serialize(request, &tmp, format);
    }

    void SerializeBase64(const TRequest& request, TString& output, TCompressorFactory::EFormat format) {
        Serialize(request, CompressedBinary, format);
        Base64EncodeUrl(CompressedBinary, output);
    }

    void SerializeFromProto(const NRichTreeProtocol::TRichRequestNode& proto, bool base64, TString& tree, TCompressorFactory::EFormat format) {
        TString binary;

        Y_ENSURE(proto.SerializeToString(&binary), "Rich tree serialization failed");

        TString compressedBinary;
        TStringOutput tmp(compressedBinary);
        TCompressorFactory::Compress(&tmp, compressedBinary, format);

        if (base64) {
            Base64EncodeUrl(compressedBinary, tree);
        } else {
            tree = compressedBinary;
        }
    }

    void Deserialize(IInputStream* input, TRequest& request) {
        // read all data from @input up to the end of stream!
        Proto.DeserializeFromString(TCompressorFactory::Decompress(input));
        Proto.SaveTo(request);
    }

    void Deserialize(const TStringBuf& binaryTree, TRequest& request) {
        TMemoryInput input(binaryTree.data(), binaryTree.size());
        Deserialize(&input, request);
    }

    void DeserializeBase64(const TStringBuf& base64Tree, TRequest& request) {
        Base64Decode(base64Tree, CompressedBinary);
        Deserialize(CompressedBinary, request);
    }

private:
    TRichTreeProto Proto;
    TString SerializedProto, CompressedBinary;
};



class TRichTreeDeserializer::TImpl {
public:
    void Deserialize(IInputStream* input, TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE) {
        // read all data from @input up to the end of stream!
        Proto.DeserializeFromString(TCompressorFactory::Decompress(input));
        Proto.SaveTo(request, mode);
    }

    void Deserialize(const TStringBuf& binaryTree, TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE) {
        TMemoryInput input(binaryTree.data(), binaryTree.size());
        Deserialize(&input, request, mode);
    }

    void DeserializeBase64(const TStringBuf& base64Tree, TRequest& request, EQtreeDeserializeMode mode = QTREE_DEFAULT_DESERIALIZE) {
        Base64Decode(base64Tree, CompressedBinary);
        Deserialize(CompressedBinary, request, mode);
    }

    void DeserializeToProto(const TStringBuf& tree, bool base64, NRichTreeProtocol::TRichRequestNode& proto, bool humanReadable) {
        TMemoryInput input(tree.data(), tree.size());
        if (base64) {
            Base64Decode(tree, CompressedBinary);
            input.Reset(CompressedBinary.data(), CompressedBinary.size());
        }
        Proto.DeserializeFromString(TCompressorFactory::Decompress(&input));
        if (humanReadable)
            Proto.ConvertToHumanReadable();
        Proto.CopyTo(proto);
    }

private:
    TRichTreeProto Proto;
    TString CompressedBinary;
};





TRichTreeSerializer::TRichTreeSerializer()
    : Impl(new TImpl())
{
}

TRichTreeSerializer::~TRichTreeSerializer() {
}

void TRichTreeSerializer::Serialize(const TRequest& request, TString& output, TCompressorFactory::EFormat format) {
    Impl->Serialize(request, output, format);
}

void TRichTreeSerializer::Serialize(const TRequest& request, TBuffer& output, TCompressorFactory::EFormat format) {
    Impl->Serialize(request, output, format);
}

void TRichTreeSerializer::SerializeBase64(const TRequest& request, TString& output, TCompressorFactory::EFormat format) {
    Impl->SerializeBase64(request, output, format);
}

void TRichTreeSerializer::SerializeFromProto(const NRichTreeProtocol::TRichRequestNode& proto, bool base64, TString& tree, TCompressorFactory::EFormat format) {
    Impl->SerializeFromProto(proto, base64, tree, format);
}




TRichTreeDeserializer::TRichTreeDeserializer()
    : Impl(new TImpl())
{
}

TRichTreeDeserializer::~TRichTreeDeserializer() {
}

void TRichTreeDeserializer::Deserialize(const TStringBuf& binaryTree, TRequest& request, EQtreeDeserializeMode mode) {
    Impl->Deserialize(binaryTree, request, mode);
}

void TRichTreeDeserializer::DeserializeBase64(const TStringBuf& base64Tree, TRequest& request, EQtreeDeserializeMode mode) {
    Impl->DeserializeBase64(base64Tree, request, mode);
}

void TRichTreeDeserializer::DeserializeToProto(const TStringBuf& tree, bool base64, NRichTreeProtocol::TRichRequestNode& proto, bool humanReadable) {
    Impl->DeserializeToProto(tree, base64, proto, humanReadable);
}

