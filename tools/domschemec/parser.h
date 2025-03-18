#pragma once

#include "ast.h"
#include "lexer.h"

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>

namespace NDomSchemeCompiler {

class TParser : private TLexer {
public:
    TParser(const TString& file);
    TParser(THolder<ILexerContext>&& ctx);

    const TProgram& GetProgram() const;
    void Run();

private:
    struct TScope;

    TProgram Program;
    TScope* CurrentScope;

    TVector<TString> ParseNamespaceName();
    bool TryParseInclude();
    bool TryParsePragmas();
    bool TryParseNamedType(bool toplevel, TFieldAttrs* fieldAttrs);
    void ParseStructBody(TStruct& structDef);
    void ParseType(TType& type, bool structAllowed=true);
    void ParseAttrs(TStructAttrs*, TFieldAttrs*, TValueAttrs*);
    void ParseAttrsDefaultValue(TValueAttrs*);
    void ParseParentTypes(TStruct& structDef);

    void AddStructTypeToScopeIfNeed(TType& type, const TString& name);
};

inline TProgram Parse(const TString& code) {
    TParser p(code);
    p.Run();
    return p.GetProgram();
}

};
