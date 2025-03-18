#pragma once

#ifndef _MSC_VER

#include <paths.h>

#endif

#include "config_lexer.h"
#include "log.h"
#include "messages.h"
#include "parsing_helpers.h"
#include "target_graph.h"
#include "target_graph_parser.h"

#include <library/cpp/deprecated/fgood/fgood.h>

#include <util/datetime/cputimer.h>
#include <util/folder/dirut.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/string/util.h>

template <typename PTypes>
TTargetGraphBase<PTypes>::TTargetGraphBase(typename PTypes::TListManager *l, bool forTest)
    : ListManager(l)
    , ForTest(forTest)
{
}

template <typename PTypes>
TTargetGraphBase<PTypes>::~TTargetGraphBase() {
}

template <typename PTypes>
TString TTargetGraphBase<PTypes>::DumpGraphviz() const {
    TString str;
    TStringOutput out(str);

    out << "digraph Targets {\n";
    out << "    graph [ nodesep=0.05 ranksep=0.15 mindist=0.0 ]\n";
    out << "    node [ fontsize=10 height=0 ]\n";
    out << "    edge [ minlen=2 ]\n";

    // Nodes
    for (typename TTargetsList::const_iterator i = Targets.begin(); i != Targets.end(); ++i) {
        out << "    \"" << (*i)->Name << "\" [ id=\"" << (*i)->Name << "\" ";
        typename PTypes::TTargetType::TOptionsMap::const_iterator shape = (*i)->Type->Options.find("shape");
        if (shape == (*i)->Type->Options.end())
            out << "shape = ellipse";
        else
            out << "shape = " << shape->second;
        out << "];\n";
    }
    // Edges
    for (typename TTargetsList::const_iterator i = Targets.begin(); i != Targets.end(); ++i) {
        for (typename PTypes::TTarget::TDependsList::const_iterator j = (*i)->Followers.begin(); j != (*i)->Followers.end(); ++j) {
            out << "    \"" << (*i)->Name << "\" -> \"" << j->GetTarget()->Name << "\"";

            TString style;
            if (j->IsCrossnode())
                style += " penwidth=3";
            if (!j->GetCondition().IsEmpty())
                style += " style=\"dashed\"";
            else if (j->GetFlags() & DF_NON_RECURSIVE)
                style += " style=\"dotted\"";

            if (style.empty())
                out << ";\n";
            else
                out << " [" << style << " ];\n";
        }
    }
    out << "}\n";

    return str;
}


template <typename PTypes>
struct TTargetGraphParserState {
    typedef typename PTypes::TGraph TOuter;
    typename TOuter::TTargetTypesList Types;
    typename TOuter::TTargetsList Targets;
    typename TOuter::TLostTargetsMMap LostTargets;
    typename PTypes::TTarget* LastTarget;

    typename TOuter::TVariablesMap DefaultVariables;
    typename TOuter::TVariablesMap StrongVariables;

    const TConfigMessage* Message;

    TTargetGraphParserState(const TConfigMessage* msg)
        : LastTarget(nullptr)
        , Message(msg)
    {
    }
};



template <typename PTypes>
void TTargetGraphBase<PTypes>::MergeNewGraph(TParserState &state) {
    // TODO: really needed only on master
    // Merge states for targets with the same names
    for (typename TTargetsList::const_iterator target = state.Targets.begin(); target != state.Targets.end(); ++target) {
        bool notLoad = false;
        typename TTargetsList::const_iterator i = Targets.find((*target)->Name);
        if (i != Targets.end() && (*target)->IsSame(*i)) {
            (*target)->SwapState(*i);
        } else {
            try {
                (*target)->LoadState();
            } catch (const yexception& e) {
                LOG("Could not load target state (" << e.what() << "), resetting to idle");
                notLoad = true;
            }
            /* sync state in all stores */
            (*target)->SaveState();
        }

        if (!notLoad) {
            std::pair<typename TLostTargetsMMap::const_iterator, typename TLostTargetsMMap::const_iterator> targets = LostTargets.equal_range((*target)->Name);
            typename PTypes::TTarget *t = (*targets.first).second.first;
            if (targets.first != LostTargets.end() && (*target)->IsSame(t)) {
                for (typename TLostTargetsMMap::const_iterator i = targets.first; i != targets.second; ++i) {
                    std::pair<typename PTypes::TTarget*, int> lostTarget(*target, (*i).second.second);
                    state.LostTargets.insert(TLostTargetItem((*target)->Name, lostTarget));
                }
            }
        }
    }

    DoSwap(Types, state.Types);
    DoSwap(Targets, state.Targets);
    DoSwap(LostTargets, state.LostTargets);

    DoSwap(DefaultVariables, state.DefaultVariables);
    DoSwap(StrongVariables, state.StrongVariables);
}

template <typename PTypes>
void TTargetGraphBase<PTypes>::AddLostTask(typename PTypes::TTarget *target, int nTask) {
    std::pair<typename PTypes::TTarget*, int> lostTarget(target, nTask);
    LostTargets.insert(TLostTargetItem(target->Name, lostTarget));
}

template <typename PTypes>
void TTargetGraphBase<PTypes>::CommitTarget(const TTargetParsed& parsed, TParserState &state,
        typename PTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer)
{
    typename TTargetTypesList::const_iterator type = state.Types.find(parsed.Type);

    if (type == state.Types.end())
        ythrow TWithBackTrace<yexception>() << "unknown target type '" << parsed.Name << "'";

    if (state.Targets.find(parsed.Name) != state.Targets.end())
        ythrow TWithBackTrace<yexception>() << "duplicate target '" << parsed.Name << "'";

    THolder<typename PTypes::TTarget> newtarget(new typename PTypes::TTarget(parsed.Name, *type, state));

    for (TVector<TTargetParsed::TDepend>::const_iterator dependParsed = parsed.Depends.begin();
            dependParsed != parsed.Depends.end(); ++dependParsed)
    {
        typename PTypes::TTarget *depend = nullptr;
        if (!dependParsed->Name.empty()) {
            typename TTargetsList::const_iterator idepend = state.Targets.find(dependParsed->Name);

            if (idepend == state.Targets.end())
                ythrow TWithBackTrace<yexception>() << "unknown dependency '" << dependParsed->Name << "' for target '" << parsed.Name << "'";

            depend = *idepend;
        } else {
            depend = state.LastTarget;
            if (depend == nullptr) {
                continue;
            }
        }

        TTargetParsed::TDepend dependParsedCopy = *dependParsed;
        newtarget->RegisterDependency(depend, dependParsedCopy, state, precomputedTaskIdsContainer);
    }

    for (TTargetParsed::TOptions::const_iterator option = parsed.Options.begin();
            option != parsed.Options.end(); ++option)
    {
        newtarget->AddOption(option->first, option->second);
    }


    state.Targets.push_back(newtarget.Get(), parsed.Name);
    state.LastTarget = newtarget.Get();
    Y_UNUSED(newtarget.Release());
}


template <typename PTypes>
void TTargetGraphBase<PTypes>::CommitTargetType(const TTargetTypeParsed& parsed, TParserState& state) {
    if (state.Types.find(parsed.Name) != state.Types.end())
        ythrow yexception() << "duplicate target type '" << parsed.Name << "'";

    THolder<typename PTypes::TTargetType> newtype(new typename PTypes::TTargetType(
            static_cast<typename PTypes::TGraph*>(this), state.Message, parsed.Name,
            parsed.Paramss, ListManager));

    for (TTargetTypeParsed::TOptions::const_iterator option = parsed.Options.begin();
            option != parsed.Options.end(); ++option)
    {
        newtype->AddOption(option->first, option->second);
    }

    state.Types.push_back(newtype.Release(), parsed.Name);
}

template <typename PTypes>
void TTargetGraphBase<PTypes>::CommitState(const TTargetGraphNodeParsed& node, TParserState &state,
        typename PTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer)
{
    if (node.Compatible<TVariableParsed>()) {
        const TVariableParsed& v = node.Cast<TVariableParsed>();
        if (!v.Strong) {
            state.DefaultVariables.insert(std::make_pair(v.Name, v.Value));
        } else {
            TString value = v.Value;
            if (value.StartsWith('!')) {
                value = value.substr(1, TString::npos);
                TVector<TString> items;
                ListManager->GetListForVariable(value, items);
                if (items.empty()) {
                    value = "";
                } else {
                    value = items[0];
                    for (unsigned int i = 1; i < items.size(); ++i) {
                        value += " ";
                        value += items[i];
                    }
                }
            }
            state.StrongVariables.insert(std::make_pair(v.Name, value));
        }
    } else if (node.Compatible<TTargetTypeParsed>()) {
        CommitTargetType(node.Cast<TTargetTypeParsed>(), state);
    } else {
        CommitTarget(node.Cast<TTargetParsed>(), state, precomputedTaskIdsContainer);
    }
}



template <typename PTypes>
void TTargetGraphBase<PTypes>::ParseConfig(const TConfigMessage* message) {
    TParserState state(message);

    // parse new graph from scratch
    TStringBuf buf(message->GetConfig().data(), message->GetConfig().size());

    TTargetGraphParsed parsed = TTargetGraphParsed::Parse(buf);

    typename PTypes::TPrecomputedTaskIdsContainer precomputedTaskIdsContainer;
    for (TVector<TTargetGraphNodeParsed>::iterator node = parsed.Nodes.begin();
            node != parsed.Nodes.end(); ++node)
    {
        CommitState(*node, state, &precomputedTaskIdsContainer);
    }

    state.LastTarget = nullptr;

    Shebang = parsed.Shebang;

    MergeNewGraph(state);
}

template <typename PTypes>
void TTargetGraphBase<PTypes>::DumpState(IOutputStream& out) const {
    TPrinter printer(out);
    DumpState(printer);
}

template <typename PTypes>
void TTargetGraphBase<PTypes>::DumpState(TPrinter& printer) const {
    printer.Println("Graph:");

    TPrinter l1 = printer.Next();

    l1.Println("Types:");

    for (typename TTargetTypesList::const_iterator type = Types.begin(); type != Types.end(); ++type) {
        TPrinter l2 = l1.Next();
        (*type)->DumpState(l2);
    }

    l1.Println("Targets:");

    for (typename TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        TPrinter l2 = l1.Next();
        (*target)->DumpState(l2);
    }

    ListManager->DumpState(l1);

    DumpStateExtra(l1);

    printer.Println("Graph.");
}

