#include "kiwi.h"

#include <yweb/robot/kiwi/protos/kwworm.pb.h>

namespace NHtmlStats
{

TProtoDoc::TProtoDoc()
    : Encoding(CODES_UNKNOWN)
{
}


TProtoDoc::~TProtoDoc()
{
}

bool TProtoStreamIterator::Next(TProtoDoc& result)
{
    using namespace NKiwiWorm;
    using namespace google::protobuf::io;

    TSimpleSharedPtr<TRecord> recPtr(new TRecord());
    TRecord& rec = *recPtr;

    if (!ParseFromZeroCopyStreamSeq(&rec, &ProtoStream)) {
        return false;
    }

    result = TProtoDoc();
    result.Rec = recPtr;

    if (rec.HasKey()) {
        result.Url = rec.GetKey();
    }

    for (size_t i = 0; i < rec.TuplesSize(); ++i) {
        TTuple t = rec.GetTuples((int)i);
        TString label = t.GetAttrName();
        const TString& rawData = t.GetRawData();

        if (t.HasInfo() && t.GetInfo().HasStatus()) {
            const TString& l = t.GetInfo().GetStatus().GetLabel();
            if (!!l) {
                label = l;
            }
        }
        if (!label || !rawData) {
            continue;
        }

        if (label == "Url" || label == "URL") {
            result.Url = rawData;
        }
        else if (label == "OriginalDocUdf") {
            result.Html = rawData;
        }
        else if (label == "Charset" || label == "Encoding") {
            result.Encoding = (ECharset)((i8)(rawData[0]));
        }
    }

    return true;
}

}
