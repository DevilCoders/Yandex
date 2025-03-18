#pragma once

#include "file_checker.h"

#include <util/generic/map.h>

namespace NChecker {
    struct TFileCheckerOpts {
        TFileOptions Options;
        ui32 Timer;

        TFileCheckerOpts(TFileOptions opts = TFileOptions(), ui32 timer = 10000)
            : Options(opts)
            , Timer(timer)
        {
        }
    };

    template <typename TItemChecker>
    class TFileRegistry: public TAtomicRefCount<TFileRegistry<TItemChecker>>, public TMap<TString, TIntrusivePtr<TItemChecker>> {
    };

    template <typename TItemChecker, typename TItemCheckerOpts = TFileCheckerOpts>
    class TRegistryChecker: public TFileChecker<TFileRegistry<TItemChecker>> {
    public:
        typedef TIntrusivePtr<TItemChecker> TItemCheckerPtr;
        typedef TFileRegistry<TItemChecker> TCheckedRegistry;
        typedef TFileChecker<TCheckedRegistry> TParent;
        typedef typename TParent::TSharedObjectPtr TSharedObjectPtr;
        typedef TMap<TString, TItemCheckerOpts> TFileSet;

        TRegistryChecker() = default;

        TRegistryChecker(const TString& fname, ui64 timems = 10000)
            : TParent(fname, TFileOptions(), timems)
        {
        }

    protected:
        virtual void GetNewFileSet(const TFile& fh, TFileSet&) const = 0;

    private:
        TSharedObjectPtr MakeNewObject(
            const TFile& fh, const TFileStat&, TSharedObjectPtr oldobj, bool /*beforestart*/) const override {
            TFileSet fs;
            GetNewFileSet(fh, fs);

            TSharedObjectPtr newobj = new TCheckedRegistry;
            if (!oldobj) {
                for (typename TFileSet::const_iterator it = fs.begin(); it != fs.end(); ++it) {
                    newobj->insert(std::make_pair(it->first, new TItemChecker(it->first, it->second)));
                }
            } else {
                for (typename TFileSet::const_iterator it = fs.begin(); it != fs.end(); ++it) {
                    // do not reinitialize items if they were already initialized (check old fileset if there were any)
                    typename TCheckedRegistry::const_iterator rit = oldobj->find(it->first);
                    if (rit != oldobj->end()) {
                        newobj->insert(std::make_pair(it->first, rit->second));
                    } else {
                        newobj->insert(std::make_pair(it->first, new TItemChecker(it->first, it->second)));
                    }
                }
            }

            return newobj;
        }

        void OnSwitchState(TSharedObjectPtr ptr, TSharedObjectPtr) const override {
            if (!ptr)
                return;

            TCheckedRegistry& r = *ptr;
            for (typename TCheckedRegistry::iterator it = r.begin(); it != r.end(); ++it) {
                it->second->Start();
            }
        }
    };

}
