#pragma once

#include "structinfo.h"

struct TFileOffset
{
    int LineOffset;
    TString Fname;
};

struct TStructParser
{
    TStructParser()
        : CurLevel(0)
        , CurStructLevel(-1)
        , CurSpecPos(std::make_pair(-1,-1))
        , LastValidLev(-1)
        , CurStruct(nullptr) {}

    // For ragel

    TString RecordSigField;
    TMap<ui32, TStructInfo*> UsedRecordSigs;
    TFileOffset FOffset;
    int Line;
    int CurLevel;
    int CurStructLevel;
    std::pair<int, int> CurSpecPos; //used for ignoring various attributes

    int LastValidLev;
    TBitMap<1024> TypeDefined;


    TString CurParentType;
    TMap<TString, TStructInfo> ParsedStructs;

    TVector<int> RagelGrammerStack;


    TPartParser Part; //partial parsed/lexed constructs

    TStructInfo* CurStruct; //if we need parse hierarchy make list(or vector) here (list of pairs with CurStructLevel!)
    TVector<ui8> NameSpaceLevels;
    TVector<char> Braces;

    char* ParseP(TVector<char>& c); //lexer start
    void Parse(); //parser start

    static TStringBuf UndefType()
    {
        return "T!!!Invalid!!!Type!!!"sv;
    }

    //post lexer begin
    void A(char c)
    {
        //printf("%c", c);
        Part.Grammer.push_back(c);
    }
    void AL(const TString& l)
    {
        if (CurSpecPos == std::make_pair(-1, -1))
        {
            Part.Literals.push_back(l);
            A('L');
        }
        else
            A('B');
    }

    void ProcSymb(char s)
    {
        A(s);
    }
    void ProcDef()
    {
        if (Part.Grammer.size() == 1 && Part.Grammer.front() >= '0' && Part.Grammer.front() < '3' && CurSpecPos == std::make_pair(-1, -1))
        {
            Y_VERIFY(Braces.size() == (size_t) CurLevel, "wrong level for access");
            Part.CurAccess = (E_ACCESS)(Part.Grammer.front() - '0');
            Part.Grammer.clear();
        }
        else
            A(':');
    }
    //various literals
    void Float(const char* b, const char* e)  { AL(TString(b,e));}
    void ProcBool(bool v) {AL(TString(v? "true":"false"));}
    void Int(const char* b, const char* e)  { AL(TString(b,e));}
    void HexInt(const char* b, const char* e)  { AL(TString(b,e));}
    void OctInt(const char* b, const char* e)  { AL(TString(b,e));}
    void ProcLit(const char* b, const char* e)  { AL(TString(b,e));}

    void In();
    void Spec(char s) {A(s);}
    void SpecId(const char* b, const char* e);
    void ProcConstType(const char* b, const char* e);
    void Out();

    void ProcOBraceSymb(char id, char c);
    void ProcCBraceSymb(char id, char c);

    void ProcEndEqSymb();
    void ProcSymbType() {  A('.'); }  // using '.' instead "::". dot "." will not be analyzed (we cast :: -> .)


    void ProcNOp(const char* b, const char* e)
    {
        Y_VERIFY(e - b == 2, "lexer error");
        A('O');
        A(b[0]);
        A(b[1]);
    }
    void ProcOpEq(const char* b, const char* e)
    {
        Y_VERIFY(e - b == 2, "lexer error");
        A('O');
        A(b[0]);
        A(b[1]);
    }
    void ProcNOp3(const char* b, const char* e)
    {
        Y_VERIFY(e - b == 3, "lexer error");
        A('P');
        A(b[0]);
        A(b[1]);
        A(b[2]);
    }

    void ProcDot()                          {   A('B');  }

    const char* Parse(const char* b, const char* e);

    void SpecAttribKeyword(bool op);
    void SpecAttribKeywordClose();
    void LinePragma(const char*, const char*);
    void OtherPragma(const char*, const char*)
    {
        printf("uknown pragma at %d\n", Line);
    }
    //post lexer end

    //can be united (Enum, Class, Struct to simplify automate states and ...)
    //post parser begin
    void EnableSkipIfNeeded(bool markerr);
    void TryAddTypeName();
    void PartToEnum();
    void PartToClass();

    void PartToStruct(bool td = false, bool noexport = false);
    void UsingType();
    void PartToTypeDef();
    void PartToMember(bool com);
    void UsingMember();
    void SkipTry();
    void SkipFunc();
    void SimpleFunc(bool complex);
    void Ctor();
    void Detor();
    void Oper();
    void ConvOper();
    void GoToNameSpace(ui8 levs, TString name);
    void GoToNameSpace();
    //post parser end

};

