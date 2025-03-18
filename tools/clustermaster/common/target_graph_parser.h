#pragma once

#include "config_lexer.h"
#include "log.h"
#include "parsing_helpers.h"

#include <library/cpp/any/any.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <utility>

using NAny::TAny;

struct TTargetTypeParsed {
    TString Name;
    TVector<TVector<TString> > Paramss;

    typedef TVector<std::pair<TString, TString> > TOptions;
    TOptions Options;

    static TTargetTypeParsed Parse(const TTokenStack& stack);
};

struct TTargetParsed {
    struct TParamMapping {
        ui32 MyLevelId;
        ui32 DepLevelId;

        TParamMapping(): MyLevelId(Max<ui32>()), DepLevelId(Max<ui32>()) {}
    };

    static TVector<TParamMapping> ParseParamMappings(TStringBuf buf);

    struct TDepend {
        // empty means prev
        TString Name;
        TString Condition;
        ui32 Flags;
        TMaybe<TVector<TParamMapping> > ParamMappings;

        TDepend()
            : Flags(0)
        {}

        TString ToString() const;
    };

    TString Name;
    TString Type;
    TVector<TDepend> Depends;

    typedef TVector<std::pair<TString, TString> > TOptions;
    TOptions Options;

    static TTargetParsed Parse(const TTokenStack& stack);
};

struct TVariableParsed {
    TString Name;
    TString Value;
    bool Strong;

    TVariableParsed(): Strong(false) {}

    static TVariableParsed Parse(const TTokenStack& stack);
};

typedef TAny TTargetGraphNodeParsed;

struct TTargetGraphParsed {
    static TTargetGraphNodeParsed ParseNode(const TTokenStack& stack);

    struct TIntermediate {
        TVector<TTokenStack> Stacks;
        TString Shebang;

        static TIntermediate Parse(TStringBuf stirngBuf);
    };

    TVector<TTargetGraphNodeParsed> Nodes;
    TString Shebang;

    static TTargetGraphParsed Parse(TStringBuf stringBuf);

    template <typename T>
    TVector<T> GetNodesOfType() const;
    TVector<TTargetTypeParsed> GetTypes() const;
    TVector<TTargetParsed> GetTargets() const;
};


