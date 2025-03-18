#pragma once

#include <util/generic/string.h>
#include <util/generic/intrlist.h>

namespace NCssSanit {
    struct TGcNode : TIntrusiveListItem<TGcNode> {
        virtual ~TGcNode() {
        }
    };

    template <class T>
    struct TNodePtr: public TGcNode {
        T Value;
    };

    class TGc {
    public:
        ~TGc() {
            DoGc();
        }

        template <class T>
        TNodePtr<T>* New() {
            TNodePtr<T>* res = new TNodePtr<T>;
            PtrList.PushBack(res);
            return static_cast<TNodePtr<T>*>(res->Node());
        }

        void DoGc() noexcept {
            while (!PtrList.Empty()) {
                delete PtrList.PopBack();
            }
        }

    private:
        TIntrusiveList<TGcNode> PtrList;
    };
}
