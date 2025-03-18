#pragma once

#include "ast.h"

#include <util/stream/output.h>

namespace NDomSchemeCompiler {

class TGenerator {
    IOutputStream& OutStream; // do not use directly
    size_t Indent;

public:
    TGenerator(IOutputStream& out);
    void Run(const TProgram& program);
private:
    IOutputStream& O();

    struct TPath;
    struct TListGenerator;

    void GenerateStruct(const TPath& path, const TString& name, bool isTopLevel, bool isConst, const TStruct& structDef);
    void GenerateTypedef(const TNamedType& type);
    static TString GetUserCppType(const TString& name, bool isConst, bool isTopLevel);
    static TString GetCppType(const TType& t, bool isConst);
    TMaybe<TString> GetDefaultValue(const TMemberDefinition& member) const;
    TMaybe<TVector<TString>> GetDefaultArray(const TMemberDefinition& member) const;
};

inline void Generate(const TProgram& program, IOutputStream& out) {
    TGenerator g(out);
    g.Run(program);
}

};
