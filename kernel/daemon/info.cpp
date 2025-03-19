#include "info.h"
#include "messages.h"

#include <library/cpp/logger/global/global.h>

#include <library/cpp/build_info/build_info.h>
#include <library/cpp/cpuload/cpu_load.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/vector.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <util/system/progname.h>
#include <util/system/mem_info.h>
#include <util/system/info.h>
#include <util/folder/filelist.h>
#include <util/stream/file.h>
#include <util/system/execpath.h>

struct TSlotsInfo {
    TSlotsInfo() {
        TFsPath path = "dump.json";
        if (!path.Exists())
            return;
        TUnbufferedFileInput fi(path);
        NJson::ReadJsonTree(&fi, &Dump);
    }
    static const TSlotsInfo Instance;
    NJson::TJsonValue Dump;
};

const TSlotsInfo TSlotsInfo::Instance;

TServerInfo GetCommonServerInfo() {
    TString programName = GetProgramName();
    programName = programName.substr(0, programName.find_first_of('.'));

    DEBUG_LOG << "Program name request result: " << programName << Endl;
    NMemInfo::TMemInfo mi = NMemInfo::GetMemInfo();
    NCpuLoad::TInfo cpu = NCpuLoad::Get();
    double la;
    NSystemInfo::LoadAverage(&la, 1);
    TServerInfo result;
    result.InsertValue("product", programName);
    result.InsertValue("Svn_root", GetArcadiaSourceUrl());
    result.InsertValue("Svn_revision", ToString(GetArcadiaLastChangeNum()));
    result.InsertValue("Svn_commit", GetProgramCommitId());
    result.InsertValue("Svn_author", GetArcadiaLastAuthor());
    result.InsertValue("Svn_branch", GetBranch());
    result.InsertValue("Svn_tag", GetTag());
    result.InsertValue("Build_date", GetProgramBuildDate());
    result.InsertValue("Build_host", GetProgramBuildHost());
    result.InsertValue("Build_type", GetBuildType());
    result.InsertValue("sandbox_task_id", GetSandboxTaskId());
    result.InsertValue("timestamp", Seconds());
    result.InsertValue("mem_size_real", Sprintf("%0.3f", mi.RSS * 1.0 / (1024 * 1024)));
    result.InsertValue("mem_size_virtual", Sprintf("%0.3f", mi.VMS * 1.0 / (1024 * 1024)));
    result.InsertValue("cpu_load_user", cpu.User);
    result.InsertValue("cpu_load_system", cpu.System);
    result.InsertValue("cpu_count", NSystemInfo::CachedNumberOfCpus());
    result.InsertValue("load_average", la);
    result.InsertValue("total_mem_size", NSystemInfo::TotalMemorySize());
    if (TSlotsInfo::Instance.Dump.IsDefined())
        result.InsertValue("slot", TSlotsInfo::Instance.Dump);
    return result;
}

TServerInfo CollectServerInfo(TAutoPtr<TCollectServerInfo> collector) {
    CHECK_WITH_LOG(collector);
    SendGlobalMessage(*collector);

    TServerInfo info = GetCommonServerInfo();
    collector->Fill(info);
    return info;
}
