
#include <yweb/robot/kiwi/kwcalc/udflib/udflib.h>

#include <kernel/walrus/advmerger.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/system/yassert.h>

using namespace NUdfLib;

TStringBuf ReadBuf(TStringBuf& raw) {
    Y_ASSERT(raw.size() >= sizeof(ui32));
    const ui32 bufferSize = *reinterpret_cast<const ui32*>(raw.data());
    raw.Skip(sizeof(ui32));
    Y_ASSERT(raw.size() >= bufferSize);
    TStringBuf res(raw.data(), bufferSize);
    raw.Skip(bufferSize);
    return res;
}
void ReadKeyInvs(TStringBuf raw, TVector<TPortionBuffers>& res) {
    while (raw.size()) {
        TStringBuf key = ReadBuf(raw);
        TStringBuf inv = ReadBuf(raw);
        res.push_back(TPortionBuffers(key.data(), key.size(), inv.data(), inv.size()));
    }
}
void WriteBuf(TString& res, TStringBuf s) {
    ui32 len = s.size();
    res.append(reinterpret_cast<const char*>(&len), sizeof(len));
    res.append(s);
}
void WriteKeyInv(TString& res, TPortionBuffers& buf) {
    WriteBuf(res, TStringBuf(reinterpret_cast<const char*>(buf.KeyBuf), buf.KeyBufSize));
    WriteBuf(res, TStringBuf(reinterpret_cast<const char*>(buf.InvBuf), buf.InvBufSize));
}

class TFakeMergePortionsTrigger {
public:
    explicit TFakeMergePortionsTrigger(const char* dataPath) {
        Y_UNUSED(dataPath);
    }

    int Run(const TParamList& input, TReturnValues& retVals) const
    {
        size_t argsNeeded = 1;
        if (input.Size() != argsNeeded)
            ythrow yexception() << "FakeMergePortionsTrigger: invalid number of parameters: " << input.Size() << ", expected " << argsNeeded;

        TStringBuf keyinvs = NUdfLib::GetRawData(input[0].Get());

        TVector<TPortionBuffers> portions;
        ReadKeyInvs(keyinvs, portions);
        NIndexerCore::TMemoryPortion res(IYndexStorage::FINAL_FORMAT);
        if (!portions.empty()) {
            MergeMemoryPortions(&portions[0], portions.size(), IYndexStorage::FINAL_FORMAT, nullptr, false, res);
        }
        TPortionBuffers buf(&res);

        TString s;
        WriteKeyInv(s, buf);

        retVals.Add(MakeStringValue(s, NKwTupleMeta::AT_BLOB));
        return 0;
    }
};

DECLARE_UDF(TFakeMergePortionsTrigger)
