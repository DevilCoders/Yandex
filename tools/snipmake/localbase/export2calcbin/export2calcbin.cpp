#include <yweb/robot/kiwi/protos/kwworm.pb.h>
#include <google/protobuf/messagext.h>
#include <util/stream/output.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

int main(int argc, const char* argv[])
{
    using TStrSet = THashSet<TString>;

    TStrSet paramNames;

    for (int i = 1; i < argc; ++i) {
        paramNames.insert(TString(argv[i]));
    }

    google::protobuf::io::TCopyingInputStreamAdaptor input(&Cin);
    google::protobuf::io::TCopyingOutputStreamAdaptor output(&Cout);
    size_t count = 0;

    for (;;)
    {
        NKiwiWorm::TRecord protoRec;
        if (!google::protobuf::io::ParseFromZeroCopyStreamSeq(&protoRec, &input)) {
            break;
        }

        NKiwiWorm::TCalcCallParams params;
        TString key = protoRec.GetKey();
        params.SetId(key);

        TStrSet foundParams;

        for (size_t i = 0; i < protoRec.TuplesSize(); ++i) {
            NKiwiWorm::TCalcParam* param = params.AddParams();
            const ::NKiwiWorm::TTuple& tuple = protoRec.GetTuples(static_cast<int>(i));
            param->SetName(tuple.GetAttrName());
            param->SetType(tuple.GetType());
            if (tuple.HasRawData()) {
                param->SetRawData(tuple.GetRawData());
            }
            else if (tuple.HasStringData()) {
                param->SetStringData(tuple.GetStringData());
            }
            else {
                // force IsNull(@Param) to return true
                param->SetRawData(TString());
                param->SetType(NKwTupleMeta::AT_UNDEF);
            }
            foundParams.insert(param->GetName());
        }

        for (const TString& name : paramNames) {
            if (foundParams.contains(name)) {
                continue;
            }
            NKiwiWorm::TCalcParam* param = params.AddParams();
            param->SetName(name);
            param->SetRawData(TString());
            param->SetType(NKwTupleMeta::AT_UNDEF);
        }

        if (!::google::protobuf::io::SerializeToZeroCopyStreamSeq(&params, &output)) {
            Cerr << "Failed to serialize/write calc params" << Endl;
        }

        ++count;
    }

    Cerr << count << " records written" << Endl;
}

