#include "proto_parser.h"

#include <google/protobuf/descriptor.pb.h>

#include <util/folder/dirut.h>
#include <util/string/split.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>

namespace NProtoParser {

class TRemorphBuiltinProtoFiles: public NGzt::NBuiltin::TFileCollection {
public:
    TRemorphBuiltinProtoFiles(const NGzt::NBuiltin::TFile& metaFile) {
        AddFileSafe<NProtoBuf::DescriptorProto>("contrib/libs/protobuf/descriptor.proto");
        Merge(metaFile);
    }
};

TProtoParser::TProtoParser(const NGzt::NBuiltin::TFile& metaFile)
    : SourceTree()
    , ProtoPool(TRemorphBuiltinProtoFiles(metaFile), &SourceTree, this)
{
    SourceTree.MapPath("", NFs::CurrentWorkingDirectory());
    SourceTree.MapPath("", "/");

    const TFileDescriptor* embeddedMeta = ProtoPool.FindFileByName(metaFile.Name());
    Y_VERIFY(nullptr != embeddedMeta, "Cannot load embedded \"%s\"", metaFile.Name().data());

    Namespace = embeddedMeta->package();
}

void TProtoParser::AddError(const TString& filename, int line, int column, const TString& message) {
    if (-1 == line)
        throw yexception() << filename << ": " << message;
    else
        throw yexception() << filename << "," << (line + 1) << ":" << (column + 1) << ": " << message;
}


void TProtoParser::AddIncludeDir(const TString& dir) {
    ::NFs::EnsureExists(dir.data());
    if (!::IsDir(dir))
        throw yexception() << "The include path " << dir << " is not valid directory";
    SourceTree.MapPath("", dir);
}

void TProtoParser::TraverseDescriptors(const TFileDescriptor* fd, TDescriptorHandler& handler) {
    if (fd->package() == Namespace) {
        for (int i = 0; i < fd->message_type_count(); ++i) {
            const TDescriptor* d = fd->message_type(i);
            // File can be imported multiple times. Process only new descriptors
            if (UniqueDescriptors.insert(d).second) {
                TString descrPath = d->file()->name();
                SourceTree.VirtualFileToDiskFile(descrPath, &descrPath);
                handler.HandleDescriptor(*d, descrPath);
            }
        }
    }
    // Process dependencies
    for (int i = 0; i < fd->dependency_count(); ++i) {
        TraverseDescriptors(fd->dependency(i), handler);
    }
}

void TProtoParser::Parse(const TString& path, TDescriptorHandler& handler) {
    TString virt;
    TString shad;
    if (TDiskSourceTree::SUCCESS == SourceTree.DiskFileToVirtualFile(path, &virt, &shad))
        TraverseDescriptors(ProtoPool.FindFileByName(virt), handler);
    else
        TraverseDescriptors(ProtoPool.FindFileByName(path), handler);
}

} // NProtoParser
