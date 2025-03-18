#pragma once

#include "periodic_checker.h"

#include <util/generic/ptr.h>
#include <util/system/rwlock.h>
#include <util/system/file.h>
#include <util/system/fstat.h>

namespace NChecker {
    template <typename TObject>
    class IObjectHolder: public TAtomicRefCount<IObjectHolder<TObject>>, TNonCopyable {
    public:
        typedef TIntrusivePtr<TObject> TSharedObjectPtr;

        virtual TSharedObjectPtr GetObject() const = 0;
        virtual ~IObjectHolder() = default;
    };

    template <typename TObject>
    class TFakeObjectHolder: public IObjectHolder<TObject> {
        typedef IObjectHolder<TObject> TParent;

    public:
        using typename TParent::TSharedObjectPtr;

    private:
        TSharedObjectPtr Object;

    public:
        explicit TFakeObjectHolder(TSharedObjectPtr obj = TSharedObjectPtr())
            : Object(obj)
        {
        }

        void SetObject(TSharedObjectPtr obj) {
            Object = obj;
        }

        TSharedObjectPtr GetObject() const override {
            return Object;
        }
    };

    struct TFileOptions {
        bool ZeroOnDelete;

        TFileOptions()
            : ZeroOnDelete()
        {
        }

        TFileOptions operator|(const TFileOptions& other) const {
            return TFileOptions(*this) |= other;
        }

        TFileOptions& operator|=(const TFileOptions& other) {
            ZeroOnDelete |= other.ZeroOnDelete;
            return *this;
        }

        static TFileOptions GetZeroOnDelete() {
            TFileOptions res;
            res.ZeroOnDelete = true;
            return res;
        }
    };

    enum EUpdateStrategy {
        US_DO_NOT_READ = 0,
        US_DROP_AND_READ = 1,
        US_READ_AND_SWAP = 2,
    };

    template <typename TObject>
    class TFileChecker
       : public IObjectHolder<TObject>,
          public IUpdater {
    private:
        typedef IObjectHolder<TObject> TParent;

    public:
        typedef typename TParent::TSharedObjectPtr TSharedObjectPtr;

    private:
        struct TState : TAtomicRefCount<TState> {
            time_t Timestamp;
            TSharedObjectPtr Object;

            TState()
                : Timestamp()
            {
            }
        };

        mutable TIntrusivePtr<TState> State;
        mutable TIntrusivePtr<TState> PendingState;

        TRWMutex Mutex;
        const TString FName;

    protected:
        TFileOptions Options;

    private:
        TPeriodicChecker Checker;

    public:
        TFileChecker(const TString& fname, TFileOptions o = TFileOptions(), ui32 timerms = 10000);

        void Start() {
            Checker.Start();
        }

        void Stop() {
            Checker.Stop();
        }

        const TString& GetMonitorFileName() const;

        TFileOptions GetFileOptions() {
            return Options;
        }
        void SetFileOptions(TFileOptions o) {
            Options = o;
        }

        TSharedObjectPtr GetObject() const override;

    protected:
        virtual TSharedObjectPtr MakeNewObject(
            const TFile& /*monitor file*/,
            const TFileStat& stat /*monitor file stat*/,
            TSharedObjectPtr /*old*/,
            bool beforestart) const = 0;

        virtual void OnSwitchState(
            TSharedObjectPtr /*newobj*/,
            TSharedObjectPtr /*oldobj*/) const {
        } // called under write lock!

        virtual EUpdateStrategy GetUpdateStrategy(
            const TFile& /*monitor file*/,
            const TFileStat& /*monitor file stat*/,
            const TSharedObjectPtr& /*old*/) const {
            return US_READ_AND_SWAP;
        }

    private:
        bool IsChanged() const override;

        bool Update() override {
            return DoUpdate(false);
        }

        void OnBeforeStart() override {
            DoUpdate(true);
            SwitchState();
        }

        void SwitchState() const;

        bool DoUpdate(bool beforestart);
    };

}

#include "file_checker_impl.h"
