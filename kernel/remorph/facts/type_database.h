#pragma once

#include "facttype.h"

#include <kernel/remorph/misc/proto_parser/proto_parser.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NFact {

class TFactTypeDatabase: protected NProtoParser::TDescriptorHandler {
private:
    NProtoParser::TProtoParser Parser;
    TVector<TFactTypePtr> Types;

protected:
    void HandleDescriptor(const NProtoParser::TDescriptor& d) {
        Types.push_back(new TFactType(d));
    }

    void HandleDescriptor(const NProtoParser::TDescriptor& d, const TString& descrPath) {
        Types.push_back(new TFactType(d, descrPath));
    }

    inline void ResetTypes() {
        Types.clear();
    }

public:
    TFactTypeDatabase();
    virtual ~TFactTypeDatabase() {
    }

    void AddIncludeDir(const TString& dir);
    void Load(const TString& path);

    inline const TVector<TFactTypePtr>& GetFactTypes() const {
        return Types;
    }
};

} // NFact
