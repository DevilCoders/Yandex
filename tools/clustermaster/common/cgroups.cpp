#include "cgroups.h"

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/split.h>

TString GetMountPath(const TString &cgroup) {
    // format: fstype mountpath mounttype mountoptions
    TUnbufferedFileInput in("/proc/mounts");
    TString procMount;

    while(in.ReadLine(procMount)) {
        TVector<TString> parts;
        Split(procMount, " ", parts);

        TString mountPath(parts[1]);
        TString mountType(parts[2]);
        TString mountOptions(parts[3]);

        if (mountType != "cgroup") {
            continue;
        }
        TVector<TString> mountOptsArray;
        Split(mountOptions, ",", mountOptsArray);

        for (const auto& opt: mountOptsArray) {
            if (opt == cgroup) {
                return mountPath;
            }
        }
    }
    ythrow yexception() << "cgroups " << cgroup << " mount path was not found";
}

size_t GetCgroupVariable(const TString &cgroup, const TString &var) {
    //  format: groupid:groupname:grouprelpath
    TUnbufferedFileInput in("/proc/self/cgroup");
    TString procGroup;

    while (in.ReadLine(procGroup)) {
        TVector<TString> parts;
        Split(procGroup, ":", parts);

        TString groupName(parts[1]);
        TString groupRelPath(parts[2]);

        if (groupName != cgroup) {
            continue;
        }
        size_t value = 0;
        TString varFileName = TStringBuilder() << GetMountPath(cgroup) << "/" << groupRelPath << "/" << var;
        TUnbufferedFileInput varInput(varFileName);
        varInput >> value;
        return value;
    }
    ythrow yexception() << "cgroups " << cgroup << " variable " << var << " was not found";
}
