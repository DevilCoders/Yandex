#include "parsestruct.h"

    //can be united (Enum, Class, Struct to simplify automate states and ...)
    void TStructParser::EnableSkipIfNeeded(bool markerr)
    {
        if (Part.Grammer.back() != ';')
        {
            Y_VERIFY(Part.Grammer.back() == '{', "interface error");
            LastValidLev = CurLevel;
        }
        if (CurStruct && markerr)
        {
            CurStruct->Error = FOffset.Fname;
            CurStruct->ErrorLine = FOffset.LineOffset + Line;
        }
    }
    void TStructParser::TryAddTypeName()
    {
        if (!Part.AllTypes.empty() && Part.AllTypes.front().size() == 1 && CurStruct != nullptr)
        {
            TString name;
            TStructInfo::AddNamePart(Part.AllTypes.front().front(), name);
            for (size_t i = 0; i < Part.TemplParam.size(); ++i)
                name += "%"; //template parameter
            CurStruct->Members.push_back(TStructInfo::TMember(Part.CurAccess, name, false, true, true, true, (ui32)Part.TemplParam.size(), (ui32)Part.TemplParam.size(), CurStruct->Members.size(), CurStruct));
        }
        if (Part.Grammer.back() == '{')
            TypeDefined.Set(CurLevel);
    }
    void TStructParser::PartToEnum()
    {
        TryAddTypeName();
        Y_VERIFY(LastValidLev == -1, "we should not launch parser in error mode");
        //todo enum parser enable;

        EnableSkipIfNeeded(false); //should we mark as error?
    }
    void TStructParser::PartToClass()
    {
        TryAddTypeName();
        //todo class parser enable;
        Y_VERIFY(LastValidLev == -1, "we should not launch parser in error mode");

        EnableSkipIfNeeded(false); //should we mark as error?
    }


    void TStructParser::PartToStruct(bool td, bool noexport)
    {
        Y_VERIFY(LastValidLev == -1, "we should not launch parser in error mode");
        TryAddTypeName();
        if (!td)
            if (Part.Grammer.back() == ';') //struct predifinition
                return;
        if (Part.AllTypes.empty()) //struct {
        {
            EnableSkipIfNeeded(false); //should we mark as error?
            return;
        }
        TStructInfo s(Part, CurParentType, ParsedStructs);
        if (noexport)
            s.NoExport = true;
        TString nname = s.GenFullName(CurParentType);
        if (nname.size() == 0 || (CurStruct && !td) || !Part.Literals.empty() || !Part.Ids.empty())
        {
            printf("too complex struct in line %d (%s:%d)\n", Line, FOffset.Fname.data(), FOffset.LineOffset + Line);
            if (!td)
                EnableSkipIfNeeded(false); //should we mark as error?
        }
        else
        {
            //printf("parsed struct %s : ", nname);
            //for (TList<TFullType>::iterator i = s.BaseTypes.begin(); i != s.BaseTypes.end(); ++i, printf(","))
                //printf("%s", ~TStructInfo::GenName(*i, CurParentType));
            //printf("\n");

            if (!td)
            {
                GoToNameSpace((ui8)s.SelfName.size(), s.GenFullName(TString()));
                CurStruct = &ParsedStructs[nname];
                CurStructLevel = CurLevel;
                Part.CurAccess = EA_PUBLIC;
            }
            else//typedef
            {
                if (CurStruct)
                    s.NoExport = true;
                s.TypeDef = true;
                s.FinalizeFill();
            }
            s.FullName = nname;
            s.Swap(ParsedStructs[nname]);
            Y_VERIFY(s.SelfName.empty(), "already has that struct %s", nname.data());
        }
    }
    void TStructParser::UsingType()
    {
        Y_VERIFY(Part.AllTypes.size() == 2 && Part.Ids.empty() && Part.CurId == 0, "wrong parsers interface");
        bool noexport = true;
        for (TFullType::const_iterator i = Part.AllTypes.back().begin(); i != Part.AllTypes.back().end(); ++i)
            if (++i->begin() != i->end())
                noexport = false;
        PartToStruct(true, noexport);
    }
    void TStructParser::PartToTypeDef()
    {
        bool complex = Part.Grammer[Part.Grammer.size() - 2] == ']';
        Y_VERIFY(Part.AllTypes.size() == 1 && (Part.Ids.size() == 1 || complex) && Part.CurId == 1, "wrong parsers interface");
        Part.AllTypes.push_front(TFullType());
        Part.AllTypes.front().push_front(TSubType());
        Part.AllTypes.front().front().push_front(Part.Ids.front());
        Part.Ids.clear();
        Part.CurId = 0;
//         if (CurStruct != 0 && Part.AllTypes.front().front().front() == TString("TExtInfo"))
//             printf("hack ignoring typedef TExtInfo");
//         else

        bool noexport = true;
        for (TFullType::const_iterator i = Part.AllTypes.back().begin(); i != Part.AllTypes.back().end(); ++i)
            if (++i->begin() != i->end())
                noexport = false;

        if (complex)
            TryAddTypeName();
        else
            PartToStruct(true, noexport);
    }
    void TStructParser::PartToMember(bool com)
    {
        Y_VERIFY(Part.AllTypes.size() == 1 && (Part.CurId == (int)Part.Ids.size() || com) && Part.CurId > 0, "interface error");
        Y_VERIFY(!com || Part.CurId == 1, "interface error");

        //printf("mebmer def: ");
        //Part.Print(Part.AllTypes.front());
        //printf(" ids = %d, literals = %d\n", (int)Part.Ids.size(), (int)Part.Literals.size());

        bool st = Part.Grammer[0] == 's' || Part.Grammer[1] == 's';
        bool cst = Part.AllTypes.front().LastConst();

        ui32 bf = 0;
        if (!Part.BitFieldWidth.empty())
            bf = (ui32)Min<ui32>(255, FromString<ui32>(Part.BitFieldWidth));

        if (CurStruct)
            for(;Part.CurId; --Part.CurId, Part.Ids.pop_front())
                CurStruct->Members.push_back(TStructInfo::TMember(Part.CurAccess, Part.Ids.front(), false, st, cst, false, bf, 0, CurStruct->Members.size(), CurStruct));

        Part.Ids.clear();
        Part.CurId = 0;
        Part.AllTypes.clear();
    }
    void TStructParser::UsingMember()
    {
        Y_VERIFY(Part.AllTypes.size() == 1, "interface error line %d (%s:%d)", Line, FOffset.Fname.data(), FOffset.LineOffset + Line);
        bool grammerValid = Part.AllTypes.front().size() > 1 && Part.AllTypes.front().back().size() == 1;
        Y_VERIFY(grammerValid, "interface/grammar error");

        TFullType f;
        f.swap(Part.AllTypes.front());
        Part.AllTypes.clear();
        TString member = f.back().front();
        f.pop_back();
        if (CurStruct)
        {
            const TStructInfo* s = CurStruct->FindSetBaseType(f, CurParentType, ParsedStructs, true);
            int found = 0;
            if (s == CurStruct)
                fprintf(stderr, "using member : referenced class is template param will be ignored %d (%s:%d)\n", Line, FOffset.Fname.data(), FOffset.LineOffset + Line);
            else if (s != nullptr)
            {
                TStructInfo::TMemberRange membs = s->FindMembers(member);
                for (TVector<TStructInfo::TMember>::const_iterator p = membs.first; p != membs.second; ++p)
                    if (p->Access != EA_PRIVATE) //for more base classess we should search only for public fields
                    {
                        Y_VERIFY(p->Name == member, "wrong logic");
                        CurStruct->Members.push_back(*p);
                        CurStruct->Members.back().Access = Part.CurAccess;
                        CurStruct->Members.back().Pos = CurStruct->Members.size() - 1;
                        CurStruct->Members.back().Overloaded = false;//will be set later
                        if (!CurStruct->Members.back().IsType && !CurStruct->Members.back().Static && !CurStruct->Members.back().Func)
                            CurStruct->HasUsing = true;
                        ++found;
                    }
            }

            if (found == 0)
            {
                fprintf(stderr, "member %s not found  %d (%s:%d)", member.data(), Line, FOffset.Fname.data(), FOffset.LineOffset + Line);
                CurStruct->Error = FOffset.Fname;
                CurStruct->ErrorLine = FOffset.LineOffset + Line;
            }
        }
    }
    void TStructParser::SkipTry()
    {
        fprintf(stderr, "warning try top level construct\n");
    }
    void TStructParser::SkipFunc()
    {
        EnableSkipIfNeeded(false); //should we mark as error?
        Part.FuncName.clear();
    }
    void TStructParser::SimpleFunc(bool complex)
    {
        Y_VERIFY(Part.FuncName.size() > 0, "interface error");
        if (!complex)
            Y_VERIFY(Part.AllTypes.size() == 1 && (Part.CurId == (int)Part.Ids.size() ) && Part.CurId == 0 && Part.FuncName.size() > 0, "interface error");

        bool st = Part.Grammer[0] == 's' || Part.Grammer[1] == 's';
        bool cst = Part.ConstFunction;

        if (complex)
            Y_VERIFY(Part.FuncParams > 0, "should be many func params");
        else
            Y_VERIFY(Part.FuncParams == 0, "should be many func params");

        ui32 fp = (ui32)Part.FuncParams;

        if (CurStruct)
        {
            CurStruct->Members.push_back(TStructInfo::TMember(Part.CurAccess, Part.FuncName, true, st, cst, false, fp, (ui32)Part.TemplParam.size(), CurStruct->Members.size(), CurStruct));
            DoSwap(CurStruct->Members.back().ReturnType, Part.AllTypes.front());
            CurStruct->Members.back().ReturnTypePtr = CurStruct->FindSetBaseType(CurStruct->Members.back().ReturnType, CurParentType, ParsedStructs, true);
        }

        SkipFunc();
    }
    void TStructParser::Oper()
    {
        SimpleFunc(Part.FuncParams > 0);
    }
    void TStructParser::ConvOper()
    {
        Part.FuncName = "<";
        SimpleFunc(false);
    }

    void TStructParser::Ctor()
    {
        if (CurStruct != nullptr)
        {
            Y_VERIFY(Part.FuncName == CurStruct->SelfName.back().front(), "wrong ctor name");
            if (Part.DefFuncParams != Part.FuncParams)
                CurStruct->HasNonTrivialConstructor = true;
            else if (Part.CurAccess == EA_PUBLIC)
                CurStruct->HasTrivialConstructor = true;
            else
                CurStruct->HasNonTrivialConstructor = true; //we have non accessible default constructor here!
        }

        SkipFunc();
    }
    void TStructParser::Detor()
    {
        if (CurStruct != nullptr)
            Y_VERIFY(Part.FuncName == CurStruct->SelfName.back().front(), "wrong ~ name");
        SkipFunc();
    }
    void TStructParser::GoToNameSpace(ui8 levs, TString name)
    {
        if (name[0] != '.')
            CurParentType += ".";
        CurParentType += name;
        NameSpaceLevels.push_back(levs);
    }
    void TStructParser::GoToNameSpace()
    {
        Y_VERIFY(Part.Ids.size() == 1, "todo we need more complex ways %d", (int)Part.Ids.size());
        GoToNameSpace(1, Part.Ids.front());
    }

