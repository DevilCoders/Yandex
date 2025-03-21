#include "binaryguard.h"
#include <library/cpp/digest/md5/md5.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/system/fstat.h>


static inline bool FileExists(const TString& fileName) {
    return !fileName.empty() && NFs::Exists(fileName);
}

namespace NUtil {

    TString CalculateFileChecksum(const TString& file) {
        if (!FileExists(file))
            return TString();
        char outbuf[33];
        return TString(MD5::File(file.c_str(), outbuf));
    }

    TString CalculateDataChecksum(const TString& data) {
        MD5 md5;
        md5.Update(data.data(), data.size());
        char outbuf[33];
        return TString(md5.End(outbuf));
    }

    i64 LastModificationTime(const TString& path) {
        return TFileStat(path.c_str()).MTime;
    }

}   // namespace NUtil


static inline bool IsSameFilePath(const TString& f1, const TString& f2) {
    return TFsPath(f1).RealPath().Fix() == TFsPath(f2).RealPath().Fix();
}

inline bool TBinaryGuard::IsSrcFile(const TDependency& dep, const TString& srcFile) const {
    return !dep.IsBuiltin && IsSameFilePath(srcFile, GetDiskFileName(dep));
}


bool TBinaryGuard::IsBinaryUpToDate(const TString& srcFile, const TString& binFile) const {

    if (!FileExists(srcFile) || !FileExists(binFile))
        return false;

    i64 binModTime = NUtil::LastModificationTime(binFile);
    i64 srcModTime = NUtil::LastModificationTime(srcFile);
    if (srcModTime > binModTime)
        return false;

    TVector<TDependency> deps;
    if (!LoadDependencies(binFile, deps))
        return false;

    bool hasRoot = false;
    for (size_t i = 0; i < deps.size(); ++i)
        if (!deps[i].IsBuiltin) {
            TString diskFile = GetDiskFileName(deps[i]);
            if (!FileExists(diskFile) || NUtil::LastModificationTime(diskFile) > binModTime)
                return false;

            // check that @srcFile is a root dependency
            if (deps[i].IsRoot) {
                if (hasRoot || !IsSameFilePath(diskFile, srcFile))
                    return false;
                hasRoot = true;
            }
        }

//    return hasRoot;
    return true;
}


TBinaryGuard::EDependencyState TBinaryGuard::GetDependencyState(const TDependency& dep) const {
    // checksum was not computed on binary compilation for some reason, so now we cannot rely on it.
    if (dep.Checksum.empty())
        return NoChecksum;

    TString freshChecksum;
    if (!dep.IsBuiltin) {
        TString diskFile = GetDiskFileName(dep);
        if (!FileExists(diskFile))
            return FileMissing;
        freshChecksum = NUtil::CalculateFileChecksum(diskFile);
    } else
        freshChecksum = CalculateBuiltinChecksum(dep);

    return (freshChecksum == dep.Checksum) ? GoodChecksum : BadChecksum;
}

TBinaryGuard::ECheckState TBinaryGuard::CheckBinary(const TString& srcFile, const TString& binFile) const {
    if (!FileExists(srcFile) || !FileExists(binFile))
        return Undefined;

    TVector<TDependency> deps;
    if (!LoadDependencies(binFile, deps))
        return BadBinary;

    bool hasRoot = false;
    for (size_t i = 0; i < deps.size(); ++i) {
        switch (GetDependencyState(deps[i])) {
            case FileMissing:
            case NoChecksum:
                return Undefined;
            case BadChecksum:
                return BadBinary;
            case GoodChecksum:
                break;
            default:
                Y_ASSERT(false);
        }
        // check that @srcFile IS a root dependency
        if (deps[i].IsRoot) {
            if (hasRoot || !IsSrcFile(deps[i], srcFile))
                return BadBinary;
            hasRoot = true;
        }
    }

//    return hasRoot ? GoodBinary : BadBinary;
    return GoodBinary;
}

bool TBinaryGuard::CollectBadSources(const TString& srcFile, const TString& binFile, TVector<TString>& results, bool missingOrEmpty) const {
    if (!FileExists(srcFile)) {
        if (missingOrEmpty) {
            results.push_back(srcFile + " (not found)");
            return true;
        }
        return false;
    }

    if (!FileExists(binFile)) {
        if (missingOrEmpty) {
            results.push_back(binFile + " (not found)");
            return true;
        }
        return false;
    }

    TVector<TDependency> deps;
    if (!LoadDependencies(binFile, deps)) {
        results.push_back(binFile + " (cannot read binary: invalid format)");
        return true;
    }

    for (size_t i = 0; i < deps.size(); ++i) {
        TString name = deps[i].Name;
        EDependencyState state = GetDependencyState(deps[i]);
        if (state == BadChecksum) {
            if (deps[i].IsBuiltin)
                name += " (builtin)";
            results.push_back(name);
        } else if (missingOrEmpty && state != GoodChecksum) {
            if (state == FileMissing)
                results.push_back(name + " (not found)");
            else if (state == NoChecksum)
                results.push_back(name + " (no checksum)");
            else
                results.push_back(name + " (?)");
        }
    }
    return results.size() > 0;
}

bool TBinaryGuard::RebuildRequired(const TString& srcFile, const TString& binFile, ECompileMode mode) const {
    switch (mode) {
        case Optional:
            // always recompile if the binary is missing
            return !FileExists(binFile) || (FileExists(srcFile) && CheckBinary(srcFile, binFile) == BadBinary);
        case Normal:
            return CheckBinary(srcFile, binFile) != GoodBinary;
        default:
            return true;    // forced
    }
}


