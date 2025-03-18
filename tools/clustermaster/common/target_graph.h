#pragma once

#include "catalogus.h"
#include "messages.h"
#include "param_list_manager.h"
#include "resource_monitor_resources.h"
#include "target_graph_parser.h"

#include <library/cpp/any/any.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

template <typename PTypes>
struct TTargetGraphParserState;

template <typename PTypes>
class TTargetGraphBase: public TNonCopyable {
public:
    using TTarget = typename PTypes::TTarget;

    using TTargetTypesList = TCatalogus<typename PTypes::TTargetType>;
    using TTargetsList = TCatalogus<TTarget>;
    using TLostTargetsMMap = TMultiMap< TString, std::pair<TTarget*, int> >;
    using TLostTargetItem = std::pair< TString, std::pair<TTarget*, int> >;

    using TVariablesMap = THashMap<TString, TString>;
    using const_iterator = typename TTargetsList::const_iterator;

    using TParserState = TTargetGraphParserState<PTypes>;

protected:

    void CommitTarget(const TTargetParsed&, TParserState &state,
            typename PTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer);
    void CommitTargetType(const TTargetTypeParsed&, TParserState &state);
    void CommitState(const TTargetGraphNodeParsed&, TParserState &state,
            typename PTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer);

    virtual void MergeNewGraph(TParserState &state);

protected:
    TTargetTypesList Types;
    TTargetsList Targets;
    TLostTargetsMMap LostTargets;

    TVariablesMap DefaultVariables;
    TVariablesMap StrongVariables;

    TString Shebang;

    TParamListManager ParamListManager;

    typename PTypes::TListManager* const ListManager;

public:
    TTargetGraphBase(typename PTypes::TListManager* l, bool forTest);
    virtual ~TTargetGraphBase();

    void ParseConfig(const TConfigMessage* message);

    TString DumpGraphviz() const;

    bool HasTarget(const TString& name) const {
        return Targets.find(name) != Targets.end();
    }

    void AddLostTask(TTarget* target, int nTask);
public:
    const TTargetsList& GetTargets() const noexcept { return Targets; }

    TTarget& GetTargetByName(const TString& name) {
        typename TTargetsList::iterator it = Targets.find(name);
        Y_VERIFY(it != Targets.end(), "target not found by name %s", name.data());
        return **it;
    }

    const_iterator begin() const {
        return Targets.begin();
    }

    const_iterator end() const {
        return Targets.end();
    }

    const_iterator find(const TString& name) const {
        return Targets.find(name);
    }

    const TString& GetShebang() const noexcept { return Shebang; }

    const typename PTypes::TListManager* GetListManager() const noexcept { return ListManager; }

    TParamListManager& GetParamListManager() { return ParamListManager; }

    const TVariablesMap& GetDefaultVariables() const noexcept { return DefaultVariables; }
    const TVariablesMap& GetStrongVariables() const noexcept { return StrongVariables; }

    const typename PTypes::TTargetType& GetTargetTypeByName(const TString& name) const {
        typename TTargetTypesList::const_iterator type = Types.find(name);
        if (type == Types.end())
            ythrow TWithBackTrace<yexception>() << "target type not found by name " << name;
        return **type;
    }

    virtual void CheckState() const {
        for (typename TTargetTypesList::const_iterator type = Types.begin(); type != Types.end(); ++type) {
            (*type)->CheckState();
        }
        for (typename TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
            (*target)->CheckState();
        }
    }

    void DumpState(IOutputStream& dest = Cout) const;

    virtual void DumpState(TPrinter&) const;

    virtual void DumpStateExtra(TPrinter&) const {}

public:
    const bool ForTest;
};
