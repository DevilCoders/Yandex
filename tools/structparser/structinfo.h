#pragma once

#include "partinfo.h"

struct TStructInfo
{
    struct TMember
    {
        ui32 BitField; //or ParamCount for complex func
        ui32 TemplParams;
        TString Name;
        ui32 Pos;
        const TStructInfo* LastStruct; //in using case that is last one's
        E_ACCESS Access;

        const TStructInfo* ReturnTypePtr; //if we do know which type is it
        TFullType ReturnType;


        bool Func;
        bool Static;
        bool Const;//only first one!
        bool IsType;
        bool Overloaded;
        TMember() : Access(EA_NONEXISTENT), Func(false), Static(false) {}
        TMember(E_ACCESS a, TString n, bool f, bool s, bool c, bool t, ui32 bf, ui32 tp, ui32 p, const TStructInfo* l)
            : BitField(bf), TemplParams(tp), Name(n), Pos(p), LastStruct(l), Access(a), Func(f), Static(s), Const(c), IsType(t), Overloaded(false) {}

        bool operator < (const TMember& r) const
        {
            if (Name == r.Name)
                return Static > r.Static;
            return Name < r.Name;
        }
        bool operator < (const TString& n) const
        {
            return Name < n;
        }
        bool operator == (const TMember& r) const
        {
            return Name == r.Name && BitField == r.BitField && Func == r.Func && Static == r.Static && Const == r.Const && Access == r.Access;
        }
    };
    struct TMemberByPos
    {
        bool operator()(const TMember& l, const TMember& r) const
        {
            return l.Pos < r.Pos;
        }
    };
    TStructInfo()
        : HasRecordSig(false), MembersSorted(false),  NoExport(false), TypeDef(false), HasNonTrivialConstructor(false)
        , HasTrivialConstructor(false), HasUsing(false) { }
    TTemplHdr TemplParam;
    TFullType SelfName;
    TList<TFullType> BaseTypes;
    TList<const TStructInfo*> BasePtrs;
    TBitMap<128> BaseTemplParams;
    TVector<TMember> Members;
    bool HasRecordSig;
    bool MembersSorted;
    bool NoExport;
    bool TypeDef;
    bool HasNonTrivialConstructor;
    bool HasTrivialConstructor;
    bool HasUsing;

    TString Error;
    ui32 ErrorLine;
    TString FullName;

    bool Exportable() const
    {
        return !NoExport && TemplParam.empty();
    }
    bool DefCtor() const
    {
        return HasTrivialConstructor || !HasNonTrivialConstructor;
    }

    bool HasUsingInBase() const
    {
        if (HasUsing)
            return true;
        for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != BasePtrs.end(); ++i)
            if ((*i)->HasUsingInBase())
                return true;
        return false;
    }

    void ExportBaseNames(TVector<TString>& names, bool includeTemplate) const //including self
    {
        if (TemplParam.empty() || includeTemplate)
            names.push_back(FullName);
        for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != BasePtrs.end(); ++i)
            (*i)->ExportBaseNames(names, includeTemplate);
    }

    bool HasManyInheritance() const
    {
        if (BaseTypes.size() > 1)
            return true;
        for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != BasePtrs.end(); ++i)
            if ((*i)->HasManyInheritance())
                return true;
        return false;
    }
    TString ErrReason(bool ignoreCtorTemplInt = false, bool ignctor = false, bool ignororderproblems = false) const
    {
        if (Error.size() > 0)
            return Error + ":" + ToString(ErrorLine); //has parse error somethere;

        for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != BasePtrs.end(); ++i)
        {
            TString errr = (*i)->ErrReason(true, true, ignororderproblems); //for base classes ctor is not important
            if (errr.size() > 0)
                return (*i)->SelfName.back().front() + ":" + errr;
        }

        if (!ignororderproblems && HasUsing)
            return "Using";
        if (!ignororderproblems && BaseTypes.size() > 1)
            return "ComplInh";
        if (!ignctor && !DefCtor())
            return "Ctor";
        if (!TemplParam.empty() && !ignoreCtorTemplInt)
            return "template";
        if (NoExport && !ignoreCtorTemplInt)
            return "Internal"; //inside some type;
        return TString();
    }
    //todo remake this to generators model {{bases*},current} (if member exist in more than one base it's gone, if it exists in current then if it is public it goes up)
    void GetPublicMembersRec(TVector<TMember>& to, TSet<TString>& alreadyvis, bool& nonvisible, ui32& offset, const TStructInfo* parent) const
    {
        TList<TSet<TString>::iterator> todel;

        int mbeg = to.ysize();
        for (TVector<TMember>::const_iterator i = Members.begin(); i != Members.end(); ++i)
        {
            std::pair<TSet<TString>::iterator, bool> r = alreadyvis.insert(i->Name);
            if (r.second || (i != Members.begin() && (i - 1)->Name == i->Name)) //overloaded * exported here
            {
                if (r.second)
                    todel.push_back(r.first);

                if (i->Access == EA_PUBLIC)
                    to.push_back(*i);
                else if (!i->Func && !i->Static && !i->IsType)
                    nonvisible = true;
            }
            else if (!i->Func && !i->Static && !i->IsType)
                nonvisible = true;
        }
        int mend = to.ysize();

        //base class processing with template param base reordering
        //here we try to reorder back, that template base type param this works correctly for one inheritance only
        int wsz = (int)BasePtrs.size();
        if (BaseTypes.size() != 1 || (wsz == 2 && parent != nullptr) || (wsz == 1 && parent == nullptr) || (wsz == 0 && parent != nullptr))
        {
            //only fully walid if (wsz == 0 && parent != 0) || (wsz == 1 && parent == 0)
            if (parent)
                parent->GetPublicMembersRec(to, alreadyvis, nonvisible, offset, nullptr);

            for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != BasePtrs.end(); ++i)
                (*i)->GetPublicMembersRec(to, alreadyvis, nonvisible, offset, nullptr);
        }
        else //BaseTypes.size() == 1 && (wsz != 2 || parent == 0)) && (wsz != 1 || parent != 0)
        {
            if (parent == nullptr && wsz == 2)
                BasePtrs.back()->GetPublicMembersRec(to, alreadyvis, nonvisible, offset, BasePtrs.front());
            else if (parent != nullptr && wsz == 1)
                BasePtrs.back()->GetPublicMembersRec(to, alreadyvis, nonvisible, offset, parent);
            else
                Y_FAIL("can't be possible");
        }

        //offset was correctly updated;
        for (int i = mbeg; i < mend; ++i)
            to[i].Pos += offset;

        while (!todel.empty())
        {
            alreadyvis.erase(todel.back());
            todel.pop_back();
        }
        offset += Members.size();
    }
    bool GetPublicMembers(TVector<TMember>& to) const
    {
        TSet<TString> s;
        bool nonvisible = false;
        ui32 offset = 0;
        GetPublicMembersRec(to, s, nonvisible, offset, nullptr);
        ::Sort(to.begin(), to.end());

        int j = 0;//leaving static but removing non uniq
        for (int i = 0; i < to.ysize(); ++i)
        {
            int k = i + 1;
            bool diffbases = false;
            for(; k < to.ysize() && to[i].Name == to[k].Name; ++k)
                if (to[i].LastStruct != to[k].LastStruct)
                    diffbases = true;
            if (diffbases)
                i = k;
            else
                for (; i < k; ++i)
                {
                    if (to[i].Const && !to[i].Func && !to[i].Static && !to[i].IsType)
                        nonvisible = true;
                    to[j++] = to[i];
                }
            --i; //compensating wrong ++ in 2 previous for
        }

        if (!nonvisible)
            nonvisible = (to.ysize() != j);
        to.resize(j);
        return !nonvisible;
    }
    void FinalizeFill()
    {
        ::Sort(Members.begin(), Members.end());
        for (int i = 1; i < Members.ysize(); ++i)
            if (Members[i - 1].Name == Members[i].Name)
                Members[i - 1].Overloaded = Members[i].Overloaded = true;
        MembersSorted = true;
    }
    const TMember* FindOneMember(const TString& n) const
    {
        if (MembersSorted)
        {
            TVector<TMember>::const_iterator l = ::LowerBound(Members.begin(), Members.end(), n);
            if (l != Members.end() && l->Name == n)
                return &(*l);
        }
        else
            for (TVector<TMember>::const_iterator l = Members.begin(); l != Members.end(); ++l)
                if (l->Name == n)
                    return &(*l);
        return nullptr;
    }
    typedef std::pair<TVector<TMember>::const_iterator, TVector<TMember>::const_iterator> TMemberRange;
    TMemberRange FindMembers(const TString& n) const
    {
        Y_VERIFY(MembersSorted, "must be sorted");
        TVector<TMember>::const_iterator b = ::LowerBound(Members.begin(), Members.end(), n);
        TVector<TMember>::const_iterator e = b;
        while (e != Members.end() && e->Name == n)
            ++e;
        TMemberRange r(b, e);
        if (b == e)
        {
            if (BasePtrs.empty())
                return r;
            Y_VERIFY(!BaseTypes.empty(), "just wrong logic");
            TMemberRange r = BasePtrs.back()->FindMembers(n);
            if (++BaseTypes.begin() == BaseTypes.end() && BasePtrs.size() < 3)
            {//template base problem workaround (will not work correctly with mutliple inheritance)
                if (r.first != r.second)
                    return r;
                if (BasePtrs.size() > 1)
                {
                    Y_VERIFY(BasePtrs.size() == 2, "just wrong logic");
                    return BasePtrs.front()->FindMembers(n);
                }
            }
            for (TList<const TStructInfo*>::const_iterator i = BasePtrs.begin(); i != --BasePtrs.end(); ++i)
            {
                TMemberRange r2 = (*i)->FindMembers(n);
                if (r.first == r.second)
                    r = r2;
                else if (r2.first != r2.second)
                    return TMemberRange(Members.end(), Members.end()); //conflicting ids
            }
        }
        return r;
    }


    static void AddNamePart(const TSubType& f, TString& ret)
    {
        if (!f.empty())
            ret += f.front();
        for (size_t j = 1; j < f.size(); ++j)
            ret += "%"; //template parameter
    }
    static TString GenName(const TFullType& name, const TString& curParentType)
    {
        TString ret;
        if (!name.front().empty())
        {
            ret += curParentType;
            ret += ".";
        }
        for (TFullType::const_iterator i = name.begin(); i != name.end(); ++i)
        {
            AddNamePart(*i, ret);
            if (i != --name.end())
                ret += ".";
        }
        return ret;
    }
    const TStructInfo* FindSetBaseType(const TFullType& w, TString curParentType, TMap<TString,TStructInfo>& from, bool readOnly = false)
    {
        if (w.begin() != w.end() && ++w.begin() == w.end() && w.front().begin() != w.front().end() && ++w.front().begin() == w.front().end())
        {//template base linking
            TTemplHdr::const_iterator k = TemplParam.begin();
            for (int z = 0; k != TemplParam.end(); ++k, ++z)
                if (k->first == TString() && k->second == w.front().front())
                {
                    if (!readOnly)
                        BaseTemplParams.Set(z);
                    break;
                }
            if (k != TemplParam.end())
            {
                //printf("template base found\n");
                return this;
            }
        }

        while(true)
        {
            TMap<TString,TStructInfo>::const_iterator p = from.find(GenName(w, curParentType));
            if (p != from.end())
                return &p->second;
            if (curParentType.size() == 0)
                return nullptr;
            size_t r = curParentType.rfind('.');
            Y_VERIFY(r != TString::npos, "we should find it");
            curParentType = curParentType.substr(0, r);
        }
    }
    explicit TStructInfo(TPartParser& f, const TString& curParentType, TMap<TString,TStructInfo>& from)
        : MembersSorted(false),  NoExport(false), TypeDef(false)
        , HasNonTrivialConstructor(false), HasTrivialConstructor(false), HasUsing(false)
    {
        //TODO HERE GOES NAME RESOLUTION
        DoSwap(TemplParam, f.TemplParam);
        DoSwap(BaseTypes, f.AllTypes);
        DoSwap(SelfName, BaseTypes.front());
        BaseTypes.pop_front();

        bool allOk = true;
        for (TList<TFullType>::const_iterator i = BaseTypes.begin(); i != BaseTypes.end(); ++i)
        {
            const TStructInfo* b = FindSetBaseType(*i, curParentType, from);
            if (b == this) //template parameter
                continue;
            if (b == nullptr)
                allOk = false;
            else
            {
                BasePtrs.push_back(b);
                if (!b->BaseTemplParams.Empty())
                {
                    Y_VERIFY(b->TemplParam.size() == i->back().size() - 1, "wrong number of template args (type search should not be matched by %% !!!) %d != %d %s", (int)b->TemplParam.size(), (int)i->size() - 1, GenName(*i, TString()).data());
                    TSubType::const_iterator z = i->back().begin();
                    //back here is last herarchy level i.e.i->back().front() short template struct name
                    z++;
                    for (int p = 0; z != i->back().end(); ++z, ++p)
                        if (b->BaseTemplParams.Get(p))
                        {
                            TFullType parserNotSupported;
                            TSubType parserNotSupported2;
                            parserNotSupported2.push_back(*z);
                            parserNotSupported.push_back(parserNotSupported2);
                            const TStructInfo* bp = FindSetBaseType(parserNotSupported, curParentType, from);
                            if (bp != this) //self template param
                                if (bp == nullptr)
                                    allOk = false;
                                else
                                    BasePtrs.push_front(bp);
                            //if (allOk)
                              //  printf("!very complex template param linked!\n");
                        }
                }
            }
        }

        if (!allOk)
        {
            printf("base not found line for : %s.", curParentType.data());
            TPartParser::Print(SelfName);
            printf("\n");
            SelfName.clear(); //we can't parse that struct because we don't know base for it
        }

        //printf("dbg: ");
        //TPartParser::Print(SelfName);
        //printf(":");
        //for (TList<TFullType>::const_iterator i = BaseTypes.begin(); i != BaseTypes.end(); ++i)
          //  TPartParser::Print(*i);
    }
    void Swap(TStructInfo& o)
    {
        DoSwap(TemplParam, o.TemplParam);
        DoSwap(BaseTypes, o.BaseTypes);
        DoSwap(SelfName, o.SelfName);
        DoSwap(BasePtrs, o.BasePtrs);
        DoSwap(BaseTemplParams, o.BaseTemplParams);
        DoSwap(NoExport, o.NoExport);
        DoSwap(Members, o.Members);
        DoSwap(MembersSorted, o.MembersSorted);

        DoSwap(TypeDef, o.TypeDef);
        DoSwap(Error, o.Error);
        DoSwap(ErrorLine, o.ErrorLine);
        DoSwap(HasNonTrivialConstructor, o.HasNonTrivialConstructor);
        DoSwap(HasTrivialConstructor, o.HasTrivialConstructor);
        DoSwap(HasUsing, o.HasUsing);
        DoSwap(FullName, o.FullName);
    }
    TString GenFullName(const TString& curParentType)
    {
        //don't try make complex parsing
        if (SelfName.empty() || (++SelfName.begin()) != SelfName.end() || SelfName.front().empty() || (++SelfName.front().begin()) != SelfName.front().end())
            return TString();
        TString ret = GenName(SelfName, curParentType);
        for (size_t i = 0; i < TemplParam.size(); ++i)
            ret += "%"; //template parameter
        return ret;
    }
};
