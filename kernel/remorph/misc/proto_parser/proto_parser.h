#pragma once

#include "get_option.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/compiler/importer.h>

#include <kernel/gazetteer/protoparser/builtin.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NProtoParser {

typedef ::google::protobuf::FileDescriptor TFileDescriptor;
typedef ::google::protobuf::Descriptor TDescriptor;
typedef ::google::protobuf::compiler::DiskSourceTree TDiskSourceTree;
typedef ::google::protobuf::compiler::MultiFileErrorCollector TMultiFileErrorCollector;

struct TDescriptorHandler {
    virtual void HandleDescriptor(const TDescriptor& d, const TString& descrPath) = 0;
};

class TProtoParser: private TMultiFileErrorCollector {
private:
    TDiskSourceTree SourceTree;
    NGzt::TMixedDescriptorPool  ProtoPool;
    TString Namespace;
    THashSet<const TDescriptor*> UniqueDescriptors;

private:
    void AddError(const TString& filename, int line, int column, const TString& message) override;
    void TraverseDescriptors(const TFileDescriptor* fd, TDescriptorHandler& handler);

public:
    TProtoParser(const NGzt::NBuiltin::TFile& metaFile);
    ~TProtoParser() override {
    }

    void AddIncludeDir(const TString& dir);
    void Parse(const TString& path, TDescriptorHandler& handler);
};

} // NProtoParser
