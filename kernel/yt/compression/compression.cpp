#include "compression.h"

#include <kernel/yt/attrs/attrs.h>
#include <kernel/yt/utils/yt_utils.h>

#include <util/generic/yexception.h>

namespace NJupiter {
    TString GetCompressionCodecName(ECompressionCodec codec) {
        return ToString(codec);
    }

    TString GetErasureCodecName(EErasureCodec codec) {
        return ToString(codec);
    }

    NYT::TNode::TMapType GetCompressionStats(NYT::IClientBasePtr client, const TString& path) {
        return GetYtAttr(client, path, TYtAttrName::CompressionStatistics).AsMap();
    }

    ECompressionCodec GetCompressionCodecAttr(NYT::IClientBasePtr client, const TString& path) {
        return FromString<ECompressionCodec>(GetYtAttr(client, path, TYtAttrName::CompressionCodec).AsString());
    }

    ECompressionCodec GetCompressionCodec(NYT::IClientBasePtr client, const TString& path) {
        NYT::TNode::TMapType codecsNode = GetYtAttr(client, path, TYtAttrName::CompressionStatistics).AsMap();
        const i64 compressedDataSize = GetYtAttr(client, path, TYtAttrName::CompressedDataSize).AsInt64();
        if (codecsNode.size() == 0) {
            if (compressedDataSize == 0) {
                return FromString<ECompressionCodec>(GetYtAttr(client, path, TYtAttrName::CompressionCodec).AsString());
            }
            ythrow yexception() << "Compression statistics is empty, but compressed_data_size > 0.";
        } else {
            Y_ENSURE(codecsNode.size() == 1, "Table has chunks compressed with different codecs. Use GetCompressionStats for this case.");
            return FromString<ECompressionCodec>(codecsNode.begin()->first);
        }
    }

    NYT::TNode::TMapType GetErasureStats(NYT::IClientBasePtr client, const TString& path) {
        return GetYtAttr(client, path, TYtAttrName::ErasureStatistics).AsMap();
    }

    EErasureCodec GetErasureCodecAttr(NYT::IClientBasePtr client, const TString& path) {
        return FromString<EErasureCodec>(GetYtAttr(client, path, TYtAttrName::ErasureCodec).AsString());
    }

    EErasureCodec GetErasureCodec(const NYT::TNode& node) {
        NYT::TNode::TMapType codecsNode = GetYtNodeAttr(node, TYtAttrName::ErasureStatistics).AsMap();
        const i64 compressedDataSize = GetYtNodeAttr(node, TYtAttrName::CompressedDataSize).AsInt64();
        if (codecsNode.size() == 0) {
            if (compressedDataSize == 0) {
                return FromString<EErasureCodec>(GetYtNodeAttr(node, TYtAttrName::ErasureCodec).AsString());
            }
            ythrow yexception() << "Erasure statistics is empty, but compressed_data_size > 0.";
        } else {
            Y_ENSURE(codecsNode.size() == 1, "Table has chunks compressed with different codecs. Use GetErasureStats for this case.");
            return FromString<EErasureCodec>(codecsNode.begin()->first);
        }
    }

    EErasureCodec GetErasureCodec(NYT::IClientBasePtr client, const TString& path) {
        NYT::TAttributeFilter filter;
        filter.AddAttribute(TYtAttrName::ErasureStatistics);
        filter.AddAttribute(TYtAttrName::CompressedDataSize);
        filter.AddAttribute(TYtAttrName::ErasureCodec);

        return GetErasureCodec(client->Get(path, NYT::TGetOptions().AttributeFilter(filter)));
    }

    void SetCompressionCodec(NYT::IClientBasePtr client, const TString& path, ECompressionCodec codec) {
        SetYtAttr(client, path, TYtAttrName::CompressionCodec, GetCompressionCodecName(codec));
    }

    void SetErasureCodec(NYT::IClientBasePtr client, const TString& path, EErasureCodec codec) {
        SetYtAttr(client, path, TYtAttrName::ErasureCodec, GetErasureCodecName(codec));
    }

}
