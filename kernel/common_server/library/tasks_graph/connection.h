#pragma once

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/object_factory/object_factory.h>
#include "task.h"

namespace NRTYScript {

    class IConnectionChecker {
    public:
        typedef NObjectFactory::TObjectFactory<IConnectionChecker, TString> TFactory;
        typedef TAtomicSharedPtr<IConnectionChecker> TPtr;
    public:
        virtual ~IConnectionChecker() {};
        virtual TString GetClass() const = 0;
        virtual bool Check(const TTaskContainer& task) const = 0;
        virtual bool IsValuableForReady() const = 0;
        virtual NJson::TJsonValue Serialize() const = 0;
        virtual void Deserialize(const NJson::TJsonValue& info) = 0;
    };

    class TCheckerContainer {
    private:
        IConnectionChecker::TPtr Checker;
    public:

        TCheckerContainer() {}

        TCheckerContainer(IConnectionChecker::TPtr checker) {
            Checker = checker;
        }

        NJson::TJsonValue Serialize() const {
            NJson::TJsonValue result(NJson::JSON_MAP);
            result["class"] = Checker->GetClass();
            result["checker"] = Checker->Serialize();
            return result;
        }

        void Deserialize(const NJson::TJsonValue& info);

        const IConnectionChecker::TPtr GetChecker() const {
            return Checker;
        }

    };

    class TFinishedChecker: public IConnectionChecker {
    public:
        virtual bool Check(const TTaskContainer& task) const override {
            return task.IsFinished();
        }

        virtual TString GetClass() const override {
            return "finished_checker";
        }

        virtual NJson::TJsonValue Serialize() const override {
            NJson::TJsonValue result(NJson::JSON_MAP);
            return result;
        }

        virtual bool IsValuableForReady() const override {
            return true;
        }

        virtual void Deserialize(const NJson::TJsonValue& /*info*/) override {

        }
    };

    class TStatusChecker: public IConnectionChecker {
    private:
        bool Success;

    protected:
        TStatusChecker(bool success) {
            Success = success;
        }

    public:
        virtual bool Check(const TTaskContainer& task) const override {
            return task.IsFinished() && (Success == task.IsSuccess());
        }

        virtual bool IsValuableForReady() const override {
            return Success;
        }

        virtual NJson::TJsonValue Serialize() const override {
            NJson::TJsonValue result(NJson::JSON_MAP);
            result["success"] = Success;
            return result;
        }

        virtual void Deserialize(const NJson::TJsonValue& info) override {
            Success = info["success"].GetBooleanRobust();
        }
    };

    class TSucceededChecker: public TStatusChecker {
    public:
        TSucceededChecker()
            : TStatusChecker(true)
        {}

        virtual TString GetClass() const override {
            return "succeeded_checker";
        }
    };

    class TFailedChecker: public TStatusChecker {
    public:
        TFailedChecker()
            : TStatusChecker(false)
        {}

        virtual TString GetClass() const override {
            return "failed_checker";
        }
    };
}
