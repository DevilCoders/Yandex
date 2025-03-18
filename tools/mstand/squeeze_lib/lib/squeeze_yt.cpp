#include "squeeze_yt.h"


namespace NMstand {

void ProcessUserSessionsRecord(const NYT::TNode& row, NRA::TLogsParser& lp) {
    const TString key = row["key"].AsString();
    const TString subKey = row["subkey"].AsString();
    const TString value = row["value"].AsString();
    time_t ts;
    if (!subKey.empty() && !TryFromString<time_t>(subKey, ts)) {
        return;
    }
    lp.AddRec(key, subKey, value);
}

inline TString GetSubkey(const NUserSessions::NProto::TTotalEvent& row) {
    return row.GetSubkey();
}

inline TString GetSubkey(const NYT::TNode& row) {
    return row["subkey"].AsString();
}

inline TString GetValue(const NUserSessions::NProto::TTotalEvent& row) {
    return row.GetValue();
}

inline TString GetValue(const NYT::TNode& row) {
    return row["value"].AsString();
}

static const THashSet<TString> TAB_VALUE_SUBKEYS = {" yuid_testids"};
static const THashSet<TString> JSON_VALUE_SUBKEYS = {" desktop_bro_exp", " bro_exp"};

template <class TReader>
THashSet<TString> GetAvailableTestids(TReader* reader, const THashSet<TString>& testids) {
    THashSet<TString> result;
    if (testids.contains("0")) {
        result.insert("0");
    }
    for (; reader->IsValid(); reader->Next()) {
        auto row = reader->GetRow();
        auto subkey = GetSubkey(row);
        TVector<TString> splittedValue = StringSplitter(GetValue(row)).Split('\t');
        if (TAB_VALUE_SUBKEYS.contains(subkey)) {
            for (const auto& testid : splittedValue) {
                TString testidStr(testid);
                if (testids.contains(testidStr)) {
                    result.insert(testidStr);
                }
            }
        }
        else if (JSON_VALUE_SUBKEYS.contains(subkey)) {
            NSc::TValue jsonValue;
            if (splittedValue.size() == 0 || !NSc::TValue::FromJson(jsonValue, splittedValue[0])) {
                continue;
            }
            for (const auto& testid : jsonValue.GetArray()) {
                if (testid.IsString()) {
                    auto testidStr = "mbro__" + TString(testid.GetString());
                    if (testids.contains(testidStr)) {
                        result.insert(testidStr);
                    }
                }
            }
        }
        else {
            break;
        }
    }
    return result;
}

///////////////////////////////////////// TUserSessionsReducerKSV

TUserSessionsReducerKSV::TUserSessionsReducerKSV(const TUserSessionsSqueezer& squeezer)
    : Squeezer(squeezer)
{}

void TUserSessionsReducerKSV::Start(TWriter* /*writer*/) {
    BlockStatDict = TBlockStatInfo("blockstat.dict");
    Squeezer.Init();
}

void TUserSessionsReducerKSV::Do(TReader* reader, TWriter* writer) {
    auto yuid = reader->GetRow()["key"].AsString();
    auto availableTestids = GetAvailableTestids(reader, Squeezer.GetTestids());
    if (availableTestids.empty()) {
        return;
    }

    if(!reader->IsValid()){
        return;
    }

    NRA::TLogsParserParams lpParams(BlockStatDict);
    lpParams.SetErrorHandler(new NRA::TCerrLogsParserErrorHandler(true, false));

    NRA::TLogsParser lp(lpParams);
    try {
        TVector<NYT::TNode> rows;
        for (; reader->IsValid(); reader->Next()) {
            NYT::TNode row = reader->GetRow();
            ProcessUserSessionsRecord(row, lp);
            if (lp.IsFatUser()) {
                Cerr << "Failed parsing session, yuid: " << yuid << Endl;
                return;
            }
            rows.push_back(row);
        }
        if (!rows.empty()) {
            lp.Join();
        }

        auto container = lp.GetRequestsContainer();
        for(const auto& action : Squeezer.SqueezeSessions(container, rows, THashMap<TString, NRA::TFilterPack>(), availableTestids)) {
            writer->AddRow(action.Action, action.Experiment.TableIndex);
        }
    } catch (const yexception &ex) {
        Cerr << "Exception in TUserSessionsReducerKSV::Do, yuid: " << yuid << ": " << ex.what() << Endl;
    } catch (...) {
        Cerr << "Unknown exception in TUserSessionsReducerKSV::Do, yuid: " << yuid << Endl;
    }
}

REGISTER_REDUCER(TUserSessionsReducerKSV);

///////////////////////////////////////// TUserSessionsReducerKSV END

///////////////////////////////////////// TUserSessionsReducer

TUserSessionsReducer::TUserSessionsReducer(const TUserSessionsSqueezer& squeezer, const NRA::TEntitiesManager& entManager, const TFilterMap& rawUid2Filter)
    : Squeezer(squeezer)
    , EntManager(entManager)
    , RawUid2Filter(rawUid2Filter)
{}

void TUserSessionsReducer::Start(TWriter* /*writer*/) {
    BlockStatDict = TBlockStatInfo("blockstat.dict");
    Squeezer.Init();
    InitFilters();
}

class TAddRecStatWriter {
public:
    TAddRecStatWriter()
        : PreAddRecTime(TInstant::Now())
    {}

    ~TAddRecStatWriter() {
        try {
                NYT::WriteCustomStatistics(ADDREC_PRECISE_STAT_NAME, (TInstant::Now() - PreAddRecTime).MicroSeconds());
                NYT::FlushCustomStatisticsStream();
        } catch(const yexception&) {
        }
    }
private:
    TInstant PreAddRecTime;
    static constexpr TStringBuf ADDREC_PRECISE_STAT_NAME ="ralib_stats/addrec_precise_time";
}; 

void TUserSessionsReducer::Do(TProtoSessionReader* reader, TWriter* writer) {

    auto yuid = reader->GetRow().GetKey();
    auto availableTestids = GetAvailableTestids(reader, Squeezer.GetTestids());
    if (availableTestids.empty()) {
        return;
    }

    if(!reader->IsValid()){
        return;
    }

    NRA::TLogsParserParams lpParams(BlockStatDict);
    lpParams.SetErrorHandler(new NRA::TCerrLogsParserErrorHandler(true, false));
    lpParams.SetEntitiesManager(EntManager);
    NRA::TLogsParser lp(lpParams);

    if (!ParseAndCheckFatUsersUnsafe(lp, reader)) {
        Cerr << "Failed parsing session, yuid: " << yuid << Endl;
        return;
    }

    try {
        auto container = lp.GetRequestsContainer();
        for(const auto& action : Squeezer.SqueezeSessions(container, TVector<NYT::TNode>(), Filters, availableTestids)) {
            writer->AddRow(action.Action, action.Experiment.TableIndex);
        }
    } catch (const yexception &ex) {
        Cerr << "Exception in TUserSessionsReducer::Do, yuid: " << yuid << ": " << ex.what() << Endl;
    } catch (...) {
        Cerr << "Unknown exception in TUserSessionsReducer::Do, yuid: " << yuid << Endl;
    }
}

void TUserSessionsReducer::InitFilters() {
    if (RawUid2Filter.empty()) {
        return;
    }
    NRA::TRequestFilterFactory::Instance().InitRegionsDB("./geodata4.bin");
    int badFilterCount = 0;
    for (const auto& item : RawUid2Filter) {
        NRA::TFilterPack filterPack;
        bool isValid = true;
        for (const auto& fltr : item.second) {
            auto filter = NRA::TRequestFilterFactory::Instance().Create(fltr.Name, fltr.Value);
            isValid &= filter != nullptr;
            filterPack.Add(filter);
        }
        if (isValid) {
            filterPack.Init();
            Filters[item.first] = filterPack;
        }
        else {
            badFilterCount++;
        }
    }
    Cerr << "Filter info: get " << RawUid2Filter.size() << ", skip " << badFilterCount << ", apply " << Filters.size() << Endl;
}

REGISTER_REDUCER(TUserSessionsReducer);

///////////////////////////////////////// TUserSessionsReducer END

NYT::TRichYPath GetInputTable(const TString& path, const TString& lowerKey, const TString& upperKey) {
    auto inputPath = NYT::TRichYPath(path);
    if (!lowerKey.empty() || !upperKey.empty()) {
        auto range = NYT::TReadRange();
        if (!lowerKey.empty()) {
            range.LowerLimit(NYT::TReadLimit().Key(lowerKey));
        }
        if (!upperKey.empty()) {
            range.UpperLimit(NYT::TReadLimit().Key(upperKey));
        }
        inputPath.AddRange(range);
    }
    return inputPath;
}

NYT::TRichYPath GetOutputTable(const NMstand::TExperimentForSqueeze& exp) {
    auto squeezer = NMstand::TSqueezerFactory::GetSqueezer(exp.Service);
    auto outputPath = NYT::TRichYPath()
        .Path(exp.TempTablePath)
        .Schema(squeezer->GetSchema());
    return outputPath;
}

NYT::TReduceOperationSpec CreateReduceSpec(const TVector<NMstand::TExperimentForSqueeze>& experiments, const NMstand::TYtParams& ytParams) {
    auto jobSpec = NYT::TUserJobSpec()
        .MemoryLimit(ytParams.MemoryLimit);

    for(const auto& filename : ytParams.YtFiles) {
        jobSpec.AddFile(filename);
    }

    auto spec = NYT::TReduceOperationSpec()
        .ReducerSpec(jobSpec)
        .ReduceBy({"key"})
        .DataSizePerJob(ytParams.DataSizePerJob);

    for (const auto& inputTable : ytParams.YuidPaths) {
        spec.AddInput<NYT::TNode>(GetInputTable(inputTable, ytParams.LowerKey, ytParams.UpperKey));
    }
    for (const auto& inputTable : ytParams.SourcePaths) {
        spec.AddInput<NYT::TNode>(GetInputTable(inputTable, ytParams.LowerKey, ytParams.UpperKey));
    }
    for (const auto& exp : experiments) {
        spec.AddOutput<NYT::TNode>(GetOutputTable(exp));
    }

    return spec;
}

NYT::TReduceOperationSpec CreateReduceSpec(
    const TVector<NMstand::TExperimentForSqueeze>& experiments,
    const NMstand::TYtParams& ytParams,
    const NRA::TEntitiesManager& entManager,
    const NYT::ITransactionPtr& tx
) {
    auto jobSpec = NYT::TUserJobSpec()
        .MemoryLimit(ytParams.MemoryLimit);

    for(const auto& filename : ytParams.YtFiles) {
        jobSpec.AddFile(filename);
    }

    auto spec = NYT::TReduceOperationSpec()
        .ReducerSpec(jobSpec)
        .ReduceBy({"key"})
        .DataSizePerJob(ytParams.DataSizePerJob);


    TVector<NYT::TRichYPath> richYPaths;
    bool isJoin=false;
    for (const auto& inputTable : ytParams.YuidPaths) {
        richYPaths.push_back(GetInputTable(inputTable, ytParams.LowerKey, ytParams.UpperKey).Foreign(true));
        isJoin=true;
    }
    for (const auto& inputTable : ytParams.SourcePaths) {
        richYPaths.push_back(GetInputTable(inputTable, ytParams.LowerKey, ytParams.UpperKey));
    }
    if (isJoin){
        spec.JoinBy({"key"});
    }

    NUserSessions::SetOperationInputInfo(spec, richYPaths, entManager.GetOrderedColumns(), tx);

    for (const auto& exp : experiments) {
        spec.AddOutput<NYT::TNode>(GetOutputTable(exp));
    }
    
    return spec;
}

NYT::TOperationOptions CreateOperationSpec(const TYtParams& ytParams) {
    auto rawOpSpec = NYT::TNode::CreateMap();

    if (ytParams.TentativeEnable) {
        rawOpSpec["tentative_pool_trees"] = NYT::TNode::CreateList().Add("cloud");
        rawOpSpec["pool_trees"] = NYT::TNode::CreateList().Add("physical");
    }

    if (!ytParams.Pool.empty()) {
        rawOpSpec["pool"] = ytParams.Pool;
    }

    if (ytParams.AddAcl) {
        auto acl = NYT::TNode::CreateList();
        acl.Add(NYT::TNode::CreateMap()
            ("action", "allow")
            ("permissions", NYT::TNode::CreateList().Add("read").Add("manage"))
            ("subjects", NYT::TNode::CreateList().Add("idm-group:44732"))
        );
        rawOpSpec["acl"] = acl;
    }

    rawOpSpec["max_data_size_per_job"] = ytParams.MaxDataSizePerJob;

    if (rawOpSpec.Empty()) {
        return NYT::TOperationOptions();
    }
    return NYT::TOperationOptions{}.Spec(rawOpSpec);
}

};
