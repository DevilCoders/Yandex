#include <kernel/text_machine/parts/core/parts_multi_tracker.h>
#include <kernel/text_machine/module/module.h>

#include <library/cpp/getopt/last_getopt.h>


int main(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetTitle("Print info about text machine units");

    TString domainName;
    opts.AddLongOption('d', "domain", "domain name")
        .Required()
        .RequiredArgument("NAME")
        .StoreResult(&domainName);

    TVector<TString> unitNames;
    opts.AddLongOption('u', "unit", "unit name")
        .Required()
        .RequiredArgument("NAME")
        .AppendTo(&unitNames);

    TVector<TString> methodNames;
    opts.AddLongOption('m', "method", "print info about specific method")
        .Optional()
        .RequiredArgument("NAME")
        .AppendTo(&methodNames);

    TVector<TString> templateArgNames;
    opts.AddLongOption('t', "template-arg", "print info about specific template arg")
        .Optional()
        .RequiredArgument("NAME")
        .AppendTo(&templateArgNames);

    opts.SetFreeArgsMin(0);
    opts.SetFreeArgsMax(0);

    NLastGetopt::TOptsParseResult optsResult{&opts, argc, argv};

    for (TStringBuf unitName : unitNames) {
        const ::NModule::IUnitInfo* info = ::NModule::TUnitRegistry::Instance().GetUnitInfo(domainName, unitName);
        if (!info) {
            Cout << "Unknown unit " << domainName << "/" << unitName << Endl;
            continue;
        }

        Cout << "---" << Endl;
        Cout << info->GetDomainName() << Endl;
        Cout << info->GetName() << Endl;
        Cout << info->GetCppName() << Endl;

        for (TStringBuf methodName : methodNames) {
            Cout << Endl;

            const ::NModule::TUnitMethodInfo* methodInfo = info->GetMethodInfo(methodName);
            if (!methodInfo) {
                Cout << "Unknown method " << domainName << "/" << unitName << "::" << methodName << Endl;
                continue;
            }
            Cout << methodInfo->GetName() << Endl;

            for (TStringBuf tArgName : templateArgNames) {
                Cout << Endl;

                const ::NModule::TMethodTemplateArgInfo* tArgInfo = methodInfo->GetTemplateArgInfo(tArgName);
                if (!tArgInfo) {
                    Cout << "Unknown template arg " << domainName << "/" << unitName << "::" << methodName << "<" << tArgName << ">" << Endl;
                    continue;
                }

                for (int value : tArgInfo->GetInstantiations()) {
                    Cout << tArgName << " = " << value
                        << " " << tArgInfo->GetDescr().GetValueLiteral(value)
                        << " " << tArgInfo->GetDescr().GetValueScopePrefix() << Endl;
                }
            }
        }

        Cout << Endl;
    }

    return 0;
}
