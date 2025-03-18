#include <yweb/robot/kiwi/protos/kwworm.pb.h>
#include <google/protobuf/messagext.h>
#include <library/cpp/charset/doccodes.h>
#include <util/generic/string.h>

int main(int, const char**)
{
    using namespace NKiwiWorm;
    using namespace google::protobuf::io;

    size_t countSkip = 0;
    size_t countGood = 0;

    TCopyingOutputStreamAdaptor outf(&Cout);
    TCopyingInputStreamAdaptor inf(&Cin);

    for (;;) {
        TRecord rec;
        if (!ParseFromZeroCopyStreamSeq(&rec, &inf)) {
            break;
        }
        TString url;
        bool hasKeyInv = false;
        bool hasArc = false;
        bool hasTag = false;
        bool hasEncoding = false;

        if (rec.HasKey()) {
            url = rec.GetKey();
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
                url = rawData;
            }
            else if (label == "Charset" || label == "Encoding") {
                ECharset encoding = (ECharset)((i8)(rawData[0]));
                hasEncoding = (encoding >= 0);
            }
            else {
                hasKeyInv = hasKeyInv || (label == "keyinv");
                hasArc = hasArc || (label == "arc");
                hasTag = hasTag || (label == "tag");
            }
        }

        if (!hasKeyInv || !hasArc || !hasTag || !hasEncoding || !url) {
            ++countSkip;
            Cerr << "Skipped document: " << url << Endl;
        }
        else {
            ++countGood;
            SerializeToZeroCopyStreamSeq(&rec, &outf);
        }
    }

    Cerr << "Good records: " << countGood << Endl;
    Cerr << "Discarded records: " << countSkip << Endl;
}

