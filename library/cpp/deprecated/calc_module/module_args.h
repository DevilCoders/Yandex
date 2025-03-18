#pragma once

#include "copy_points.h"
#include "simple_module.h"

class TParseArgException: public yexception {
};

namespace NModuleArg {
    class IAccessPointCreator {
    public:
        virtual ~IAccessPointCreator() {
        }
        virtual TAccessPointHolder Create(TSimpleModule* module) const = 0;
    };

    typedef TAtomicSharedPtr<IAccessPointCreator> TAccessPointCreatorHolder;

}

class TModuleArg: public NModuleArg::TAccessPointCreatorHolder {
private:
    typedef NModuleArg::TAccessPointCreatorHolder TBase;

    template <class T>
    class TModuleArgData: public NModuleArg::IAccessPointCreator {
    private:
        typedef TSlaveCopyPoint<T> TPoint;

        const TString Name;
        const T Val;

        TModuleArgData(const TString& name, T val)
            : Name(name)
            , Val(val)
        {
        }

        TAccessPointHolder Create(TSimpleModule* module) const override {
            return new TPoint(module, Val, Name);
        }

    public:
        static NModuleArg::TAccessPointCreatorHolder Create(const TString& name, T val) {
            return new TModuleArgData(name, val);
        }
        static T GetValue(IAccessPoint& point) {
            TPoint* pointPtr = dynamic_cast<TPoint*>(&point);
            if (!pointPtr) {
                ythrow TParseArgException() << "Incompatible type of arg";
            }
            return pointPtr->GetValue();
        }
    };

public:
    template <class T>
    TModuleArg(const TString& name, T val)
        : TBase(TModuleArgData<T>::Create(name, val))
    {
    }

    template <class T>
    static T GetValue(IAccessPoint& point) {
        return TModuleArgData<T>::GetValue(point);
    }
};

class TModuleArgs {
private:
    class TArgsModule: public TSimpleModule {
    private:
        typedef TVector<TAccessPointHolder> TArgs;
        TArgs Args;

    public:
        TArgsModule()
            : TSimpleModule("TArgsModule")
        {
        }

        void AddArg(TModuleArg arg) {
            Args.push_back(arg->Create(this));
        }
    };

    typedef TAtomicSharedPtr<TArgsModule> TArgsModuleHolder;
    TArgsModuleHolder Args;
    TVector<ui64> LegacyArgs;

    void ForceArgsExistence() {
        if (!Args) {
            Args = new TArgsModule;
        }
    }
    IAccessPoint& DoGetPoint(const TString& name) const {
        if (!Args) {
            ythrow TWrongPointNameException() << "Empty args";
        }
        return Args->GetAccessPoint(name);
    }
    template <class T>
    T DoGetArg(const TString& name) const {
        return TModuleArg::GetValue<T>(DoGetPoint(name));
    }

public:
    TModuleArgs() {
    }
    explicit TModuleArgs(const TVector<ui64>& legacyArgs)
        : LegacyArgs(legacyArgs)
    {
    }
    explicit TModuleArgs(TModuleArg arg) {
        AddArg(arg);
    }
    template <class... R>
    TModuleArgs(TModuleArg arg1, R... args) {
        AddArg(arg1, args...);
    }

    template <class... R>
    void AddArg(TModuleArg arg1, TModuleArg arg2, R... r) {
        AddArg(arg1);
        AddArg(arg2, r...);
    }
    void AddArg(TModuleArg arg) {
        ForceArgsExistence();
        Args->AddArg(arg);
    }
    void AddLegacyArg(ui64 legacyArg) {
        LegacyArgs.push_back(legacyArg);
    }

    const TVector<ui64>& GetLegacyArgs() const {
        return LegacyArgs;
    }

    template <class T>
    T GetArg(const TString& name) const try {
        return DoGetArg<T>(name);
    } catch (const TWrongPointNameException& e) {
        ythrow TParseArgException() << "Failed to parse arg: " << e.what();
    }
    template <class T>
    T GetArg(const TString& name, T defaultValue) const try {
        return DoGetArg<T>(name);
    } catch (const TWrongPointNameException&) {
        return defaultValue;
    }

    template <bool strict>
    void Connect(const TString& name, IAccessPoint& point) const try {
        DoGetPoint(name).Connect(point);
    } catch (const TWrongPointNameException& e) {
        if (strict) {
            throw;
        }
    }
};
