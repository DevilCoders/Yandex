#include "builtin.h"

#include <util/generic/yexception.h>
#include <util/generic/singleton.h>
#include <util/generic/algorithm.h>

#include <kernel/gazetteer/proto/base.pb.h>

namespace NGzt {

typedef NProtoBuf::FileDescriptorProto TFileDescriptorProto;

namespace NBuiltin {

    const TFileDescriptor* FindInGeneratedPool(const TString& name) {
        return NProtoBuf::DescriptorPool::generated_pool()->FindFileByName(name);
    }

    TFile::TFile(const TString& name)
        : GeneratedDescriptor(nullptr)
    {
        if (!name)
            ythrow yexception() << "NBuiltin::TFile's base name cannot be empty";
        AddAlias(name);
    }

    inline bool TFile::Is(const TStringBuf& name) const {
        return ::Find(Aliases_.begin(), Aliases_.end(), name) != Aliases_.end();
    }

    inline void TFile::AddAliasImpl(const TString& name) {
        if (!name.empty() && !Is(name)) {
            Aliases_.push_back(name);
            if (!GeneratedDescriptor)
                GeneratedDescriptor = NBuiltin::FindInGeneratedPool(name);
        }
    }

    TFile& TFile::AddAlias(const TString& name) {
        AddAliasImpl(name);

        constexpr TStringBuf proto = ".proto";
        constexpr TStringBuf gztproto = ".gztproto";

        // also add complementing .gztproto or .proto
        if (name.EndsWith(proto))
            AddAliasImpl(TString::Join(TStringBuf(name).Chop(proto.size()), gztproto));
        else if (name.EndsWith(gztproto))
            AddAliasImpl(TString::Join(TStringBuf(name).Chop(gztproto.size()), proto));

        return *this;
    }

    void TFile::MergeAliases(const TFile& file) {
        for (size_t i = 0; i < file.Aliases().size(); ++i)
            AddAliasImpl(file.Aliases()[i]);
    }



    const TFile& TFileCollection::AddFile(const TString& name, const TString& alias1, const TString& alias2) {
        return Merge(TFile(name).AddAlias(alias1).AddAlias(alias2));
    }

    TFile* TFileCollection::FindByAlias(const TFile& file) {
        TFile* ret = nullptr;
        for (size_t i = 0; i < file.Aliases().size(); ++i) {
            TMap<TString, TRef>::iterator it = Index.find(file.Aliases()[i]);
            if (it != Index.end()) {
                if (!ret)
                    ret = it->second.Get();
                else if (it->second.Get() != ret)
                    ythrow yexception() << "Cannot identify builtin file unambiguously";
            }
        }
        return ret;
    }

    void TFileCollection::AddToIndex(TFile& file) {
        for (size_t i = 0; i < file.Aliases().size(); ++i)
            Index[file.Aliases()[i]] = &file;
    }

    const TFile& TFileCollection::Merge(const TFile& file) {
//        if (!file.FindInGeneratedPool())
//            return;

        TRef myfile = FindByAlias(file);
        if (!myfile) {
            myfile = new TFile(file);
            Files.push_back(myfile);
        } else
            myfile->MergeAliases(file);

        AddToIndex(*myfile);
        return *myfile;
    }

    void TFileCollection::MergeFiles(const TFileCollection& files) {
        for (size_t i = 0; i < files.Size(); ++i)
            Merge(files[i]);

        if (files.Maximized)
            Maximized = true;
    }

    TRef TFileCollection::FindExplicit(const TStringBuf& name) const {
        TMap<TString, TRef>::const_iterator it = Index.find(name);
        return it != Index.end() ? it->second : nullptr;
    }

    TRef TFileCollection::FindImplicit(const TString& name) const {
        // search in generated_pool and construct TFile on the fly
        TRef file(new TFile(name));
        return file->FindInGeneratedPool() ? file : nullptr;
    }

    TConstRef TFileCollection::Find(const TStringBuf& name) const {
        TRef ret = FindExplicit(name);
        if (!ret && Maximized)
            ret = FindImplicit(TString(name));
        return ret.Get();
    }

    const TFileDescriptor* TFileCollection::FindFileDescriptor(const TStringBuf& name) const {
        TConstRef ret = Find(name);
        return ret.Get() ? ret->FindInGeneratedPool() : nullptr;
    }

    const TFile* TFileCollection::FindOrAdd(const TString& name) {
        TRef ret = FindExplicit(name);
        if (!ret && Maximized) {
            TRef impl = FindImplicit(name);
            if (impl.Get()) {
                Merge(*impl);
                ret = FindExplicit(name);
            }
        }
        return ret.Get();
    }

    TString TFileCollection::DebugString() const {
        TString ret;
        for (size_t i = 0; i < Files.size(); ++i) {
            ret.append(Files[i]->Name()).append(" ->");
            for (size_t j = 0; j < Files[i]->Aliases().size(); ++j)
                ret.append(' ').append(Files[i]->Aliases()[j]);
            ret.append('\n');
        }
        return ret;
    }


    static const TString BASE_PROTO = "kernel/gazetteer/proto/base.proto";

    class TDefaultProtoFiles: public TFileCollection {
    public:
        const TFile& DescriptorProto;
        const TFile& BaseProto;

        TDefaultProtoFiles()
            : DescriptorProto(
                AddFileSafe<NProtoBuf::DescriptorProto>("google/protobuf/descriptor.proto", "descriptor.proto"))
            , BaseProto(
                AddFileSafe<TArticle>(BASE_PROTO, "base.proto"))
        {
        }
    };

    const TFileCollection& DefaultProtoFiles() {
        return Default<TDefaultProtoFiles>();
    }

    const TString& BaseProtoName() {
        return BASE_PROTO;
    }

    const TFile& BaseProtoFile() {
        return Default<TDefaultProtoFiles>().BaseProto;
    }



}   // namespace NBuiltin



TMixedDescriptorDatabase::TMixedDescriptorDatabase(const NBuiltin::TFileCollection& builtins,
                                                   NProtoBuf::compiler::SourceTree* srcTree,
                                                   NProtoBuf::compiler::MultiFileErrorCollector* errors)
    : BuiltinDb(*NProtoBuf::DescriptorPool::generated_pool())
    , DiskDb(srcTree)
{
    Builtins.MergeFiles(builtins);
    DiskDb.RecordErrorsTo(errors);
}

TMixedDescriptorDatabase::TInfo::TInfo(NProtoBuf::FileDescriptorProto* file)
    : File(file)
    , Builtin(nullptr)
{
}

bool TMixedDescriptorDatabase::CheckBuiltinOrClear(TInfo& info) {
    info.Builtin = Builtins.FindOrAdd(info.File->name());
    if (info.Builtin)
        return true;

    info.File->Clear();
    return false;
}

bool TMixedDescriptorDatabase::DoFindFileByName(const TString& filename, TInfo& info) {
    info.Builtin = Builtins.FindOrAdd(filename);
    if (info.Builtin) {
        return BuiltinDb.FindFileByName(info.Builtin->GeneratedPoolName(), info.File);
    } else
        return DiskDb.FindFileByName(filename, info.File);
}

bool TMixedDescriptorDatabase::DoFindFileContainingSymbol(const TString& symbol_name, TInfo& info) {
    if (BuiltinDb.FindFileContainingSymbol(symbol_name, info.File))
        if (CheckBuiltinOrClear(info))
            return true;

    return DiskDb.FindFileContainingSymbol(symbol_name, info.File);
}

bool TMixedDescriptorDatabase::DoFindFileContainingExtension(const TString& containing_type, int field_number, TInfo& info) {
    if (BuiltinDb.FindFileContainingExtension(containing_type, field_number, info.File))
        if (CheckBuiltinOrClear(info))
            return true;

    return DiskDb.FindFileContainingExtension(containing_type, field_number, info.File);
}

bool TMixedDescriptorDatabase::FindFileByName(const TString& filename, NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != nullptr
        && DoFindFileByName(filename, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::FindFileContainingSymbol(const TString& symbol_name, NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != nullptr
        && DoFindFileContainingSymbol(symbol_name, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::FindFileContainingExtension(const TString& containing_type, int field_number,
                                                           NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != nullptr
        && DoFindFileContainingExtension(containing_type, field_number, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::Preprocess(TInfo& info) {
    if (!info.Builtin)
        PatchImports(info);
    Files.insert(info.File->name());
    return true;
}

void TMixedDescriptorDatabase::PatchImports(TInfo& info) {
    // Canonicalize builtins' imports to make sure parent DescriptorPool
    // finds these imported files later using its FindFileByName() method.
    // See comment to TMixedDescriptorPool::FindFileByName() below.
    for (int i = 0; i < info.File->dependency_size(); ++i) {
        const NBuiltin::TFile* file = Builtins.FindOrAdd(info.File->dependency(i));
        if (file)
            info.File->set_dependency(i, file->GeneratedPoolName());
    }
}

TString TMixedDescriptorDatabase::SearchableName(const TString& filename) {
    const NBuiltin::TFile* b = Builtins.FindOrAdd(filename);
    return b ? b->GeneratedPoolName() : filename;
}

const TFileDescriptor* TMixedDescriptorPool::FindFileByName(const TString& name) {
    // Here it is important to search builtin files by their names from generated_pool.
    // For example, "test.gztproto" should be searched as "test.proto",
    // otherwise Database finds it as "test.proto", Pool puts it in its tables as "test.proto"
    // and then try find it there as "test.gztproto" and fails.

    return Pool.FindFileByName(Database.SearchableName(name));
}

void TMixedDescriptorPool::CollectFiles(TVector<const TFileDescriptor*>& files) {
    files.clear();
    const TSet<TString>& fileNames = Database.RequestedFiles();
    for (TSet<TString>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        const TFileDescriptor* fd = FindFileByName(*it);
        if (fd)
            files.push_back(fd);
    }
}

}   // namespace NGzt
