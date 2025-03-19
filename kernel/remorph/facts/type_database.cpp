#include "type_database.h"

#include <google/protobuf/descriptor.pb.h>

#include <util/folder/dirut.h>
#include <util/string/split.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>
#include <util/generic/singleton.h>

namespace NFact {

struct TFactMetaFile: public NGzt::NBuiltin::TFile {
    TFactMetaFile()
        : NGzt::NBuiltin::TFile("kernel/remorph/facts/factmeta.proto")
    {
        AddAlias("factmeta.proto");
    }
};

TFactTypeDatabase::TFactTypeDatabase()
    : Parser(Default<TFactMetaFile>())
{
}

void TFactTypeDatabase::AddIncludeDir(const TString& dir) {
    Parser.AddIncludeDir(dir);
}

void TFactTypeDatabase::Load(const TString& path) {
    Parser.Parse(path, *this);
}

} // NFact
