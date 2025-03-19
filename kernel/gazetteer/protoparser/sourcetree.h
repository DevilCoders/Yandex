#pragma once

#include "builtin.h"

#include <kernel/gazetteer/common/protohelpers.h>
#include <kernel/gazetteer/common/binaryguard.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGzt {

class TGztSourceTree: public TDiskSourceTree, public TBinaryGuard {
public:
    // if defaultBuiltins then NBuiltin::DefaultProtoFiles() are integrated
    TGztSourceTree(const TString& importPath = TString(), bool defaultBuiltins = true);
    TGztSourceTree(const TVector<TString>& importPath, bool defaultBuiltins = true);

    const NBuiltin::TFileCollection& BuiltinFiles() const {
        return BuiltinFiles_;
    }

    // Append @files to list of builtin files.
    void IntegrateFiles(const NBuiltin::TFileCollection& files);

    // Append TProto's descriptor file to builtins.
    template <typename TProto>
    void IntegrateBuiltinFile() {
        BuiltinFiles_.AddFileSafe<TProto>(TProto::descriptor()->file()->name());
    }

    // Treat as much files as possible as builtins.
    void MaximizeBuiltins() {
        BuiltinFiles_.Maximize();
    }

    TString CanonicName(const TString& name) const;
    TString Checksum(const TString& name) const;

    TBinaryGuard::TDependency AsDependency(const TString& name) const;

    using TDiskSourceTree::GetDiskFileName;

    void VerifyBinary(const TString& gztSrc, const TString& gztBin, bool missingOrEmpty = false);

private:
    // implements TBinaryGuard --------------------------------

    // Only loads list of dependencies files from binary file (*.gzt.bin) with their checksums
    bool LoadDependencies(const TString& binFile, TVector<TBinaryGuard::TDependency>& res) const override;
    TString GetDiskFileName(const TBinaryGuard::TDependency&) const override;
    TString CalculateBuiltinChecksum(const TBinaryGuard::TDependency&) const override;

private:
    class TGztBuiltinFiles: public NBuiltin::TFileCollection {
    public:
        TGztBuiltinFiles(bool defaultBuiltins);
    };

    TGztBuiltinFiles BuiltinFiles_;
};


}   // namespace NGzt
