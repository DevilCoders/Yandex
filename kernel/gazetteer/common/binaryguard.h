#pragma once

#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>


namespace NUtil {
    // utility helpers
    i64 LastModificationTime(const TString& file);
    TString CalculateFileChecksum(const TString& file);
    TString CalculateDataChecksum(const TString& data);
}


class TBinaryGuard {
public:
    enum ECheckState {
        BadBinary,      // the binary is definitely compiled from sources other than currently available ones
        GoodBinary,     // the binary is fully consistent with all sources (and all sources are available at their remembered locations too)
        Undefined       // some of the sources (or the binary itself) cannot be found, it that case the state of the binary is undefined
    };

    // Returns flag displaying if @bin_file was built from @srcFile in its current exact state.
    // This check is similiar to IsBinaryUpToDate but more reliable in cases when
    // file modification time could be changed without real file modification.
    // This will read all source files and check their md5-hashes against checksum stored in @binFile.
    ECheckState CheckBinary(const TString& srcFile, const TString& binFile) const;



    // Same as CheckBinary, checks all sources and collects the list of files inconsistent with given binary.
    bool CollectBadSources(const TString& srcFile, const TString& binFile, TVector<TString>& results,
                           bool missingOrEmpty = false) const;


    // Returns true when specified binary is consistent with specified source file -
    // and therefore need not be re-compiled.
    inline bool IsGoodBinary(const TString& srcFile, const TString& binFile) const {
        return CheckBinary(srcFile, binFile) == GoodBinary;
    }

    // Returns true when specified binary file is inconsistent with specified source file.
    inline bool IsBadBinary(const TString& srcFile, const TString& binFile) const {
        return CheckBinary(srcFile, binFile) == BadBinary;
    }

    enum ECompileMode {
        Optional,    // compile only if the binary is missing or inconsistent with the sources.
        Normal,      // skip compilation only if the binary is consistent with all sources.
        Forced       // compile anyway
    };

    // to build or not to build - that's the question
    bool RebuildRequired(const TString& srcFile, const TString& binFile, ECompileMode mode) const;



    // Returns true if @srcFile is not newer than @binFile (as well as all imported gzt or proto files)
    // This would mean there is no need to re-build @srcFile in order to get its fresh up-to-date binary representation.
    bool IsBinaryUpToDate(const TString& srcFile, const TString& binFile) const;



    virtual ~TBinaryGuard() {
    }

    struct TDependency {
        TString Name;            // either builtin identity or virtual file name
        TString Checksum;
        bool IsBuiltin;         // a dependency could be a real file on disk or some builtin (virtual) object
        bool IsRoot;            // a root of compilation (main source gazetteer)

        inline TDependency(const TString& name, const TString& checksum, bool isbuiltin, bool isroot = false)
            : Name(name), Checksum(checksum), IsBuiltin(isbuiltin), IsRoot(isroot)
        {
        }
    };

private:
    enum EDependencyState {
        FileMissing,
        NoChecksum,
        BadChecksum,
        GoodChecksum
    };

    EDependencyState GetDependencyState(const TDependency& dep) const;
    inline bool IsSrcFile(const TDependency& dep, const TString& srcFile) const;

private:
    // TBinaryGuard interface

    virtual bool LoadDependencies(const TString& fileName, TVector<TDependency>& res) const = 0;

    virtual TString CalculateBuiltinChecksum(const TDependency&) const {
        return TString();
    }

    // For disk file return its path in file system (real path)
    virtual TString GetDiskFileName(const TDependency& dep) const {
        return dep.Name;
    }
};

