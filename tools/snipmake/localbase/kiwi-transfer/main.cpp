#include <yweb/robot/kiwi/clientlib/client.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/vector.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/event.h>
#include <util/system/mutex.h>

TString SOURCE_HOST = "kiwi.yandex.net";
int SOURCE_PORT = 31402;

TString DESTINATION_HOST = "localhost";
int DESTINATION_PORT = 29402;

class TImprovedKiwiSession : public NKiwi::TSession {
public:
    template <typename... Params>
    TImprovedKiwiSession(Params... params)
        : NKiwi::TSession(params...)
    {
        InitBranchInfo();
        InitAttrInfo();
    }

    TString GetFullObjectRequest(
            ui8 keytype,
            TVector<TString> branches)
    {
        TStringStream request;
        bool first = true;
        char sep = ' ';
        request << "return\n";
        for (auto branch : branches) {
            TVector<TString> attrs = ListAttributes(keytype, branch);
            for (auto attr : attrs) {
                request << '\t' << sep << " ?" << attr << ":" << branch << '\n';
                if (first) {
                    first = false;
                    sep = ',';
                }
            }
        }
        request << ";\n";
        return request.Str();
    }

    TVector<TString> ListAttributes(ui8 keytype, const TString& branch) {
        TVector<ui16> attrIdList;
        GetBranchAttrs(keytype, Branch2Id.at(branch), attrIdList);
        TVector<TString> result;
        for (auto attrId : attrIdList) {
            TString name;
            NKwTupleMeta::EAttrType type;
            GetAttrInfo(attrId, name, type);
            result.push_back(name);
        }
        return result;
    }

    TString GetAttrName(ui8 keytype, ui16 branchId, ui16 attrId) const {
        return KeyTypeBranchIdAttrId2Name.at(keytype).at(branchId).at(attrId);
    }

    ui16 GetAttrId(ui8 keytype, ui16 branchId, const TString& attrName) const {
        return KeyTypeBranchIdName2AttrId.at(keytype).at(branchId).at(attrName);
    }

    bool HasAttrWithName(ui8 keytype, ui16 branchId, const TString& attrName) const {
        return KeyTypeBranchIdName2AttrId.at(keytype).contains(branchId)
               && KeyTypeBranchIdName2AttrId.at(keytype).at(branchId).contains(attrName);
    }

    NKwTupleMeta::EAttrType GetAttrType(ui8 keytype, ui16 branchId, ui16 attrId) const {
        return KeyTypeBranchIdAttrId2AttrType.at(keytype).at(branchId).at(attrId);
    }

private:
    void InitBranchInfo() {
        TVector<ui16> branches;
        ListBranches(branches);
        for (auto branchId : branches) {
            TString branchName = GetBranchName(branchId);
            Id2Branch[branchId] = branchName;
            Branch2Id[branchName] = branchId;
        }
    }

    void InitAttrInfo() {
        TVector<ui16> branches;
        ListBranches(branches);
        TVector<ui8> keytypeList;
        keytypeList.push_back(10); //KT_HOST
        keytypeList.push_back(50); //KT_DOC_DEF
        keytypeList.push_back(70); //KT_IMAGE
        for (const auto& keytype : keytypeList) {
            for (const auto& branchid : branches) {
                TVector<ui16> attrIdList;
                GetBranchAttrs(keytype, branchid, attrIdList);
                for (const auto& attrId : attrIdList) {
                    TString attrName;
                    NKwTupleMeta::EAttrType type;
                    GetAttrInfo(attrId, attrName, type);
                    KeyTypeBranchIdAttrId2Name[keytype][branchid][attrId] = attrName;
                    KeyTypeBranchIdName2AttrId[keytype][branchid][attrName] = attrId;
                    KeyTypeBranchIdAttrId2AttrType[keytype][branchid][attrId] = type;
                }
            }
        }
    }

private:
    THashMap<ui16, TString> Id2Branch;
    THashMap<TString, ui16> Branch2Id;

    THashMap<ui8, THashMap<ui16, THashMap<ui16, TString>>> KeyTypeBranchIdAttrId2Name;
    THashMap<ui8, THashMap<ui16, THashMap<ui16, NKwTupleMeta::EAttrType>>> KeyTypeBranchIdAttrId2AttrType;
    THashMap<ui8, THashMap<ui16, THashMap<TString, ui16>>> KeyTypeBranchIdName2AttrId;
};

NKiwi::TKiwiObject CopyObject(
        const TString& key,
        const NKiwi::TKiwiObject& srcObject,
        ui8 keyType,
        TImprovedKiwiSession& srcSession,
        TImprovedKiwiSession& dstSession)
{
    const TInstant now = TInstant::Now();
    NKiwi::TKiwiObject result;
    NKiwi::NTuples::TTupleIterator iter = srcObject.GetIterator();
    for (; iter.IsValid(); iter.Advance()) {
        const auto tuple = iter.Current();
        if (tuple->IsNull()) {
            continue;
        }
        ui16 attrId = tuple->GetId();
        ui16 branchId = tuple->GetVer();
        if (srcSession.GetAttrType(keyType, branchId, attrId) == NKwTupleMeta::AT_GENERIC_TABLE) {
            continue;
        }
        const TString& attrName = srcSession.GetAttrName(keyType, branchId, attrId);
        if (!dstSession.HasAttrWithName(keyType, branchId, attrName)) {
            Cerr << "WARN/WRITE\t" << key << "\t" << "Attribute is not defined in destination kiwi: " << attrName << ":" << branchId << Endl;
            continue;
        }
        ui16 newAttrId = dstSession.GetAttrId(keyType, branchId, attrName);
        result.Add(newAttrId, branchId, now.Seconds(), tuple->GetData(), tuple->GetSize());
    }
    return result;
}

bool CopyObject(const TString& key,
        ui8 keytype,
        TImprovedKiwiSession& srcSession,
        TImprovedKiwiSession& dstSession,
        const TString& query)
{
    NKiwi::TSyncReader reader(srcSession, keytype, query, nullptr, 1);
    NKiwi::TKiwiObject object;
    NKiwi::TReaderBase::EReadStatus status = reader.Read(key, object);
    if (status != NKiwi::TReaderBase::READ_OK) {
        Cerr << "ERROR/READ\t" << key << "\t" << NKiwi::TReaderBase::StatusText(status) << Endl;
        return false;
    }
    NKiwi::TKiwiObject copyObject = CopyObject(
            key, object, keytype, srcSession, dstSession);
    NKiwi::TSyncWriter syncWriter(dstSession);
    int code;
    TString explanation;
    NKiwi::TSyncWriter::EWriteStatus wStatus = syncWriter.Write(
            keytype, key, copyObject, code, explanation, true);
    if (wStatus != NKiwi::TSyncWriter::WRITE_OK) {
        Cerr << "ERROR/WRITE\t" << key << "\t" << NKiwi::TSyncWriter::StatusText(wStatus)
             << "; " << explanation << Endl;
        return false;
    }
    Cerr << "OK\t" << key << Endl;
    return true;
}

int main(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption('c', "server", "source server").RequiredArgument("SERVER").DefaultValue("kiwi.yandex.net");
    opts.AddLongOption('p', "port", "source port").RequiredArgument("PORT").DefaultValue("31402");
    opts.AddLongOption('d', "dest-server", "destination server").RequiredArgument("SERVER").DefaultValue("localhost");
    opts.AddLongOption('t', "dest-port", "destination port").RequiredArgument("PORT").DefaultValue("31402");
    opts.AddLongOption('k', "keytype", "key type").RequiredArgument("KEYTYPE").DefaultValue("50");
    opts.AddLongOption('b', "branch-list", "comma separated branch list").RequiredArgument("BRANCHES").DefaultValue("TRUNK,RUS,USA,TUR");

    opts.AddHelpOption('h');
    opts.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
    const int sourcePort = parseResult.Get<int>("port");
    const TString sourceHost = parseResult.Get("server");
    const int destinationPort = parseResult.Get<int>("dest-port");
    const TString destinationHost = parseResult.Get<TString>("dest-server");

    const TString user = "any";

    const ui8 keyType = parseResult.Get<int>("keytype");
    const TVector<TString> branchList = SplitString(parseResult.Get<TString>("branch-list"), ",");

    TImprovedKiwiSession sourceKiwi(sourceHost, user, sourcePort);
    TImprovedKiwiSession destinationKiwi(destinationHost, user, destinationPort);

    TString query = sourceKiwi.GetFullObjectRequest(keyType, branchList);

    TString key;
    int goodCount = 0;
    int badCount = 0;
    while (Cin.ReadLine(key)) {
        if (CopyObject(key, keyType, sourceKiwi, destinationKiwi, query)) {
            ++goodCount;
        }
        else {
            ++badCount;
        }
    }
    Cerr << "STATS\t" << goodCount << "\t" << badCount << Endl;
    return (badCount * 10) > (goodCount + badCount);
}

