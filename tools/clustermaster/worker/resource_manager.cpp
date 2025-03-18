#include "resource_manager.h"

#include "netaddrresolver.h"

#include <tools/clustermaster/communism/client/parser.h>

void TResourcesStruct::Parse(const TString& what, const TString& defaultHost) {
    TBase::clear();
    Solver.Reset(nullptr);

    TString string(what);

    if (string.find('@') == TString::npos) {
        string += '@';
        string += defaultHost;
        Auto = true;
    }

    try {
        NCommunism::ParseDefinition(string, *this, Solver, *Singleton<TNetworkAddressResolver>());

        if (TBase::empty() && !Auto)
            throw TResourcesIncorrect() << "No claims specified";
    } catch (const NCommunism::TParserError& e) {
        TBase::clear();
        throw TResourcesIncorrect() << e.what();
    } catch (...) {
        TBase::clear();
        throw;
    }
}

TAutoPtr<TResourcesDefinition> TResourcesStruct::CompoundDefinition() const {
    TAutoPtr<TResourcesDefinition> definition(new TResourcesDefinition);

    for (TBase::const_iterator i = TBase::begin(); i != TBase::end(); ++i)
        if (i->Name.empty())
            definition->AddClaim(i->Key, i->Val);
        else
            definition->AddSharedClaim(i->Key, i->Val, i->Name);

    /*
     * Idea behind this was the following: if something went wrong and target statistics is incorrect we could prevent
     * clustermaster from running tons of tasks of some target by connecting to communism with commie and taking resource
     * that has name equal to name of target (TargetName). This situation never occurred - so I removed this.
     */
#if 0
    if (!TargetName.Empty())
        definition->AddClaim(TargetName, TargetNameClaim);
#endif

    return definition;
}
