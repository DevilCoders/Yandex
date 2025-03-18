#pragma once

#include "calc_module.h"
#include "map_helpers.h"

#include <util/system/condvar.h>

#ifdef FreeModule
#undef FreeModule
#endif

namespace NSimpleCalculationNet {
    class TInitData: private NNonCopyable::TNonCopyable {
    private:
        typedef TBidirectMultivalMap<TCalcModuleHolder, TCalcModuleHolder> TModulesDependencies;
        TCalcModuleSet Modules;
        TModulesDependencies Dependencies;

        TMutex Lock;
        TCondVar CondVar;

        std::pair<TCalcModuleHolder, bool> GetModule();
        void FreeModule(TCalcModuleHolder module);
        void MakeThreadWork();
        static void* DoMakeThreadWork(void* me);

    public:
        TInitData();
        ~TInitData();

        void AddModule(TCalcModuleHolder module, TCalcModuleSet dependencies = TCalcModuleSet());

        void Init(size_t numThreads = 1);
    };

}

class TSimpleCalculationNet {
private:
    typedef NSimpleCalculationNet::TInitData TInitData;
    typedef TMap<TString, TCalcModuleHolder> TModules;
    TModules Modules;

    TCalcModuleHolder GetModule(const TString& moduleName);

    explicit TSimpleCalculationNet(const TModules& modules);

    TCalcModuleHolder GetExistingModule(const TString& moduleName);

public:
    TSimpleCalculationNet();
    TSimpleCalculationNet(TVector<TCalcModuleHolder> moduleHolders);
    ~TSimpleCalculationNet();

    void Insert(const TString& name, TCalcModuleHolder module);
    void InsertActionsList(const TString& name, const TVector<TString>& points);

    void InsertActionsList(const TString& name, const TString& points);

    /* The following types of description are allowed:
     *  <module1>:<point1> - <module2>:<point2>
     *  <module1> <- <module2>
     *  <module1> <- <module2> : <pointPattern1> <pointPattern2> ... <pointPatternN>
     */
    void Connect(const TString& connectionDescription);

    void Connect(const TString& moduleAndPoint, IAccessPoint& point);

    void MultiConnect(const TString& connectionDescriptions);
    void Init(TInitData& initData);

    void Import(const TSimpleCalculationNet& calculationNet);
    TSimpleCalculationNet Export(const TVector<TString>& modules) const;
    TSimpleCalculationNet Export(const TString& modules) const;

    void PrintDotOutput(IOutputStream& output);
};

using TSimpleCalculationNetHolder = TAtomicSharedPtr<TSimpleCalculationNet>;
