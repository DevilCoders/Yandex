#include "parsestruct.h"
#include <util/string/split.h>

    void TStructParser::Parse()
    {
        Y_VERIFY(CurSpecPos == std::make_pair(-1, -1), "can't parse then spec attribute marker is not closen");
        if (LastValidLev == -1)
        {
            //printf("dbg : ");
            //for (size_t i = 0; i < Part.Grammer.size(); ++i)
              //  printf("%c", Part.Grammer[i]);
            //printf("\n");

            Y_VERIFY(!Part.Grammer.empty(), "parsing empty data");
            //hack to remove wrong struct ... {...}
            if (Part.Grammer.back() == ';' && Part.Grammer.size() == 2 && Part.Grammer.front() == 'I' &&
                Part.Ids.front() == UndefType())
            {
                Part.Ids.pop_front();
                Part.Grammer.clear();
                Part.Grammer.push_back(';');
            }

            if (Part.Grammer.size() > 1 || Part.Grammer[0] != ';')
            {
                //bool mustParse = Part.Ids.back() == TString("RecordSig");
                for (TList<TString>::iterator i = Part.Ids.begin(); i != Part.Ids.end(); i++)
                    if (*i == RecordSigField)
                        CurStruct->HasRecordSig = true;
                if (Part.Grammer.back() == ';')
                    TypeDefined.Reset(CurLevel);
                char* r = ParseP(Part.Grammer);
                if (r)
                {
                    printf("parser error line %d pos %d %c (in %s:%d) : ", Line,  (int)(r - Part.Grammer.begin()), *r, FOffset.Fname.data(), FOffset.LineOffset + Line);
                    for (size_t i = 0; i < Part.Grammer.size(); ++i)
                        printf("%c", Part.Grammer[i]);
                    printf("\n");
                    if (Part.Grammer.back() != ';')
                        LastValidLev = CurLevel;
                    if (CurStruct)
                    {
                        CurStruct->Error = FOffset.Fname;
                        CurStruct->ErrorLine = FOffset.LineOffset + Line;
                    }
                }
            }
//            printf(" ids processed %d\n\n", Part.CurId);
        }
        Part.Clear(); //todo check error
        //printf("\n");
    }
    void TStructParser::In()
    {
        /*
        A('{');
        CurLevel++;
        ProcOBraceSymb(0, '{');

        if (Braces.size() == (size_t)CurLevel)
        {
            Parse();

            if (NameSpaceLevels.size() < (size_t)CurLevel)
                NameSpaceLevels.push_back((ui8)0);
            Y_VERIFY(NameSpaceLevels.size() == (size_t)CurLevel, "wrong lev line %d (in %s:%d) : ", Line,  ~FOffset.Fname, FOffset.LineOffset + Line);
        }
        else
            printf("can't parse { inside other braces\n");
            */

        A('{');
        if (Braces.size() == (size_t)CurLevel)
            Parse();
        else
            printf("can't parse { inside other braces\n");

        CurLevel++;
        ProcOBraceSymb(0, '{');

        if (Braces.size() == (size_t)CurLevel)
        {
            if (NameSpaceLevels.size() < (size_t)CurLevel)
                NameSpaceLevels.push_back((ui8)0);
            Y_VERIFY(NameSpaceLevels.size() == (size_t)CurLevel, "wrong lev line %d (in %s:%d) : ", Line,  FOffset.Fname.data(), FOffset.LineOffset + Line);
        }

    }
    //can be united (Enum, Class, Struct to simplify automate states and ...)

    void TStructParser::SpecId(const char* b, const char* e)
    {
        if (CurSpecPos == std::make_pair(-1, -1))
        {
            Part.Ids.push_back(TString(b, e));
            A('I');
        }
        else
            A('B');
    }
    void TStructParser::ProcConstType(const char* b, const char* e)
    {
        if (CurSpecPos == std::make_pair(-1, -1))
        {
            if (Part.Grammer.size() && Part.Grammer.back() == 'I' && Part.Ids.back()[0] == '!')
                Part.Ids.back() += " ";
            else
            {
                Part.Ids.push_back("!");
                A('I');
            }
            Part.Ids.back() += TStringBuf(b, e);
        }
        else
            A('B');
    }


    void TStructParser::Out()
    {
        A('}');
        //printf("\n");
        --CurLevel;
        ProcCBraceSymb(0, '}');

        if (Braces.size() == (size_t)CurLevel)
        {
            //TODO ENUM CONST PARSING INVALID HERE!
            Part.Clear();

            while (NameSpaceLevels.back() > 0)
            {
                size_t p = CurParentType.rfind('.');
                Y_VERIFY(p != TString::npos, "we should find it");
                CurParentType = CurParentType.substr(0, p);
                --NameSpaceLevels.back();
            }
            NameSpaceLevels.pop_back();


            if (CurStruct != nullptr && CurLevel == CurStructLevel)
            {
                CurStruct->FinalizeFill();
                CurStruct = nullptr;
                //printf("parser disabled\n");
            }
            else
                if (LastValidLev == CurLevel && (Braces.size() == (size_t)CurLevel))
                {
                    LastValidLev = -1;
                    //printf("parser restored\n");
                }

            if (TypeDefined.Get(CurLevel))
            {
                TypeDefined.Reset(CurLevel);
                TStringBuf b(UndefType());
                SpecId(b.begin(), b.end());
            }
            if (CurLevel == 0)
                Y_VERIFY(NameSpaceLevels.size() == (size_t)CurLevel, "wrong lev line %d (in %s:%d) : ", Line,  FOffset.Fname.data(), FOffset.LineOffset + Line);
        }
    }
    void TStructParser::ProcEndEqSymb()
    {
        A(';');
        if (Braces.size() == (size_t)CurLevel)
            Parse();
        else if (LastValidLev == -1)
            printf("can't parse ; inside braces\n");
        //printf("\n");
    }


    void TStructParser::ProcOBraceSymb(char id, char c)
    {
        if (id > 0)
            A(c);
        Braces.push_back(id);
    }
    void TStructParser::ProcCBraceSymb(char id, char c)
    {
        if (id > 0)
            A(c);
        Y_VERIFY(!Braces.empty() && Braces.back() == id, "non pair %c", c);
        Braces.pop_back();

        if ((int)Braces.size() == CurSpecPos.second)
        {
            Y_VERIFY((int)Part.Grammer.size() >= CurSpecPos.first, "wrong pos");
            Part.Grammer.resize(CurSpecPos.first);
            CurSpecPos = std::make_pair(-1, -1);
        }
    }
    void TStructParser::SpecAttribKeyword(bool op)
    {
        if (CurSpecPos == std::make_pair(-1, -1))
            CurSpecPos = std::make_pair((int)Part.Grammer.size(), (int)Braces.size());

        if (op)
        {
            A('[');
            ProcOBraceSymb(7, '[');
        }
    }
    void TStructParser::SpecAttribKeywordClose()
    {
        A(']');
        if (CurSpecPos == std::make_pair(-1, -1))
        {
            ProcCBraceSymb(2, ']'); //normal ']' not ']]'
            ProcCBraceSymb(2, ']'); //normal ']' not ']]'
        }
        else
            ProcCBraceSymb(7, ']'); //']]'
    }

    struct TMyLineConsume
    {
        int Tokens;
        TFileOffset Finfo;
        TMyLineConsume() : Tokens(0) {}
        inline bool Consume(const char* b, const char* e, const char*)
        {
            if (b != e)
            {
                switch (Tokens)
                {
                case 0: break; // #line
                case 1:
                    try
                    {
                        Finfo.LineOffset = FromString(TStringBuf(b, e));
                    }
                    catch (const TFromStringException&)
                    {
                        fprintf(stderr, "can't convert line info\n");
                        Finfo.LineOffset = -1<<20;
                    }
                case 2:
                    Finfo.Fname = TStringBuf(b, e);
                }
                ++Tokens;
            }
            return true;
        }
    };
    void TStructParser::LinePragma(const char* b, const char* e)
    {
        ++Line;

        TMyLineConsume mln;
        TSetDelimiter<const char> delim("\t \r\n");
        SplitString<const char*>(b, e, delim, mln);
        FOffset = mln.Finfo;
        FOffset.LineOffset -= Line;
    }

