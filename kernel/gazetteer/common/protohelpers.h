#pragma once

#include "serialize.h"

#include <library/cpp/protobuf/json/proto2json.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/io/tokenizer.h>

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

// Defined in this file:
class TErrorCollector;
class TDiskSourceTree;
class TSimpleJsonConverter;



//simple implementation of ErrorCollector and MultiFileErrorCollector - just prints all errors to stderr
class TErrorCollector: public NProtoBuf::io::ErrorCollector,
                       public NProtoBuf::compiler::MultiFileErrorCollector,
                       public NProtoBuf::DescriptorPool::ErrorCollector
{
public:
    // implements NProtoBuf::io::ErrorCollector
    void AddError(int line, int column, const TProtoStringType& message) override;

    // implements NProtoBuf::compiler::MultiFileErrorCollector
    void AddError(const TProtoStringType& filename, int line, int column, const TProtoStringType& message) override;

    // implements NProtoBuf::DescriptorPool::ErrorCollector
    void AddError(const TProtoStringType& filename, const TProtoStringType& element_name,
                          const NProtoBuf::Message* descriptor,
                          NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location,
                          const TProtoStringType& message) override;


    void AddErrorAtCurrentFile(int line, int column, const TProtoStringType& message);
    void SetCurrentFile(const TString& filename) {
        CurrentFile = filename;
    }

    // print error with @tree current file
    void AddError(const TDiskSourceTree* tree, int line, int column, const TProtoStringType& message);

    // just error without specific location
    void AddError(const TProtoStringType& message) {
        AddError(-1, 0, message);
    }

private:
    TString CurrentFile;
};


// Adds some helpers to NProtoBuf::compiler::DiskSourceTree
class TDiskSourceTree: public NProtoBuf::compiler::SourceTree
{
    typedef NProtoBuf::compiler::DiskSourceTree TProtobufDiskSourceTree;
    typedef TProtobufDiskSourceTree::DiskFileToVirtualFileResult EResolveRes;
    class TPathStack;

public:
    TDiskSourceTree();
    TDiskSourceTree(const TVector<TString>& importPaths);
    ~TDiskSourceTree() override;

    class TFileBase {
    public:
        // does Stack.Push() in derived class
        TFileBase(const TString& vfile, TPathStack& stack)
            : Stack(stack)
            , VirtualName(vfile)
        {
        }

        // does Stack.Pop()
        ~TFileBase();

        const TString& Virtual() const {
            return VirtualName;
        }

    private:
        TPathStack& Stack;
        const TString VirtualName;
    };

    // Using RAII idiom for push/pop in path stack
    class TRootFile: public TFileBase {
    public:
        TRootFile(const TString& file, TDiskSourceTree& tree)
            : TFileBase(tree.PushRootFile(file), *tree.RootPath)
        {
        }
    };

    class TCurrentFile: public TFileBase {
    public:
        TCurrentFile(const TString& file, TDiskSourceTree& tree)
            : TFileBase(tree.PushCurrentFile(file), *tree.CurPath)
        {
        }
    };


public:
    // Split @path by ":" and add given paths as roots.
    // @path could be a value of command-line argument  "-I" or "--proto_path", for example, as in protoc.
    void AddImportPaths(const TString& path);

    void AddImportPaths(const TVector<TString>& paths) {
        for (size_t i = 0; i != paths.size(); ++i)
            AddImportPaths(paths[i]);
    }

    TString RootDiskFile() const;

    TString CurrentDiskFile() const;
    TString CurrentDir() const {
        return GetDirName(CurrentDiskFile());
    }


    // Find or throw exception
    // Try returning virtual path from root or import paths in the first place, than go searching from current dir
    TString FindVirtualFile(const TString& diskFile) const;

    // Non-ambiguous @virtualFile from corresponding to @diskFile, or false
    bool DiskFileToVirtualFile(const TString& diskFile, TString& virtualFile) const;

    // Resolve @diskFile from @virtualFile
    bool VirtualFileToDiskFile(const TString& virtualFile, TString& diskFile) const;

    // Simple helper for previous
    TString GetDiskFileName(const TString& virtual_file) const {
        TString diskFile;
        return VirtualFileToDiskFile(virtual_file, diskFile) ? diskFile : TString();
    }

    // normalization: re-map to disk and back
    TString CanonicName(const TString& virtualFile) const;

    // Open file for reading via memory mapping, supports .gz (gzipped)
    TAutoPtr<IInputStream> OpenDiskFile(const TString& diskFile) const;
    TAutoPtr<IInputStream> OpenVirtualFile(const TString& virtualFile) const;

    // implements SourceTree -------------------------------------------
    NProtoBuf::io::ZeroCopyInputStream* Open(const TString& virtualFile) override;

private:
    static void AddPathAsRoot(const TString& path, TProtobufDiskSourceTree& tree) {
        if (!path.empty())
            tree.MapPath("", path);
    }

    static void RaiseMappingError(EResolveRes res, const TString& file, const TString& shadowingDiskFile);

    EResolveRes DiskFileToVirtualFile(const TString& diskFile, TString* virtualFile, TString* shadowingDiskFile) const;

    TString PushCurrentFile(const TString& file);
    TString PushRootFile(const TString& file);


    const TPathStack* CurrentPathStack() const;

private:
    THolder<TProtobufDiskSourceTree> ImportTree;
    THolder<TPathStack> RootPath;
    THolder<TPathStack> CurPath;
    TMutex Mutex;       // for const access to non-const methods of DiskSourceTree
};



// to make TSet<const NProtoBuf::Message*>
// NOT THREAD-SAFE!
class MessageLess
{
public:
    MessageLess();
    bool operator()(const NProtoBuf::Message* a, const NProtoBuf::Message* b);
private:
    TString BufferA, BufferB;
};



class TJsonPrinter
{
public:
    TJsonPrinter();
    TString ToString(const NProtoBuf::Message& message);
    void ToString(const NProtoBuf::Message& message, TString& out);
    void ToStream(const NProtoBuf::Message& message, IOutputStream& out);

    void SetSingleLineMode(bool mode) {
        Config.FormatOutput = !mode;
    }

private:
    NProtobufJson::TProto2JsonConfig Config;
};



// If @from has exactly the same message-type as TGeneratedMessage then @from is simply casted to TGeneratedMessage and returned.
// Otherwise @from is copied into @buffer via Reflection or re-serialization.
template <class TGeneratedMessage>
inline const TGeneratedMessage* CastProto(const NProtoBuf::Message* from, TGeneratedMessage* buffer) {
    if (const TGeneratedMessage* direct = dynamic_cast<const TGeneratedMessage*>(from)) {
        return direct;
    }

    if (from->GetDescriptor() == buffer->GetDescriptor()) {
        buffer->CopyFrom(*from);
        return buffer;
    }

    bool reserialized = ReserializeProto(from, buffer);
    if (reserialized) {
        return buffer;
    }

    ythrow yexception() << "Incompatible @to and @from messages";
}

// Convert @from data to @to via binary format,
// @from and @to should be binary compatible.
// Return true on successful conversion.
bool ReserializeProto(const NProtoBuf::Message* from, NProtoBuf::Message* to);
