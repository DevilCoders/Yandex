#pragma once

#include "id_for_string.h"
#include "master_list_manager.h"
#include "param_list_manager.h"
#include "printer.h"
#include "target_graph.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

// fake name for cluster when target is not clustered
static char const* const NO_CLUSTERS_NAME = "NO_CLUSTER";

namespace NTargetTypePrivate {

    TVector<TString> ExpandHosts(const TVector<TString>& hosts, const TMasterListManager* listManager);

}


template <typename PTypes>
class TTargetTypeBase {
    typedef typename PTypes::TGraph TGraph;
public:
    TGraph* const Graph;
private:
    const TString Name;

protected:
    const ui32 ParamCount;

public:

    const TString& GetName() const { return Name; }

    typedef THashMap<TString, TString> TOptionsMap;

    TOptionsMap Options;

    TTargetTypeBase(
            TGraph* graph,
            const TString& name,
            const TVector<TVector<TString> >& paramss)
        : Graph(graph)
        , Name(name)
        , ParamCount(paramss.size() - 1)
    { }

    virtual ~TTargetTypeBase() {}

public:
    void AddOption(const TString& key, const TString& value) {
        Options.insert(std::make_pair(key, value));
    }

    ui32 GetParamCount() const {
        return ParamCount;
    }

    virtual void CheckState() const {
    }

    void DumpState(TPrinter& out) const {
        out.Println("Target type " + Name + ":");
        TPrinter l1 = out.Next();
        DumpStateExtra(l1);
        out.Println("Target type " + Name + ".");
    }

    virtual void DumpStateExtra(TPrinter&) const {}
};
