#include "parsestruct.h"
#include <util/string/subst.h>
#include <util/stream/file.h>
#include <util/stream/buffered.h>
#include <util/generic/ptr.h>
#include <util/stream/printf.h>
#include <util/string/printf.h>

struct TPrnExport
{
    TBufferedOutput& To;
    TPrnExport(TBufferedOutput& to, const TString&) : To(to) {}
    TVector<TString> BaseNames;
    void Export(const TString& n, const TStructInfo& si, bool vis, TVector<TStructInfo::TMember>& members)
    {
        TString erres = si.ErrReason();
        if (erres.size() == 0 && !vis)
            erres += "NAF";

        //             if (+erres == 0)
        //                 continue;
        Printf(To, "%s: %s :(", n.data(),  erres.data());

        BaseNames.clear();
        si.ExportBaseNames(BaseNames, true);

        for (TVector<TString>::iterator i = BaseNames.begin(); i != BaseNames.end(); ++i)
        {
            SubstGlobal(*i, ".","::");
            if (i == BaseNames.begin())
                Y_VERIFY(*i == n, "wrong naming");
            else
                To << *i << ",";
        }

        To << ") {";
        for (TVector<TStructInfo::TMember>::const_iterator i = members.begin(); i != members.end(); ++i)
        {
            const TStructInfo::TMember& m = *i;
            if (m.TemplParams > 0)
                To << "<" << (int) m.TemplParams << '>';
            if (m.IsType)
                To << "*";
            else
            {
                if (m.Const)
                    To << "!";
                if (m.Static)
                    To << "$";
            }
            if (m.Name[0] == '!' || m.Name[0] == '<')
                To << "operator ";
            To << m.Name;
            if (m.Func)
                To << "()";
            if (m.Overloaded)
                To << "+";
            if (m.BitField > 0)
                Printf(To, ":%d", (int)m.BitField);
            To << ",";
        }
        Printf(To, "}\n");

    }
    void End(size_t prntStructs)
    {
        Printf(To, "--- %d\n", (int)prntStructs);
    }
};
namespace NExample
{//better to read generator code, it may be more correct than this example
    struct TA
    {
        int B;
        int F() const { return 0;}
    };

    struct TStructInfoTA//actually template<> TStructInfo<TA>
    {
        struct TAcc_B
        {
            //TTo and TFrom are templates because you can overload return type and operator = if needed
            //register member function will know type for certain that parser is not
            template<class TTo>    static TTo Get(const TA& f) { return TTo(f.B);}
            template<class TFrom>  static void Set(TA& t, TFrom& from) { t.B = from;}
        };
        struct TAxxx_F
        {
            template<class TTo>    static TTo Call(const TA& f) {return TTo(f.F());}
        };
    };
}
struct TDefExport
{
    TBufferedOutput& To;
    TString Namespace;
    TString RecordSigField;
    TString TypeName;
    bool ExportOtherOverload;
    TVector<TString> BaseNames;
    TSet<TString> OverFuncs;
    TDefExport(TBufferedOutput& to, const TString& nam, const TString& recordSigField = TString()) : To(to), Namespace(nam), RecordSigField(recordSigField), TypeName("typename"), ExportOtherOverload(true)
    {
        if (Namespace.size() > 0)
            To << "namespace " << Namespace << "{\n";
        if (RecordSigField.size() > 0)
            To << "template<ui32 " << RecordSigField << "> struct TStructInfoBy" << RecordSigField << " {};\n";
        To << "template<typename TStruct> struct TStructInfo {\n";
        To << "    const static ui32 InAccessible = (1<<30); //this struct was not exported\n";
        To << "};\n";
    }
    TVector<TString> Structs;
    TString EncodeName(const TStructInfo::TMember& m)
    {
        TString n = m.Name;
        bool op = false;
        Y_VERIFY(n.size() > 0 && n[0] != '<', "shoud not be called");
        if (n[0] == '!')
        {
            Y_VERIFY(n.size() < 20, "wrong size for operator");
            char tmp[32];
            for (size_t i = 0; i < n.size(); ++i)
                tmp[i] = 'a' + (n[i]%20);
            tmp[n.size()] = 0;
            n.assign(tmp);
            op = true;
        }
        if (m.Static)
            return Sprintf("TA%c%c%dx%d_%s", op ? '_' : 'c', m.Const ? 'c':'_', (int)m.BitField, (int)m.TemplParams, n.data());
        else
            return Sprintf("TSAc%c%dx%d_%s", op ? '_' : 'c', (int)m.BitField, (int)m.TemplParams, n.data());

    }
    void PrintConstPtrMod(const TFullType& f)
    {
        Y_VERIFY(f.PointersCount < 60, "too much for pointers");
        for (ui32 i = 0; i < f.PointersCount; ++i)
            To << (f.Consts.Get(i)? " const":"") << "* ";
    }
//     void PrintConstRefMod(const TFullType& f)
//     {
//         To << (f.LastConst()? " const":"") << (f.IsReference?" &" : "");
//     }
    static ui64 EncodeNameui64(const TString& n)
    {
       ui64 ret = 0;
        for (size_t i = 0; i < n.size(); ++i)
            ret |= (((ui64)n[i])<<(i*8));
        return ret;
    }
    void Export(const TString& n, const TStructInfo& si, bool vis, TVector<TStructInfo::TMember>& members)
    {
        TString erres = si.ErrReason(false, true, true);

        Structs.push_back(n);
        //             if (+erres == 0)
        //                 continue;
        //Printf(To, "%s: %s {", ~n,  ~erres);

        To << "template<> struct TStructInfo<" << TypeName << " " <<  n << "> {\n";

        ui32 types = 0;
        ui32 baseTypes = 0;
        ui32 templTypes = 0;
        ui32 staticFuncNoParam = 0;
        ui32 staticFuncTemplParam = 0;
        ui32 staticFuncOther = 0;
        ui32 staticConst = 0;
        ui32 funcNoParam = 0;
        ui32 funcNoParamConst = 0;
        ui32 funcTemplParam = 0;
        ui32 funcOther = 0;
        ui32 funcOtherConst = 0;
        ui32 cConst = 0;
        ui32 ordinal = 0;
        ui32 bitField = 0;
        ui32 constBitfield = 0;

        OverFuncs.clear();
//        To << "    struct TNoAcc { /*TODO: to be done*/ };\n";
        for (TVector<TStructInfo::TMember>::const_iterator i = members.begin(); i != members.end(); ++i)
        {
            const TStructInfo::TMember& m = *i;
            if (!m.IsType && !m.Func && !m.Static)
            {
                To << "    struct TAcc_" << m.Name << "{\n";
                To << "        const static ui64 Name0 = " << EncodeNameui64(m.Name) << ";\n";
                To << "        template<class TTo>    static TTo Get(const " << TypeName << " " << n << "& f) { return TTo(f." << m.Name << ");}\n";
                if (!m.Const)
                    To << "        template<class TFrom>  static void Set(" << TypeName << " " << n << "& f, const TFrom& from) { f." << m.Name << " = from;}\n";
                To << "    };\n";
            }
//             else if (m.Func && !m.Static && m.BitField == 0 && m.Const && m.TemplParams == 0)
//             {
//                 To << "    struct TAcc_" << m.Name << "{\n";
//                 To << "        template<class TTo>   static TTo Call(const " << TypeName << " " << n << "& f) { return TTo(f." << m.Name << "());}\n";
//                 To << "    };\n";
//             }
            else if (m.Func && m.Name[0] == '<')
            {
                //conversation operator
            }
            else if (m.Func && !m.Static && m.TemplParams == 0)
            {
                TString aname = EncodeName(m);
                TString name = m.Name;
                if (name[0] == '!')
                    name = "operator " + name.substr(1);

                if (OverFuncs.insert(aname).second)
                {
                    To << "    struct " << aname << "{\n";
                    To << "        const static ui64 Name0 = " << EncodeNameui64(m.Name) << ";\n";
                    To << "        template<class TTo";
                    for (ui32 k = 0; k < m.BitField; ++k)
                        To << ", typename T_" << k << "_T";
                    To << ">   static TTo Call(" << (m.Const ? "const" : "") << TypeName << " " << n << "& f";
                    for (ui32 k = 0; k < m.BitField; ++k)
                        To << ", T_" << k << "_T v" << k;
                    To << ") { return TTo(f." << name << "(";
                        for (ui32 k = 0; k < m.BitField; ++k)
                            To << (k > 0 ? ", " : "") << "v" << k;
                    To << "));}\n";
                    To << "    };\n";
                }
            }
            else if (m.Func && m.Static && m.TemplParams == 0)
            {
                TString aname = EncodeName(m);
                TString name = m.Name;
                if (name[0] == '!')
                    name = "operator " + name.substr(1);

                if (OverFuncs.insert(aname).second)
                {
                    To << "    struct " << aname << "{\n";
                    To << "        const static ui64 Name0 = " << EncodeNameui64(m.Name) << ";\n";
                    To << "        template<class TTo";
                    for (ui32 k = 0; k < m.BitField; ++k)
                        To << ", typename T_" << k << "_T";
                    To << ">   static TTo Call(";
                    for (ui32 k = 0; k < m.BitField; ++k)
                        To << (k > 0 ? ", " : "") <<"T_" << k << "_T v" << k;
                    To << ") { return TTo(" << n << "::" << name << "(";
                    for (ui32 k = 0; k < m.BitField; ++k)
                        To << (k > 0 ? ", " : "") << "v" << k;
                    To << "));}\n";
                    To << "    };\n";
                }
            }
        }

        To << "    template<class TProc> static void RegisterStructMembers("<< TypeName << " " << n << "* f, TProc& proc) {\n";
        To << "        (void)f, (void)proc;\n";
        BaseNames.clear();
        si.ExportBaseNames(BaseNames, false); //TODO TEMPLATE PARAM NOW CAN'T BE EXPORTED TOO HARD TASK!

        for (TVector<TString>::iterator i = BaseNames.begin(); i != BaseNames.end(); ++i)
        {
            SubstGlobal(*i, ".","::");
            if (i == BaseNames.begin())
                Y_VERIFY(*i == n, "wrong naming");
            else
                (To << "        proc.template RegisterBaseType< " << *i << ">(\"" << *i << "\");\n"), baseTypes ++;
        }

        for (TVector<TStructInfo::TMember>::const_iterator i = members.begin(); i != members.end(); ++i)
        {
            const TStructInfo::TMember& m = *i;
            if (m.IsType)
                if (m.TemplParams == 0)
                    (To << "        proc.template RegisterType< " << n <<"::" << m.Name << ">(\"" << m.Name << "\");\n"), types ++;
                else
                    (To << "        //template type " << n <<"::" << m.Name << "\n"), templTypes ++;
            else if (m.Static)
            {
//                 if (m.Func && m.Name[0] == '<')
//                     To << "        //conversation operator \n";
//                 else
                if (m.Func)
                {
                    Y_VERIFY(m.Name[0] != '<', "static conversation operator???");
                    TString aname = EncodeName(m);
                    TString name = m.Name;
                    if (name[0] == '!')
                        name = "operator " + name.substr(1);
                    if (m.TemplParams != 0)
                        (To << "        //template func " << n <<"::" << name << "\n"), staticFuncTemplParam ++;
                    else if (m.BitField == 0)
                        (To << "        proc.template RegisterStaticFunc<" << aname  << ">(\"" << name << "\", &" << n << "::" << name << ");\n"), staticFuncNoParam++;
                    else if (ExportOtherOverload || ! m.Overloaded)
                        (To << "        proc.template RegisterStaticFuncOther<" << (int)m.BitField  << ", " << aname  << ">(\"" << name << "\", &" << n <<"::" << name << ");\n"), staticFuncOther++;
                    else
                        (To << "        //overloaded func " << n <<"::" << name << " with " << m.BitField << " params \n");
                }
                else
                    if (m.Const)
                        (To << "        proc.template RegisterStaticConst< " << n << "::" << m.Name << ">(\"" << m.Name << "\");\n"),staticConst++;
            }
            else
            {
                if (m.Func && m.Name[0] == '<')
                {
                    if (!m.Const || m.ReturnTypePtr == nullptr || !m.ReturnTypePtr->TemplParam.empty())
                        To << "        //conversation operator (non const or unparsed return type or template return type)\n";
                    else
                    {
                        TString n = m.ReturnTypePtr->FullName;
                        SubstGlobal(n, ".", "::");
                        To << "        proc.template RegisterConvert<" << TypeName << " " << n;
                        PrintConstPtrMod(m.ReturnType);
                        To << ", " << (m.ReturnType.LastConst() ? "true, " : "false, ") << (m.ReturnType.IsReference ? "true>" : "false>");
                        To << "(\"" << n;
                        PrintConstPtrMod(m.ReturnType);
                        To << "\");\n";
                    }
                }
                else if (m.Func)
                {
                    TString aname = EncodeName(m);
                    TString name = m.Name;
                    if (name[0] == '!')
                        name = "operator " + name.substr(1);
                    if (m.TemplParams != 0)
                        (To << "        //template func " << n <<"::" << m.Name << "\n"), funcTemplParam ++;
                    else if (m.BitField == 0)
                            if (m.Const)
                                (To << "        proc.template RegisterFuncConst<" << aname  << ">(\"" << name << "\", &" << n << "::" << name << ");\n"), funcNoParamConst++;
                            else
                                (To << "        proc.template RegisterFunc<" << aname  << ">(\"" << name << "\", &" << n << "::" << name << ");\n"), funcNoParam++;
                    else if (ExportOtherOverload || ! m.Overloaded)
                         if (m.Const)
                             (To << "        proc.template RegisterFuncOtherConst<" << (int)m.BitField  << ", " << aname  << ">(\"" << name << "\", &" << n <<"::" << name << ");\n"), funcOtherConst++;
                         else
                             (To << "        proc.template RegisterFuncOther<" << (int)m.BitField  << ", " << aname  << ">(\"" << name << "\", &" << n <<"::" << name << ");\n"), funcOther++;
                    else
                        if (m.Const)
                            (To << "        //overloaded const member func " << n << "::" << name << " with " << m.BitField << " params \n");
                        else
                            (To << "        //overloaded member func " << n << "::" << name << " with " << m.BitField << " params \n");
                }
                else if (m.BitField == 0)
                    if (m.Const)
                        (To << "        proc.template RegisterConst<TAcc_" << m.Name << ">(\"" << m.Name << "\", f->" << m.Name << ");\n"),cConst++;
                    else
                        (To << "        proc.template RegisterOrdinal<TAcc_" << m.Name << ">(\"" << m.Name << "\", f->" << m.Name << ");\n"),ordinal++;
                else
                    if (m.Const)
                        (To << "        proc.template RegisterConstBitfield<" << (int)m.BitField << ", TAcc_" << m.Name << ">(\"" << m.Name << "\", (f == 0) ? Min<i64>() : f->" << m.Name << ");\n"),constBitfield++;
                    else
                        (To << "        proc.template RegisterBitfield<" << (int)m.BitField << ", TAcc_" << m.Name << ">(\"" << m.Name << "\", (f == 0) ? Min<i64>() : f->" << m.Name << ");\n"),bitField++;
            }
        }
        To << "    }\n";
        if (types == 0)
            To << "    typedef TNone TAllTypes;\n";
        else
        {
            To << "    typedef TTypeList<";
            bool tfirst = true;
            for (TVector<TStructInfo::TMember>::const_iterator i = members.begin(); i != members.end(); ++i)
                if (i->IsType && i->TemplParams == 0)
                    if (tfirst)
                        (To <<" " << n << "::" << i->Name), tfirst = false;
                    else
                        To << "," << n << "::" << i->Name;
            To << "> TAllTypes;\n";
        }

        if (baseTypes == 0)
            To << "    typedef TNone TBaseTypes;\n";
        else
        {
            To << "    typedef TTypeList<";
            bool tfirst = true;
            for (TVector<TString>::iterator i = (BaseNames.begin() + 1); i != BaseNames.end(); ++i)
                if (tfirst)
                    (To << TypeName << " " << *i), tfirst = false;
                else
                    To << "," << TypeName << " " << *i;
            To << "> TBaseTypes;\n";
        }
        //todo compactify all of them
        To << "    const static ui32 Types = " << types <<";\n";
        To << "    const static ui32 BaseTypes = " << baseTypes <<";\n";
        To << "    const static ui32 StaticFuncsNoParam = " << staticFuncNoParam <<";\n";
        To << "    const static ui32 StaticFuncsOther = " << staticFuncOther <<";\n";
        To << "    const static ui32 StaticConsts = " << staticConst <<";\n";
        To << "    const static ui32 FuncsNoParam = " << funcNoParam <<";\n";
        To << "    const static ui32 FuncsNoParamConst = " << funcNoParamConst <<";\n";
        To << "    const static ui32 FuncsOther = " << funcOther <<";\n";
        To << "    const static ui32 FuncsOtherConst = " << funcOther <<";\n";

        To << "    const static ui32 Consts = " << cConst <<";\n";
        To << "    const static ui32 OrdinalFields = " << ordinal <<";\n";
        To << "    const static ui32 BitFields = " << bitField <<";\n";
        To << "    const static ui32 ConstBitFields = " << constBitfield <<";\n";

        To << "    const static ui32 ReadNonFuncsMembers = " << (ordinal + bitField + cConst + constBitfield) <<";\n";

        if (erres.size() > 0)
            To << "    const static ui32 InAccessible = 100; //!!! has parsing Errors " << erres << "\n";
        else if (si.HasUsingInBase())
            To << "    const static ui32 InAccessible = 10; //has using and so order is not standart\n";
        else if (si.HasManyInheritance())
            To << "    const static ui32 InAccessible = 5; //has many inheritance and if template base there order is not standart\n";
        else if (!vis)
            To << "    const static ui32 InAccessible = 2; //has unaccessible (for write) member(s)\n";
        else if (!si.DefCtor())
            To << "    const static ui32 InAccessible = 1; //no default ctor\n";
        else
            To << "    const static ui32 InAccessible = 0;\n";

        To << "    static const char* TypeName() { return \"" << n << "\";}\n";
        To << "    typedef " << TypeName << " " << n << " TAboutType;\n";
        To << "};\n";
        if (RecordSigField.size() > 0 && si.HasRecordSig)
        {
            To << "template<> struct TStructInfoBy" << RecordSigField << "<" << " " << n << "::" << RecordSigField << "> { typedef TStructInfo<" << " " << n << "> StructInfo; };\n";
            To << "// The previous declaration only works when structures' fields named `" << RecordSigField << "' are unique.\n";
            To << "// If this gives a compilation error, it is probably your fault, person who gave the same values to the supposedly unique fields of different structures.\n";
        }
        To << "\n";
    }
    void End(size_t prntStructs)
    {
        To << "struct TRegAll {\n";
        To << "     template<class TProc> static void RegisterStructs(TProc& proc) {\n";
        for (TVector<TString>::const_iterator i = Structs.begin(); i != Structs.end(); ++i)
            To << "        proc.template RegisterStruct<" << TypeName << " " <<  *i  << ", TStructInfo<" << TypeName << " " <<  *i  << "> >(\"" << *i << "\");\n";
        To << "     } // " << prntStructs << " structs\n";
        To << "};\n";
        if (Namespace.size() > 0)
            To << "}";
    }
};

template<class TO>
void PrintOnlyExport(TO& to, TStructParser& parser, const TStructInfo::TMember* smember, const TString& fromnamespace)
{
    TVector<TStructInfo::TMember> members;
    size_t prntStructs = 0;
    for(TMap<TString, TStructInfo>::const_iterator i = parser.ParsedStructs.begin(); i != parser.ParsedStructs.end(); ++i)
        if (i->second.Exportable())
        {
            members.clear();
            const TStructInfo& si = i->second;
            if (si.NoExport)
                continue;
            bool vis = si.GetPublicMembers(members);

            bool allow = false;
            if (smember == nullptr)
                allow = true;
            else
            {
                TVector<TStructInfo::TMember>::const_iterator j = ::LowerBound(members.begin(), members.end(), *smember);
                for (; j != members.end() && j->Name == smember->Name; ++j)
                    if (*j == *smember)
                        if (smember->LastStruct == nullptr || j->LastStruct == &si)
                            allow = true;
            }


            if (!allow)
                continue;

            TString n = i->first;
            SubstGlobal(n, ".","::");

            if (fromnamespace.size() > 0)
            {
                TStringBuf t(n);
                size_t p = t.rfind(':');
                Y_VERIFY(p != TStringBuf::npos, "should have ::");
                size_t s = 0;
                if (!fromnamespace.StartsWith("::"))
                    s = 2;
                if (fromnamespace.EndsWith("::"))
                {
                    if (t.substr(s, p + 1 - s) != TStringBuf(fromnamespace))
                        continue;
                }
                else
                {
                    --p;
                    Y_VERIFY(t[p] == ':', "should have ::");
                    if (t.substr(s, p - s) != TStringBuf(fromnamespace))
                        continue;
                }
            }

            ::Sort(members.begin(), members.end(), TStructInfo::TMemberByPos());

            to.Export(n, si, vis, members);

            ++prntStructs;
        }
    to.End(prntStructs);
}


//find . -name '*.h' | xargs -I Z echo '#include "'Z'"'
int main(int argc, char **argv) {
    //TVector<TString> wanted_structures;
    //TVector<TString> wanted_enums_v;
    //bool dump_all_enums = false;

    TString selfield;
    TString outFile;
    TString expnamespace;
    TString fromnamespace;
    TString recordSigField;
    bool printOnly = false;
    bool notypename = false;
    bool noexportOtherOverload = false;
    bool finalSelectField = false;

using namespace NLastGetopt;
TOpts opts;
opts.AddLongOption('f', "selected_field", "filter by selected field").StoreResult(&selfield).Optional().RequiredArgument("field"); //.DefaultValue("")
opts.AddLongOption("final_only", "filter by selected field but in final struct only").StoreValue(&finalSelectField, true).Optional().NoArgument();
opts.AddLongOption('u', "unique_id", "field that identifies structures (static ui32)").StoreResult(&recordSigField).Optional().RequiredArgument("record_sig");
opts.AddLongOption('o', "output_file", "output generated file").StoreResult(&outFile).Optional().RequiredArgument("file"); //.DefaultValue("")
opts.AddLongOption('n', "to_namespace", "namespace for all generated info").StoreResult(&expnamespace).Optional().RequiredArgument("name"); //.DefaultValue("")
opts.AddLongOption('m', "from_namespace", "namespace from which structures should be extracted").StoreResult(&fromnamespace).Optional().RequiredArgument("name"); //.DefaultValue("")
opts.AddLongOption('p', "printonly", "output generated file format").StoreValue(&printOnly, true).Optional().NoArgument(); //.DefaultValue("")
opts.AddLongOption("gcc44_no_typename", "disable generation explicit typename due to compiler bug < gcc45").StoreValue(&notypename, true).Optional().NoArgument(); //.DefaultValue("")
opts.AddLongOption("no_complex_overloaded_func_export", "disable export of complex overloaded func").StoreValue(&noexportOtherOverload, true).Optional().NoArgument(); //.DefaultValue("")
TOptsParseResult optres(&opts, argc, argv);
const TVector<TString>& freeArgs = optres.GetFreeArgs();




    TStructParser parser;
    parser.RecordSigField = recordSigField;
    TStructInfo::TMember smember;
    if (selfield.size() > 0)
    {
        TString buf;
        buf += "struct sel {" + selfield + "; };";
        parser.Parse(buf.begin(), buf.end());
        Y_VERIFY(parser.ParsedStructs.size() == 1, "something wrong with parser");
        TStructInfo& s = parser.ParsedStructs.begin()->second;
        Y_VERIFY(s.Members.size() == 1, "field not found");
        smember = s.Members.front();
        if (!finalSelectField)
            smember.LastStruct = nullptr;
    }
    parser.ParsedStructs.clear();

    for (int arg = 0; arg < freeArgs.ysize(); arg++) {
        TFILEPtr f(freeArgs[arg], "rb");
        TVector<char> buf((size_t)f.length());
        if (f.read(buf.begin(), 1, buf.size()) != buf.size())
            throw yexception() << "short read in input file";
        f.close();

        const char *er = parser.Parse(buf.begin(), buf.end());
        Y_VERIFY(er == nullptr, "!!!!!!!failed on %c %d %d line %d\n", *er, *er, '\n', parser.Line);
        Y_VERIFY(parser.Braces.empty(), "!!!!! failed non closed brace found");
        Y_VERIFY(parser.NameSpaceLevels.empty() && parser.CurParentType.empty(), "!!!!! failed non closed namespace found");
    }
    printf("END: collected structs %d\n", (int)parser.ParsedStructs.size());

    THolder<TUnbufferedFileOutput> fout_temp;
    THolder<TBufferedOutput> fout;
    if (outFile.size() > 0)
    {
        fout_temp.Reset(new TUnbufferedFileOutput(outFile));
        fout.Reset(new TBufferedOutput(fout_temp.Get()));
        fout->SetPropagateMode(true);
    }
    else
    {
        fout.Reset(new TBufferedOutput(&Cout));
        fout->SetPropagateMode(true);
        fout->SetFinishPropagateMode(false);
    }

    if (printOnly)
    {
        TPrnExport to(*fout, expnamespace);
        PrintOnlyExport(to, parser, (selfield > nullptr) ?  &smember : nullptr, fromnamespace);
    }
    else
    {
        TDefExport to(*fout, expnamespace, recordSigField);
        if (notypename)
            to.TypeName = "";
        if (noexportOtherOverload)
            to.ExportOtherOverload = false;
        PrintOnlyExport(to, parser, (selfield > nullptr) ?  &smember : nullptr, fromnamespace);
    }

    fout->Flush();
    fout->Finish();
    return 0;
}
