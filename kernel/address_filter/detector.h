#pragma once

#include "token_types.h"
#include <util/stream/str.h>
#include <util/stream/trace.h>

#include <utility>


namespace NAddressFilter {

class TDetector {

    class TAddressSchemaNode;
    typedef TSimpleSharedPtr<TAddressSchemaNode> TAddressSchemaNodePtr;

    class TAddressSchemaNode {
    public:
        TAddressSchemaNode(const TString& name);

        void AddSchema(const TString& ruleName, const TVector<ETokenSubType>& tokens, size_t startPos = 0, size_t skipLen = 0);

        void Print(const TString& tab) const;
        TString GetPrintableNextKeys() const;


    private:
        TString GetPrintableWay(const TVector<ETokenSubType>& tokens, size_t startPos) const;

    public:
        bool IsFinal;
        TTokenType NextKeys;
        TVector< std::pair <ETokenSubType, TAddressSchemaNodePtr> > NextNodes;
        TString Name;
        size_t SchemaLen;
        size_t SkipLen;
    };

public:
    TDetector(bool good_streets);

    TSimpleSharedPtr< TVector<TAddressPosition> > FilterText(const TVector<TTokenType>& reversedTokens) const;

private:
    void PrintTokenType(size_t tokenIndex, const TTokenType& token) const;
    void PrintStartAll() const;
    void PrintStopOnNode(const TAddressSchemaNodePtr& shemaNode) const;
    void PrintFound(size_t tokenIndex, const TAddressSchemaNodePtr& shemaNode) const;
    void PrintAdvance(const TAddressSchemaNodePtr& schemaNode, const TAddressSchemaNodePtr& nextNode) const;
    void PrintActiveNumber(size_t n) const;

    static TVector<ETokenSubType> CreateVector(ETokenSubType token1,
                                              ETokenSubType token2,
                                              ETokenSubType token3 = TT_ERROR,
                                              ETokenSubType token4 = TT_ERROR,
                                              ETokenSubType token5 = TT_ERROR,
                                              ETokenSubType token6 = TT_ERROR,
                                              ETokenSubType token7 = TT_ERROR,
                                              ETokenSubType token8 = TT_ERROR,
                                              ETokenSubType token9 = TT_ERROR,
                                              ETokenSubType token10 = TT_ERROR);


    void AddSchema(const TString& name, TVector<ETokenSubType> tokens);
    void AddSchema(const TString& name, size_t skipLen, TVector<ETokenSubType> tokens);

private:
    TAddressSchemaNodePtr SchemaTreeRoot;
};

} //NAddressFilter
