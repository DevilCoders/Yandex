#include "simple_calc_net.h"

#include "action_points.h"
#include "stream_points.h"

#include <util/system/thread.h>

namespace NSimpleCalculationNet {
    std::pair<TCalcModuleHolder, bool> TInitData::GetModule() {
        TCalcModuleHolder ret;
        for (const auto& module : Modules) {
            if (Dependencies.GetImage(module).empty()) {
                ret = module;
                break;
            }
        }
        if (!!ret)
            Modules.erase(ret);
        return std::pair<TCalcModuleHolder, bool>(ret, Modules.empty());
    }
    void TInitData::FreeModule(TCalcModuleHolder module) {
        TCalcModuleSet set;
        set.insert(module);
        Dependencies.RemoveSetFromCodomain(set);
        CondVar.BroadCast();
    }
    void TInitData::MakeThreadWork() {
        while (true) {
            TCalcModuleHolder module;
            {
                TGuard<TMutex> guard(Lock);
                std::pair<TCalcModuleHolder, bool> ret = GetModule();
                if (!ret.first) {
                    if (ret.second)
                        return;
                    CondVar.WaitT(Lock, TDuration::Seconds(1));
                    continue;
                }
                module = ret.first;
            }
            TMasterActionPoint initModule;
            module->Connect("init", initModule);
            initModule.DoAction();
            {
                TGuard<TMutex> guard(Lock);
                FreeModule(module);
            }
        }
    }
    void* TInitData::DoMakeThreadWork(void* me) {
        ((TInitData*)me)->MakeThreadWork();
        return nullptr;
    }

    TInitData::TInitData() {
    }
    TInitData::~TInitData() = default;

    void TInitData::AddModule(TCalcModuleHolder module, TCalcModuleSet dependencies) {
        TGuard<TMutex> guard(Lock);
        Modules.insert(module);
        Dependencies.AddPairs(module, dependencies);
    }

    void TInitData::Init(size_t numThreads) {
        if (!numThreads) {
            ythrow yexception() << "At least one thread should be used for initializing.\n";
        }
        // Removing dependencies to uninitable modules.
        TCalcModuleSet dependencies = Dependencies.GetCodomain();
        for (const auto& module : Modules)
            dependencies.erase(module);
        Dependencies.RemoveSetFromCodomain(dependencies);
        // Preforming init tasks according to deps.
        if (numThreads == 1) {
            MakeThreadWork();
        } else {
            TVector<TSimpleSharedPtr<TThread>> threads(numThreads);
            for (size_t i = 0; i < (size_t)numThreads; i++) {
                threads[i] = new TThread(DoMakeThreadWork, this);
                threads[i]->Start();
            }
            for (size_t i = 0; i < (size_t)numThreads; i++)
                threads[i]->Join();
        }
    }

}

TCalcModuleHolder TSimpleCalculationNet::GetModule(const TString& moduleName) {
    TModules::const_iterator it = Modules.find(moduleName);
    return (it == Modules.end()) ? nullptr : it->second;
}

TSimpleCalculationNet::TSimpleCalculationNet(const TModules& modules)
    : Modules(modules)
{
}

TCalcModuleHolder TSimpleCalculationNet::GetExistingModule(const TString& moduleName) {
    TCalcModuleHolder ret = GetModule(moduleName);
    if (!ret) {
        ythrow yexception() << "Trying to get absent module: " << moduleName << "\n";
    }
    return ret;
}

namespace {
    template <size_t tokensNum>
    TVector<TString> SplitStringToTokens(const TString& description, const char* delimiter, size_t maxFields = 0, int options = 0) {
        TVector<TString> ret = SplitString(description, delimiter, maxFields, options);
        if (ret.size() != tokensNum) {
            ythrow yexception() << "Wrong number of subtokens in description: " << description << "\n";
        }
        return ret;
    }
}

TSimpleCalculationNet::TSimpleCalculationNet() {
}
TSimpleCalculationNet::TSimpleCalculationNet(TVector<TCalcModuleHolder> moduleHolders) {
    TSet<TCalcModuleHolder, TSharedPtrLess<TCalcModuleHolder>> moduleHoldersSet;
    for (size_t i = 0; i < moduleHolders.size(); ++i) {
        TCalcModuleHolder& module = moduleHolders[i];
        if (moduleHoldersSet.insert(module).second) {
            Insert(module->GetName() + "_" + ToString<size_t>((size_t)module.Get()), module);
        }
    }
}

TSimpleCalculationNet::~TSimpleCalculationNet() = default;

void TSimpleCalculationNet::Insert(const TString& name, TCalcModuleHolder module) {
    if (Modules.find(name) != Modules.end()) {
        ythrow yexception() << "Duplicating module: " << name;
    }
    Modules[name] = module;
}
void TSimpleCalculationNet::InsertActionsList(const TString& name, const TVector<TString>& points) try {
    TMasterOutputPoint<IAccessPoint*> actions;
    Modules[name]->Connect("actionpoint_input", actions);
    for (size_t i = 0; i < points.size(); i++) {
        TVector<TString> moduleAndPoint = SplitStringToTokens</*tokensNum=*/2>(points[i], ":", 0, 1);
        TCalcModuleHolder module = GetExistingModule(moduleAndPoint[0]);
        actions.Write(&module->GetAccessPoint(moduleAndPoint[1]));
    }
} catch (const yexception& e) {
    ythrow yexception() << "Inserting to action list \"" << name << "\" has failed:\n"
                        << e.what();
}

void TSimpleCalculationNet::InsertActionsList(const TString& name, const TString& points) try {
    TVector<TString> pointsVec = SplitString(points, " ");
    InsertActionsList(name, pointsVec);
} catch (const yexception& e) {
    ythrow yexception() << "Inserting points \"" << points << "\" has failed:\n"
                        << e.what();
}
/* The following types of description are allowed:
 *  <module1>:<point1> - <module2>:<point2>
 *  <module1> <- <module2>
 *  <module1> <- <module2> : <pointPattern1> <pointPattern2> ... <pointPatternN>
 */
void TSimpleCalculationNet::Connect(const TString& connectionDescription) try {
    TVector<TString> tokens = SplitString(connectionDescription, " ");
    if (!(
            (tokens.size() == 3 && tokens[1] == "-") ||
            (tokens.size() == 3 && tokens[1] == "<-") ||
            (tokens.size() > 4 && tokens[1] == "<-" && tokens[3] == ":"))) {
        ythrow yexception() << "Doesn't match any of the following formats:\n"
                               "module1:point1 - module2:point2\n"
                               "module1 <- module2\n"
                               "module1 <- module2 : point1 <...> pointN\n";
    }
    if (tokens[1] == "-") {
        TVector<TString> moduleAndPoint1 = SplitStringToTokens</*tokensNum=*/2>(tokens[0], ":", 0, 1);
        TVector<TString> moduleAndPoint2 = SplitStringToTokens</*tokensNum=*/2>(tokens[2], ":", 0, 1);
        TCalcModuleHolder module1 = GetExistingModule(moduleAndPoint1[0]);
        TCalcModuleHolder module2 = GetExistingModule(moduleAndPoint2[0]);
        MagicConnect(module1, moduleAndPoint1[1], module2, moduleAndPoint2[1]);
    } else if (tokens[1] == "<-") {
        TCalcModuleHolder module1 = GetExistingModule(tokens[0]);
        TCalcModuleHolder module2 = GetExistingModule(tokens[2]);
        if (tokens.size() > 4) {
            for (size_t i = 4; i < tokens.size(); i++) {
                MagicConnect(module1, module2, tokens[i]);
            }
        } else {
            MagicConnect(module1, module2);
        }
    } else {
        Y_FAIL("Wrong connection description - \"%s\".\n", connectionDescription.data());
    }
} catch (const yexception& e) {
    ythrow yexception() << "Connection by description \"" << connectionDescription << "\" has failed:\n"
                        << e.what();
}

void TSimpleCalculationNet::Connect(const TString& moduleAndPoint, IAccessPoint& point) try {
    TVector<TString> moduleAndPointParsed = SplitStringToTokens</*tokensNum=*/2>(moduleAndPoint, ":", 0, 1);
    TCalcModuleHolder module = GetExistingModule(moduleAndPointParsed[0]);
    module->Connect(moduleAndPointParsed[1], point);
} catch (const yexception& e) {
    ythrow yexception() << "Connection to point by module:point description \"" << moduleAndPoint << "\" has failed:\n"
                        << e.what();
}

void TSimpleCalculationNet::MultiConnect(const TString& connectionDescriptions) {
    TVector<TString> descriptionsVec = SplitString(connectionDescriptions, "\n");
    for (size_t i = 0; i < descriptionsVec.size(); i++) {
        Connect(descriptionsVec[i]);
    }
}
void TSimpleCalculationNet::Init(TInitData& initData) {
    TModules initModules;
    for (TModules::const_iterator it = Modules.begin(); it != Modules.end(); it++) {
        const TString& name = it->first;
        const TCalcModuleHolder& module = it->second;
        try {
            module->CheckReady();
        } catch (yexception& e) {
            ythrow yexception() << "Module \"" << name << "\" is unready, because: " << e.what();
        }
        if (module->GetPointNames().contains("init")) {
            initModules[name] = module;
        }
    }
    // Find connection owners
    typedef TSet<TString> TDict;
    typedef TMap<IAccessPoint::TConnectionId, TCalcModuleSet> TConnectionId2Modules;
    TConnectionId2Modules connectionOwners;
    for (TModules::const_iterator it = initModules.begin(); it != initModules.end(); ++it) {
        const TCalcModuleHolder& module = it->second;
        const TDict& pointNames = module->GetPointNames();
        for (const auto& pointName : pointNames) {
            TSlaveAccessPoint* point = dynamic_cast<TSlaveAccessPoint*>(&module->GetAccessPoint(pointName));
            if (point) {
                if (IAccessPoint::TConnectionId connectionId = point->GetConnectionId()) {
                    connectionOwners[connectionId].insert(module);
                }
            }
        }
    }
    // Fill init dependencies
    for (TModules::const_iterator it = initModules.begin(); it != initModules.end(); ++it) {
        const TCalcModuleHolder& module = it->second;
        const TAccessPointInfo* info = module->GetAccessPointInfo("init");
        if (info) {
            TCalcModuleSet dependencies;
            const TDict& usedPoints = info->GetUsedPoints();
            for (const auto& usedPoint : usedPoints) {
                TMasterAccessPoint* point = dynamic_cast<TMasterAccessPoint*>(&module->GetAccessPoint(usedPoint));
                if (point) {
                    if (IAccessPoint::TConnectionId connectionId = point->GetConnectionId()) {
                        TConnectionId2Modules::const_iterator dit = connectionOwners.find(connectionId);
                        if (dit != connectionOwners.end()) {
                            const TCalcModuleSet& modules = dit->second;
                            dependencies.insert(modules.begin(), modules.end());
                        }
                    }
                }
            }
            initData.AddModule(module, dependencies);
        }
    }
}

void TSimpleCalculationNet::Import(const TSimpleCalculationNet& calculationNet) {
    Modules.insert(calculationNet.Modules.begin(), calculationNet.Modules.end());
}
TSimpleCalculationNet TSimpleCalculationNet::Export(const TVector<TString>& modules) const {
    TModules exportModules;
    for (size_t i = 0; i < modules.size(); i++) {
        const TString& moduleName = modules[i];
        TModules::const_iterator it = Modules.find(moduleName);
        if (it == Modules.end()) {
            ythrow yexception() << "Trying to export absent module: " << moduleName << "\n";
        }
        exportModules[moduleName] = it->second;
    }
    return TSimpleCalculationNet(exportModules);
}
TSimpleCalculationNet TSimpleCalculationNet::Export(const TString& modules) const {
    TVector<TString> modulesVec = SplitString(modules, " ");
    return Export(modulesVec);
}
void TSimpleCalculationNet::PrintDotOutput(IOutputStream& output) {
    // Header
    output << "digraph \"modules number: " << Modules.size() << "\" {\n"
           << "  graph [\n"
           << "    rankdir = \"LR\"\n"
           << "    ranksep = 7\n"
           << "    concentrate = true\n"
           << "  ];\n"
           << "  node [ shape = \"record\", fontsize=24 ];" << Endl;
    // Connection map
    typedef TSet<TString> TDict;
    typedef std::pair<TDict, TDict> TMaster2Slave;
    typedef TMap<TString, TDict> TPoint2Point;
    typedef TMap<IAccessPoint::TConnectionId, TMaster2Slave> TConnectionsInfo;
    TConnectionsInfo connectionsInfo;
    // Cluster & node & internal edge declarations:
    for (auto& Module : Modules) {
        const TString& moduleName = Module.first;
        ICalcModule& module = *Module.second;
        output << "  subgraph cluster_" << moduleName << " {\n"
                                                         "    label=\""
               << moduleName << "\\n(" << module.GetName() << ")\";\n"
                                                              "    color=grey;\n"
                                                              "    fontsize=40;"
               << Endl;
        TDict pointNames = module.GetPointNames();
        TPoint2Point internalDeps;
        for (const auto& pointName : pointNames) {
            IAccessPoint* point = &module.GetAccessPoint(pointName);
            TString nodeName = moduleName + "___" + pointName;
            TString color = "grey";
            if (dynamic_cast<TSlaveAccessPoint*>(point)) {
                color = "green";
                if (IAccessPoint::TConnectionId connectionId = point->GetConnectionId()) {
                    connectionsInfo[connectionId].second.insert(nodeName);
                }
            } else if (dynamic_cast<TMasterAccessPoint*>(point)) {
                color = "red";
                if (IAccessPoint::TConnectionId connectionId = point->GetConnectionId()) {
                    connectionsInfo[connectionId].first.insert(nodeName);
                }
            }
            output << "    " << nodeName << " [ label=\"" << pointName << "\", color=" << color << " ];" << Endl;
            const TAccessPointInfo* info = module.GetAccessPointInfo(pointName);
            if (info) {
                const TDict& usedPoints = info->GetUsedPoints();
                for (const auto& usedPoint : usedPoints) {
                    IAccessPoint* point2 = &module.GetAccessPoint(usedPoint);
                    if (dynamic_cast<TMasterAccessPoint*>(point2)) {
                        TString masterPointName = moduleName + "___" + usedPoint;
                        internalDeps[nodeName].insert(masterPointName);
                    }
                }
            }
        }
        for (TPoint2Point::const_iterator pit = internalDeps.begin(); pit != internalDeps.end(); ++pit) {
            const TString& pointName = pit->first;
            const TDict& masters = pit->second;
            for (const auto& masterName : masters) {
                if (masterName.EndsWith("input") || pointName.EndsWith("output")) {
                    output << "    " << masterName << " -> " << pointName << " [style=dashed, dir=\"back\"];" << Endl;
                } else {
                    output << "    " << pointName << " -> " << masterName << " [style=dashed];" << Endl;
                }
            }
        }
        output << "  }" << Endl;
    }
    // Edge declarations:
    for (TConnectionsInfo::const_iterator it = connectionsInfo.begin(); it != connectionsInfo.end(); ++it) {
        const TDict& masters = it->second.first;
        const TDict& slaves = it->second.second;
        for (const auto& masterName : masters) {
            for (const auto& slaveName : slaves) {
                if (masterName.EndsWith("input") || slaveName.EndsWith("output")) {
                    output << "  " << slaveName << " -> " << masterName << " [dir=\"back\"]" << Endl;
                } else {
                    output << "  " << masterName << " -> " << slaveName << Endl;
                }
            }
        }
    }
    // EOF
    output << "}" << Endl;
}
