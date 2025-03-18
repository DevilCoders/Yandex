#pragma once

#include <google/protobuf/messagext.h>
#include <library/cpp/charset/doccodes.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NKiwiWorm
{
    class TRecord;
}

namespace NHtmlStats
{

struct TProtoDoc
{
    TString Url;
    ECharset Encoding;
    TString Html;
    TSimpleSharedPtr<NKiwiWorm::TRecord> Rec;

    TProtoDoc();
    ~TProtoDoc();
};

class TProtoStreamIterator
{
    google::protobuf::io::TCopyingInputStreamAdaptor ProtoStream;

public:
    TProtoStreamIterator(IInputStream* inf)
        : ProtoStream(inf)
    {
    }

    bool Next(TProtoDoc& result);
};

}

