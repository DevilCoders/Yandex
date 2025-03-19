#include "detector.h"

#include <util/stream/trace.h>
#include <util/generic/utility.h>


namespace NAddressFilter {

TDetector::TAddressSchemaNode::TAddressSchemaNode(const TString& name)
    : IsFinal(false)
    , Name(name)
{
}

void TDetector::TAddressSchemaNode::Print(const TString& tab) const {
    Y_DBGTRACE(DEBUG, tab << "Node " << Name);
    for (size_t i = 0; i < NextNodes.size(); i++) {
        Y_DBGTRACE(DEBUG, tab << TYPE_DESCRIPTIONS[NextNodes[i].first]);
        NextNodes[i].second->Print(tab + TString("\t"));
    }
}

TString TDetector::TAddressSchemaNode::GetPrintableNextKeys() const {
    TStringStream stream;
    for (size_t tokenSubType = NextKeys.FirstNonZeroBit(); tokenSubType != NextKeys.Size(); tokenSubType = NextKeys.NextNonZeroBit(tokenSubType)) {
        stream << TYPE_DESCRIPTIONS[tokenSubType] << " ";
    }
    return stream.Str();
}

TString TDetector::TAddressSchemaNode::GetPrintableWay(const TVector<ETokenSubType>& tokens, size_t startPos) const {
    TStringStream stream;
    for(size_t i = 0; i <= startPos; i++) {
        stream << TYPE_DESCRIPTIONS[tokens[i]] << "-";
    }
    return stream.Str();
}

void TDetector::TAddressSchemaNode::AddSchema(const TString& ruleName, const TVector<ETokenSubType>& tokens, size_t startPos, size_t skipLen) {
    if (startPos == tokens.size()) {
        Name = ruleName;
        SchemaLen = tokens.size();
        SkipLen = skipLen;
        IsFinal = true;
        return;
    }

    if (!NextKeys.Get(tokens[startPos])) {
        TAddressSchemaNodePtr node(new TAddressSchemaNode(GetPrintableWay(tokens, startPos)));
        NextNodes.push_back(std::pair <ETokenSubType, TAddressSchemaNodePtr>(tokens[startPos], node));

        NextKeys.Set(tokens[startPos]);

        node->AddSchema(ruleName, tokens, startPos + 1, skipLen);

    } else {

        for(size_t i = 0; i < NextNodes.size(); i++) {
            if (NextNodes[i].first == tokens[startPos]) {
                NextNodes[i].second->AddSchema(ruleName, tokens, startPos + 1, skipLen);
                break;
            }
        }
    }
}


TDetector::TDetector(bool good_streets)
    : SchemaTreeRoot(new TAddressSchemaNode(TString("root")))
{
    AddSchema(TString("CDN"), CreateVector(TT_CAPITAL, TT_DESCRIPTOR, TT_NUMBER));

    AddSchema(TString("DCN"), CreateVector(TT_DESCRIPTOR, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DCEN"), CreateVector(TT_DESCRIPTOR, TT_CAPITAL, TT_EMPTY, TT_NUMBER));
    AddSchema(TString("DCCN"), CreateVector(TT_DESCRIPTOR, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DCCCN"), CreateVector(TT_DESCRIPTOR, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));

    AddSchema(TString("GCCN"), CreateVector(TT_CITY, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("GCCCN"), CreateVector(TT_CITY, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));

    AddSchema(TString("CHN"), CreateVector(TT_CAPITAL, TT_HOUSE, TT_NUMBER));
    AddSchema(TString("CEHN"), CreateVector(TT_CAPITAL, TT_EMPTY, TT_HOUSE, TT_NUMBER));

    AddSchema(TString("CNBN"), CreateVector(TT_CAPITAL, TT_NUMBER, TT_BUILDING, TT_NUMBER));

    AddSchema(TString("DNCN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DNCCN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DNECN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_EMPTY, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DNCEN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_CAPITAL, TT_EMPTY, TT_NUMBER));

    AddSchema(TString("NLN"), CreateVector(TT_NUMBER, TT_LINE, TT_NUMBER));
    AddSchema(TString("LHN"), CreateVector(TT_LINE, TT_HOUSE, TT_NUMBER));

    AddSchema(TString("DHN"), CreateVector(TT_DESCRIPTOR, TT_HOUSE, TT_NUMBER));

    AddSchema(TString("LVoN"), CreateVector(TT_LINE, TT_VO_11, TT_NUMBER));
    AddSchema(TString("LVovON"), CreateVector(TT_LINE, TT_VO_21, TT_VO_22, TT_NUMBER));

    AddSchema(TString("SN"), CreateVector(TT_STREET_DICT_11, TT_NUMBER));
    AddSchema(TString("SSN"), CreateVector(TT_STREET_DICT_21, TT_STREET_DICT_22, TT_NUMBER));
    AddSchema(TString("SSSN"), CreateVector(TT_STREET_DICT_31, TT_STREET_DICT_32, TT_STREET_DICT_33, TT_NUMBER));
    AddSchema(TString("SSSSN"), CreateVector(TT_STREET_DICT_41, TT_STREET_DICT_42, TT_STREET_DICT_43, TT_STREET_DICT_44, TT_NUMBER));

    AddSchema(TString("GdCN"), CreateVector(TT_CITY_DICT_11, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("GdGdCN"), CreateVector(TT_CITY_DICT_21, TT_CITY_DICT_22, TT_CAPITAL, TT_NUMBER));

    AddSchema(TString("CEDN"), CreateVector(TT_CAPITAL, TT_EMPTY, TT_DESCRIPTOR, TT_NUMBER));
    AddSchema(TString("NNeLN"), CreateVector(TT_NUMBER, TT_NUMBER_END, TT_LINE, TT_NUMBER));
    AddSchema(TString("GCNCN"), CreateVector(TT_CITY, TT_CAPITAL, TT_NUMBER, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("GCNNeCN"), CreateVector(TT_CITY, TT_CAPITAL, TT_NUMBER, TT_NUMBER_END, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("DNNeCCN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_NUMBER_END, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));

    AddSchema(TString("DsN"), CreateVector(TT_DESCRIPTOR, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("DssN"), CreateVector(TT_DESCRIPTOR, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("DsssN"), CreateVector(TT_DESCRIPTOR, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("DssssN"), CreateVector(TT_DESCRIPTOR, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));
    AddSchema(TString("sDN"), CreateVector(TT_STREET_DICT_11S, TT_DESCRIPTOR, TT_NUMBER));
    AddSchema(TString("ssDN"), CreateVector(TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_DESCRIPTOR, TT_NUMBER));
    AddSchema(TString("sssDN"), CreateVector(TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_DESCRIPTOR, TT_NUMBER));
    AddSchema(TString("ssssDN"), CreateVector(TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_DESCRIPTOR, TT_NUMBER));

    AddSchema(TString("DNMN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_MONTH, TT_NUMBER));
    AddSchema(TString("DNMHN"), CreateVector(TT_DESCRIPTOR, TT_NUMBER, TT_MONTH, TT_HOUSE, TT_NUMBER));
    AddSchema(TString("NMDN"), CreateVector(TT_NUMBER, TT_MONTH, TT_DESCRIPTOR, TT_NUMBER));


    AddSchema(TString("ACN"), 1, CreateVector( TT_ADDRESS, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("ACCN"), 1, CreateVector( TT_ADDRESS, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("ACCCN"), 1, CreateVector( TT_ADDRESS, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("ACCCCN"), 1, CreateVector( TT_ADDRESS, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("AECN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("AECCN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("AECCCN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("AECCCCN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_CAPITAL, TT_NUMBER));
    AddSchema(TString("AsN"), 1, CreateVector( TT_ADDRESS, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("AssN"), 1, CreateVector( TT_ADDRESS, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("AsssN"), 1, CreateVector( TT_ADDRESS, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("AssssN"), 1, CreateVector( TT_ADDRESS, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));
    AddSchema(TString("AGdsN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_11, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("AGdssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_11, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("AGdsssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_11, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("AGdssssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_11, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));
    AddSchema(TString("AEGdsN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_11, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("AEGdssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_11, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("AEGdsssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_11, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("AEGdssssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_11, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));
    AddSchema(TString("AGdGdsN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("AGdGdssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("AGdGdsssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("AGdGdssssN"), 1, CreateVector( TT_ADDRESS, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));
    AddSchema(TString("AEGdGdsN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_11S, TT_NUMBER));
    AddSchema(TString("AEGdGdssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_21S, TT_STREET_DICT_22S, TT_NUMBER));
    AddSchema(TString("AEGdGdsssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_31S, TT_STREET_DICT_32S, TT_STREET_DICT_33S, TT_NUMBER));
    AddSchema(TString("AEGdGdssssN"), 2, CreateVector(TT_ADDRESS, TT_EMPTY, TT_CITY_DICT_21, TT_CITY_DICT_22, TT_STREET_DICT_41S, TT_STREET_DICT_42S, TT_STREET_DICT_43S, TT_STREET_DICT_44S, TT_NUMBER));

    AddSchema(TString("DEHN"), 2, CreateVector(TT_DESCRIPTOR, TT_EMPTY, TT_HOUSE, TT_NUMBER));

    if (good_streets) {
        AddSchema(TString("GS_CD"), CreateVector(TT_CAPITAL, TT_DESCRIPTOR_NN));
        AddSchema(TString("GS_DC"), CreateVector(TT_DESCRIPTOR_NN, TT_CAPITAL));
        AddSchema(TString("GS_DNC"), CreateVector(TT_DESCRIPTOR_NN, TT_NUMBER, TT_CAPITAL));
        AddSchema(TString("GS_NLN"), CreateVector(TT_NUMBER, TT_LINE, TT_NUMBER));
        AddSchema(TString("GS_NNeL"), CreateVector(TT_NUMBER, TT_NUMBER_END, TT_LINE));
        AddSchema(TString("GS_GCNC"), CreateVector(TT_CITY, TT_CAPITAL, TT_NUMBER, TT_CAPITAL));
        AddSchema(TString("GS_GCNNeC"), CreateVector(TT_CITY, TT_CAPITAL, TT_NUMBER, TT_NUMBER_END, TT_CAPITAL));
        AddSchema(TString("GS_DNNeCC"), CreateVector(TT_DESCRIPTOR_NN, TT_NUMBER, TT_NUMBER_END, TT_CAPITAL, TT_CAPITAL));
        AddSchema(TString("GS_CNNeD"), CreateVector(TT_CAPITAL, TT_NUMBER, TT_NUMBER_END, TT_DESCRIPTOR_NN));

        AddSchema(TString("GS_MdM"), CreateVector(TT_METRO_DESCR, TT_METRO_11));
        AddSchema(TString("GS_MdM"), CreateVector(TT_METRO_DESCR, TT_METRO_21, TT_METRO_22));
        AddSchema(TString("GS_MdM"), CreateVector(TT_METRO_DESCR, TT_METRO_31, TT_METRO_32, TT_METRO_33));

        AddSchema(TString("GS_MdM"), CreateVector(TT_DESCRIPTOR_NN, TT_NAME_DESCR, TT_CAPITAL));
    }

    if (TRACE_DEBUG <= StdDbgLevel())
        SchemaTreeRoot->Print(TString(""));
}

TVector<ETokenSubType> TDetector::CreateVector(ETokenSubType token1,
                                                    ETokenSubType token2,
                                                    ETokenSubType token3,
                                                    ETokenSubType token4,
                                                    ETokenSubType token5,
                                                    ETokenSubType token6,
                                                    ETokenSubType token7,
                                                    ETokenSubType token8,
                                                    ETokenSubType token9,
                                                    ETokenSubType token10) {
    TVector<ETokenSubType> tokens;

    if (token10 != TT_ERROR)
        tokens.push_back(token10);
    if (token9 != TT_ERROR)
        tokens.push_back(token9);
    if (token8 != TT_ERROR)
        tokens.push_back(token8);
    if (token7 != TT_ERROR)
        tokens.push_back(token7);
    if (token6 != TT_ERROR)
        tokens.push_back(token6);
    if (token5 != TT_ERROR)
        tokens.push_back(token5);
    if (token4 != TT_ERROR)
        tokens.push_back(token4);
    if (token3 != TT_ERROR)
        tokens.push_back(token3);
    if (token2 != TT_ERROR)
        tokens.push_back(token2);
    if (token1 != TT_ERROR)
        tokens.push_back(token1);

    return tokens;
}

void TDetector::AddSchema(const TString& name, size_t skipLen, TVector<ETokenSubType>  tokens) {
    SchemaTreeRoot->AddSchema(name, tokens, 0, skipLen);
}

void TDetector::AddSchema(const TString& name, TVector<ETokenSubType> tokens) {
    AddSchema(name, 0, tokens);
}


void TDetector::PrintTokenType(size_t tokenIndex, const TTokenType& token) const {
    Y_UNUSED(tokenIndex);
    Y_UNUSED(token);
    Y_DBGTRACE(DEBUG, "----------------------------------------");
    Y_DBGTRACE(DEBUG, tokenIndex << " type: "<< GetDebugTokenType(token));
}

void TDetector::PrintFound(size_t tokenIndex, const TAddressSchemaNodePtr& schemaNode) const {
    Y_UNUSED(tokenIndex);
    Y_UNUSED(schemaNode);
    Y_DBGTRACE(DEBUG, "Found " << schemaNode->Name << " From " << tokenIndex + 1 - schemaNode->SchemaLen << " to " << tokenIndex - schemaNode->SkipLen);
    Y_DBGTRACE(DEBUG, "" << tokenIndex << " " << schemaNode->SchemaLen);

}

void TDetector::PrintAdvance(const TAddressSchemaNodePtr& schemaNode, const TAddressSchemaNodePtr& nextNode) const {
    Y_UNUSED(nextNode);
    Y_UNUSED(schemaNode);
    Y_DBGTRACE(DEBUG, "Advance from " << schemaNode->Name << " to " << nextNode->Name);
}

void TDetector::PrintStartAll() const {
    Y_DBGTRACE(DEBUG, "Started all rules");
}

void TDetector::PrintStopOnNode(const TAddressSchemaNodePtr& schemaNode) const {
    Y_UNUSED(schemaNode);
    Y_DBGTRACE(DEBUG, "On " << schemaNode->Name << " no one from " << schemaNode->GetPrintableNextKeys());
}

void TDetector::PrintActiveNumber(size_t n) const {
    Y_UNUSED(n);
    Y_DBGTRACE(DEBUG, "Active schemas " << n);
}


TSimpleSharedPtr< TVector<TAddressPosition> > TDetector::FilterText(const TVector<TTokenType>& reversedTokens) const {
    TSimpleSharedPtr< TVector<TAddressPosition> > result(new TVector<TAddressPosition>());

    TVector <TAddressSchemaNodePtr> currentSchemas;
    TVector <TAddressSchemaNodePtr> nextSchemas;
    currentSchemas.push_back(SchemaTreeRoot);


    for(size_t revTokenIndex = 0; revTokenIndex < reversedTokens.size(); revTokenIndex++) {
        TTokenType token = reversedTokens[revTokenIndex];
        bool found = false;

        if (TRACE_DEBUG <= StdDbgLevel())
            PrintTokenType(revTokenIndex, token);

        if (currentSchemas.size() != 0) {
            for(size_t shmemaIndex = 0; (shmemaIndex < currentSchemas.size()) && !found; shmemaIndex++) {
                TAddressSchemaNodePtr schemaNode = currentSchemas[shmemaIndex];

                if ((schemaNode->NextKeys & token).Empty()) {
                    if (TRACE_DEBUG <= StdDbgLevel())
                        PrintStopOnNode(schemaNode);

                    continue;
                }

                for (size_t tokenSubType = token.FirstNonZeroBit(); (tokenSubType != token.Size()) && !found; tokenSubType = token.NextNonZeroBit(tokenSubType)) {
                    for (size_t nextIndex = 0; nextIndex < schemaNode->NextNodes.size(); nextIndex++) {
                        if ((ETokenSubType)tokenSubType == schemaNode->NextNodes[nextIndex].first) {
                            if (schemaNode->NextNodes[nextIndex].second->IsFinal) {
                                found = true;
                                result->push_back(TAddressPosition(revTokenIndex - schemaNode->NextNodes[nextIndex].second->SkipLen, revTokenIndex - schemaNode->NextNodes[nextIndex].second->SchemaLen + 1));

                                if (TRACE_DEBUG <= StdDbgLevel())
                                    PrintFound(revTokenIndex, schemaNode->NextNodes[nextIndex].second);

                                break;
                            } else {
                                nextSchemas.push_back(schemaNode->NextNodes[nextIndex].second);

                                if (TRACE_DEBUG <= StdDbgLevel())
                                    PrintAdvance(schemaNode, schemaNode->NextNodes[nextIndex].second);
                            }
                        }
                    }
                }
            }
        }

        if (found) {
            currentSchemas.clear();
            nextSchemas.clear();
            currentSchemas.push_back(SchemaTreeRoot);
            continue;
        }

        currentSchemas.clear();
        nextSchemas.push_back(SchemaTreeRoot);
        DoSwap(currentSchemas, nextSchemas);
        currentSchemas.push_back(SchemaTreeRoot);


        if (TRACE_DEBUG <= StdDbgLevel())
            PrintActiveNumber(currentSchemas.size());
    }

    return result;
}

} //NAddressFilter
