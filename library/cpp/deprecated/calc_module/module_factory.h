#pragma once

#include "calc_module.h"
#include "module_args.h"

class IModuleFactory {
public:
    virtual ~IModuleFactory() {
    }
    virtual TCalcModuleHolder BuildModule(TString name, TModuleArgs args = TModuleArgs()) const = 0;

    template <class... R>
    inline TCalcModuleHolder BuildModule(TString name, TModuleArg arg, R... args) {
        return BuildModule(name, TModuleArgs(arg, args...));
    }

    template <class... R>
    inline TCalcModuleHolder BuildModule(TString name, ui64 arg, R... r) {
        TModuleArgs args;
        AddLegacyArg(args, arg, r...);
        return BuildModule(name, args);
    }

private:
    void AddLegacyArg(TModuleArgs& args, ui64 arg) {
        args.AddLegacyArg(arg);
    }
    template <class... R>
    void AddLegacyArg(TModuleArgs& args, ui64 arg1, ui64 arg2, R... r) {
        args.AddLegacyArg(arg1);
        AddLegacyArg(args, arg2, r...);
    }
};
