#pragma once

#include "result.h"

namespace NHtmlDetect {
    typedef void(TInitFun)(int& cs);
    typedef void(TPushFun)(int& cs, const ui8* p, const ui8* pe, TDetectResult& r);

    struct TMachineDesc {
        TInitFun* InitFun;
        TPushFun* PushFun;
    };

    class TMachine {
    public:
        static const size_t NumMachines = 3;
        TMachine(size_t machineId); // 0 <= machineId < NumMachines

        void Push(const char* text, size_t len);

        int GetState() const {
            return cs;
        }
        void SetState(int s) {
            cs = s;
        }
        const TDetectResult& GetResult() const {
            return Result;
        }

    private:
        // machine context
        int cs;
        // output
        TDetectResult Result;
        TMachineDesc MDesc;
    };

}
