#pragma once

#include <library/cpp/getopt/small/opt.h>

#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/algorithm.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/string/split.h>
#include <utility>

#include <stdarg.h>
#include <stdlib.h>

#include <fcntl.h>

enum E_ACCESS
{
    EA_PUBLIC = 0,
    EA_PROTECTED = 1,
    EA_PRIVATE = 2,
    EA_NONEXISTENT = 255,
};
typedef TList<std::pair<TString, TString> > TTemplHdr;
typedef TList<TString> TSubType; //A<B,c,d>
struct TFullType : TList<TSubType>
{
    TFullType() : PointersCount(0), IsReference(false) {}
    ui32 PointersCount;
    TBitMap<64> Consts;
    bool LastConst() const { return Consts.Get(PointersCount); }
    bool IsReference;
    void Swap(TFullType& o)
    {
        DoSwap<TList<TSubType> > (*this, o);
        DoSwap(PointersCount, o.PointersCount);
        DoSwap(IsReference, o.IsReference);
        DoSwap(Consts, o.Consts);
    }
    void swap(TFullType& o) {Swap(o);}
};
//typedef TList<TSubType> TFullType; //A::B<c,d>::E;

struct TPartParser
{
    TPartParser()
    {
        Clear();
    }
    int CurId;
    int FuncParams;
    int DefFuncParams;
    TVector<char> Grammer;
    E_ACCESS CurAccess;

    TList<TString> Ids;
    TList<TString> Literals;

    TTemplHdr TemplParam;

    TSubType CurSub;
    TFullType CurFullType;
    TList<TFullType> AllTypes;
    bool HasLeadingDot;
    bool EnumClass;
    bool ConstFunction;
    TString FuncName;

    TString BitFieldWidth;

    bool PrefConst;

    void MarkPrefConst()
    {
        PrefConst = true;
    }

    void GetFunctionName()
    {
        Y_VERIFY(CurId == 1 && !Ids.empty(), "wrong parsing");
        FuncName = Ids.front();
        Ids.pop_front();
        CurId = 0;
    }
    void OperName(const char *p)
    {
        const char* b = p;
        while(b >= (p - 5) && *b != 'o')
            --b;
        Y_VERIFY(b >= (p - 5) && *b == 'o', "interface error");
        Y_VERIFY(FuncName.size() == 0, "interface error no function name should be here");
        ++b;
        if (*b == 'P' || *b == 'O')
            ++b;
        Y_VERIFY(b < p, "wrong pos");
        FuncName.reserve(5);
        FuncName = "!";
        FuncName += TStringBuf(b, p);
    }
    void HasDefParam(bool lit)
    {
        if (lit)
        {
            Y_VERIFY(!Literals.empty(), "wrong parsing");
            Literals.pop_front();
        }
        else
        {
            Y_VERIFY(!Ids.empty() && CurId == 2, "interface error");
            --CurId;
            Ids.erase(++Ids.begin());
        }

        DefFuncParams++;
    }
    void DropComplexParams()
    {
        if (CurId != 0) //can be param wo id
        {
            Y_VERIFY(CurId == 1 && !Ids.empty(), "wrong parsing");
            Ids.pop_front();
            CurId = 0;
        }
        FuncParams++;
    }
    static void Print(const TSubType & s)
    {
        if (s.empty())
        {
            printf("!"); //root marker
            return;
        }
        printf("%s", s.front().data());
        TSubType::const_iterator i = s.begin();
        ++i;
        if (i != s.end())
        {
            printf("<");
            for(;i != s.end(); ++i, printf(","))
                printf("%s", i->data());
            printf(">");
        }
    }
    static void Print(const TFullType& f)
    {
        for (TFullType::const_iterator i = f.begin(); i != f.end(); ++i, printf("."))
            Print(*i);
    }
    void Clear()
    {
        DefFuncParams = FuncParams = CurId = 0;
        Grammer.clear();
        Ids.clear();
        HasLeadingDot = false;
        EnumClass = false;
        ConstFunction = false;
        PrefConst = false;
        TemplParam.clear();
        Literals.clear();
        CurSub.clear();
        CurFullType.clear();
        AllTypes.clear();
        FuncName.clear();
        BitFieldWidth.clear();
    }
    void BitField()
    {
        Y_VERIFY(!Literals.empty(), "compatibility error");
        BitFieldWidth = Literals.front();
        Literals.pop_front();
    }

    void TemplHdrIdParsed()
    {
        if (CurId == 1)
        {
            TemplParam.push_back(std::make_pair(TString(), Ids.front()));
            Ids.pop_front(); --CurId;
        }
        else
        {
            TString f = Ids.front();
            Ids.pop_front();--CurId;
            TemplParam.push_back(std::make_pair(f, Ids.front()));
            Ids.pop_front();--CurId;
        }
        Y_VERIFY(CurId == 0, "wrong id");
    }
    void TemplatePrefParsed()
    {
        Y_VERIFY(CurId == 0, "wrong id");
        /*
        printf("parsed : template<");
        for (TTemplHdr::const_iterator i = TemplParam.begin(); i != TemplParam.end(); ++i)
            if (i->First().empty())
                printf("class %s,", i->Second());
            else
                printf("%s %s,", i->First(), i->Second());
        printf(">\n");
        */
    }

    void AddTemplParam()
    {
        Y_VERIFY(CurId == 1 && !Ids.empty(), "wrong parsing");
        CurSub.push_back(Ids.front());
        Ids.pop_front();
        CurId = 0;
    }
    void AddTemplBase()
    {
        AddTemplParam();
        //todo full type parsing???
    }
    void AddTemplLiteral()
    {
        Y_VERIFY(!Literals.empty(), "wrong parsing");
        CurSub.push_back(Literals.front());
        Literals.pop_front();
    }

    void TemplateIdParsed()
    {
        Y_VERIFY(CurId == 0, "wrong id");
        if (HasLeadingDot)
        {
            Y_VERIFY(CurFullType.empty(), "internal err");
            CurFullType.push_back(TSubType());
            HasLeadingDot = false;
        }
        CurFullType.push_back(TSubType());
        CurSub.swap(CurFullType.back());
        //todo full type parsing???
        CurSub.clear();
    }
    void CompexIdsParsed()
    {
        //printf("end of complexid\n");
        Y_VERIFY(HasLeadingDot == false, "internal parsing error");
        AllTypes.push_back(TFullType());
        if (PrefConst)
            CurFullType.Consts.Set(0);
        DoSwap(CurFullType, AllTypes.back());
        CurFullType.clear();
        PrefConst = false;
    }
    void LastCmplTypeAddPointer()
    {
        Y_VERIFY(!AllTypes.empty(), "should CoplexIds be parsed");
        ++AllTypes.back().PointersCount;
    }
    void MarkPostConst()
    {
        AllTypes.back().Consts.Set(AllTypes.back().PointersCount);
    }
    void LastCmplTypeAddReference()
    {
        Y_VERIFY(!AllTypes.empty(), "should CoplexIds be parsed");
        Y_VERIFY(!AllTypes.back().IsReference, "already set reference - parser bug");
        AllTypes.back().IsReference = true;
    }
};
