#pragma once
#include <util/generic/string.h>

namespace NKnnService {
    constexpr TStringBuf SaasComponentName = "HNSW";
    constexpr TStringBuf MultiRequestParamName = "multiReqBase64";

    //values in meta-responce
    constexpr TStringBuf ShardIdDocAttrName = "KnnShardId";
    constexpr TStringBuf RequestIdInDocAttrName = "RequestIdInMultiRequest";
    constexpr TStringBuf BodyGtaAttrName = "_Body";

    //legacy, component still parse them, but there no way to set them in request:
    constexpr TStringBuf TopSizeParamCgiName = "top_size";
    constexpr TStringBuf SearchSizeParamCgiName = "search_size";
    constexpr TStringBuf PackedEmbedParamCgiName = "embed_packed";
    constexpr TStringBuf DiscrModeCgiName = "discr_mode";
    constexpr TStringBuf TextEmbedParamCgiName = "embed_text";
    constexpr TStringBuf ModelFingerPrintParamCgiName = "embed_finger_print";
    constexpr TStringBuf Base64ProtectedFieldsParamCgiName = "base64_protect_gtafields";
    constexpr TStringBuf Base64EncodeFieldsParamCgiName = "base64_encode_fields";
}
