#include "proto_helpers.h"

#include <library/cpp/protobuf/util/pb_io.h>

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TNullErrorCollector
    : public google::protobuf::io::ErrorCollector
{
    void AddError(
        int line,
        int column,
        const google::protobuf::string& message) override
    {
        Y_UNUSED(line);
        Y_UNUSED(column);
        Y_UNUSED(message);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool TryParseProtoTextFromStringWithoutError(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);
    NProtoBuf::TextFormat::Parser parser;

    TNullErrorCollector nullErrorCollector;
    parser.RecordErrorsTo(&nullErrorCollector);

    if (!parser.Parse(&adaptor, &dst)) {
        // remove everything that may have been read
        dst.Clear();
        return false;
    }

    return true;
}

bool TryParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    return TryParseFromTextFormat(in, dst);
}

bool TryParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    if (!TFsPath(fileName).Exists()) {
        return false;
    }

    TFileInput in(fileName);
    return TryParseFromTextFormat(in, dst);
}

bool IsReadWriteMode(const NProto::EVolumeAccessMode mode)
{
    switch (mode) {
        case NProto::VOLUME_ACCESS_READ_ONLY:
        case NProto::VOLUME_ACCESS_USER_READ_ONLY:
            return false;
        case NProto::VOLUME_ACCESS_READ_WRITE:
        case NProto::VOLUME_ACCESS_REPAIR:
            return true;
        default:
            Y_VERIFY_DEBUG(false, "Unknown EVolumeAccessMode: %d", mode);
            return false;
    }
}

}   // namespace NCloud::NBlockStore
