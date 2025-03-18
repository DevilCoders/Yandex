#pragma once

#include "module_factory.h"

class TSimpleFactory: public IModuleFactory {
private:
    typedef TCalcModuleHolder (*TArgsModuleBuilder)(TModuleArgs);
    typedef TCalcModuleHolder (*TModuleBuilder)();
    typedef TMap<TString, TArgsModuleBuilder> TArgsModuleBuilders;
    typedef TMap<TString, TModuleBuilder> TModuleBuilders;
    TArgsModuleBuilders ArgsModuleBuilders;
    TModuleBuilders ModuleBuilders;

    void AddModuleBuilder(TString name, TArgsModuleBuilder moduleBuilder) {
        ArgsModuleBuilders[name] = moduleBuilder;
    }
    void AddModuleBuilder(TString name, TModuleBuilder moduleBuilder) {
        ModuleBuilders[name] = moduleBuilder;
    }

protected:
    template <class T>
    void AddModuleBuilder(TString name) {
        AddModuleBuilder(name, &T::BuildModule);
    }

public:
    TCalcModuleHolder BuildModule(TString name, TModuleArgs args = TModuleArgs()) const override {
        TCalcModuleHolder ret;
        // New flexible scheme
        if (!ret) {
            TArgsModuleBuilders::const_iterator it = ArgsModuleBuilders.find(name);
            if (it != ArgsModuleBuilders.end()) {
                ret = it->second(args);
            }
        }
        // No args
        if (!ret) {
            TModuleBuilders::const_iterator it = ModuleBuilders.find(name);
            if (it != ModuleBuilders.end()) {
                ret = it->second();
            }
        }
        if (!ret) {
            ythrow yexception() << "Wrong module name: " << name;
        }
        return ret;
    }
};
