#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/compiler/importer.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/hash.h>

namespace NGzt {

typedef NProtoBuf::FileDescriptor TFileDescriptor;

namespace NBuiltin {

    const TFileDescriptor* FindInGeneratedPool(const TString& name);

    class TFile: public TSimpleRefCount<TFile> {
    public:
        explicit TFile(const TString& name);

        TFile& AddAlias(const TString& name);            // automatically adds .gztproto alias to .proto and vice verse
        void MergeAliases(const TFile& file);

        const TString& Name() const {
            return Aliases_[0];
        }

        const TFileDescriptor* FindInGeneratedPool() const {
            return GeneratedDescriptor;
        }

        const TString& GeneratedPoolName() const {
            // could be different with Name()
            return GeneratedDescriptor ? GeneratedDescriptor->name() : Name();
        }

        const TVector<TString>& Aliases() const {
            return Aliases_;
        }

    private:
        bool Is(const TStringBuf& name) const;
        void AddAliasImpl(const TString& name);

    private:
        TVector<TString> Aliases_;
        const TFileDescriptor* GeneratedDescriptor; // could be NULL!
    };


    typedef TIntrusivePtr<TFile> TRef;
    typedef TIntrusiveConstPtr<TFile> TConstRef;


    class TFileCollection : TNonCopyable {
    public:
        TFileCollection()
            : Maximized(false)
        {
        }

        // adding of ready files
        const TFile& Merge(const TFile& file);
        void MergeFiles(const TFileCollection& files);

        // construct and merge
        const TFile& AddFile(const TString& name, const TString& alias1 = TString(), const TString& alias2 = TString());

        // This method is guaranteed to find specified descriptor parent file descriptor in generated pool.
        // If not present it will run protobuf static descriptors registration
        template <typename TProto>
        const TFile& AddFileSafe(const TString& alias1 = TString(), const TString& alias2 = TString()) {
            // this will register TGeneratedProto in generated_pool if this did not run yet
            if (Y_UNLIKELY(TProto::descriptor() == nullptr))
                ythrow yexception() << "TFileCollection::AddFileSafe() failed: TGeneratedProto descriptor is not not linked into executable";
            return AddFile(TProto::descriptor()->file(), alias1, alias2);
        }

        // Add whole @file descriptor (probably from generated_pool) to file collection.
        // The name of this file will be canonical name for added file.
        // Also all dependencies (imports) are added recursively.
        const TFile& AddFile(const TFileDescriptor* file, const TString& alias1 = TString(), const TString& alias2 = TString()) {
            for (int i = 0; i < file->dependency_count(); ++i)
                AddFile(file->dependency(i));   // without aliases
            return AddFile(file->name(), alias1, alias2);
        }

        size_t Size() const {
            return Files.size();
        }

        const TFile& operator[] (size_t index) const {
            return *Files[index];
        }

        // treat all generated_pool as builtin files
        void Maximize() {
            Maximized = true;
        }

        TConstRef Find(const TStringBuf& name) const;
        const TFileDescriptor* FindFileDescriptor(const TStringBuf& name) const;

        // either find explicit or add implicit and return it.
        const TFile* FindOrAdd(const TString& name);

        bool Has(const TStringBuf& name) const {
            return Find(name).Get() != nullptr;
        }

        TString DebugString() const;

    private:
        TFile* FindByAlias(const TFile& file);
        TRef FindExplicit(const TStringBuf& name) const;
        TRef FindImplicit(const TString& name) const;
        void AddToIndex(TFile& file);

    private:
        TVector<TRef> Files;
        TMap<TString, TRef> Index;

        bool Maximized;
    };



    // Contains:
    // 1. "contrib/libs/protobuf/descriptor.proto" for protobuf options
    // 2. "base.proto" with base gazetteer definitions
    const TFileCollection& DefaultProtoFiles();

    const TString& BaseProtoName();      // Canonical name of base.proto
    const TFile& BaseProtoFile();       // the builtin file itself

}   // namespace NBuiltin



// DescriptorDatabase with NBuiltin::TFileCollection inside.
// Makes copies of all requested files descriptors which are marked as builtins.
// Otherwise reads files from disk (doing dependency path canonization)
class TMixedDescriptorDatabase: public NProtoBuf::DescriptorDatabase {
public:
    TMixedDescriptorDatabase(const NBuiltin::TFileCollection& builtinFiles,
                             NProtoBuf::compiler::SourceTree* srcTree,
                             NProtoBuf::compiler::MultiFileErrorCollector* errors);

    NProtoBuf::DescriptorPool::ErrorCollector* GetValidationErrorCollector() {
        return DiskDb.GetValidationErrorCollector();
    }

    // implements NProtoBuf::DescriptorDataBase
    bool FindFileByName(const TString& filename, NProtoBuf::FileDescriptorProto* output) override;
    bool FindFileContainingSymbol(const TString& symbol_name, NProtoBuf::FileDescriptorProto* output) override;
    bool FindFileContainingExtension(const TString& containing_type, int field_number, NProtoBuf::FileDescriptorProto* output) override;

    const TSet<TString>& RequestedFiles() const {
        return Files;
    }

    TString SearchableName(const TString& filename);

private:
    struct TInfo {
        NProtoBuf::FileDescriptorProto* File;       // NULL if unsuccessful;
        const NBuiltin::TFile* Builtin;

        TInfo(NProtoBuf::FileDescriptorProto* file);
    };

    bool DoFindFileByName(const TString& filename, TInfo& output);
    bool DoFindFileContainingSymbol(const TString& symbol_name, TInfo& output);
    bool DoFindFileContainingExtension(const TString& containing_type, int field_number, TInfo& output);

    bool CheckBuiltinOrClear(TInfo& info);
    bool Preprocess(TInfo& info);
    void PatchImports(TInfo& info);

private:
    NBuiltin::TFileCollection Builtins;
    NProtoBuf::DescriptorPoolDatabase BuiltinDb;
    NProtoBuf::compiler::SourceTreeDescriptorDatabase DiskDb;

    // All requested files with all their dependencies, sorted alphabetically
    TSet<TString> Files;
};


// Mixed descriptor pool: a set of specified builtin protos + protos read from disk
class TMixedDescriptorPool {
public:
    TMixedDescriptorPool(const NBuiltin::TFileCollection& builtins,
                         NProtoBuf::compiler::SourceTree* srcTree,
                         NProtoBuf::compiler::MultiFileErrorCollector* errors)
        : Database(builtins, srcTree, errors)
        , Pool(&Database, Database.GetValidationErrorCollector())
    {
    }

    // imitate DescriptorPool, search descriptor using mixed Database:
    // either in memory (if it is built-in) or on disk
    const TFileDescriptor* FindFileByName(const TString& name);

    const TFileDescriptor* ImportBuiltinProto(const NBuiltin::TFile& proto) {
        return FindFileByName(proto.Name());
    }

    // Collect all requested file descriptors with all their dependencies
    // Files are sorted by their canonical name.
    void CollectFiles(TVector<const TFileDescriptor*>& files);

    TString DebugString() {
        TString s("TMixedDescriptorPool::RequestedFiles():");
        TVector<const TFileDescriptor*> files;
        CollectFiles(files);
        for (size_t i = 0; i < files.size(); ++i)
            s.append("\n\t").append(files[i]->name());
        return s;
    }

private:
    TMixedDescriptorDatabase Database;
    NProtoBuf::DescriptorPool Pool;
};

}   // namespace NGzt
