#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "embedding.h"

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

namespace NPyBind {
#if PY_MAJOR_VERSION >= 3
    class TDeleteRawMem {
    public:
        template <typename T>
        static inline void Destroy(T* t) noexcept {
            PyMem_RawFree(t);
        }
    };

    template <typename T>
    using TRawMemHolder = THolder<T, TDeleteRawMem>;

    static void SetProgramName(char* name) {
        TRawMemHolder<wchar_t> wideName(Py_DecodeLocale(name, nullptr));
        Y_ENSURE(wideName);
        Py_SetProgramName(wideName.Get());
    }
#else
    auto SetProgramName = Py_SetProgramName;
#endif

    TEmbedding::TEmbedding(char* argv0) {
        SetProgramName(argv0);
        Py_Initialize();
    }

    TEmbedding::~TEmbedding() {
        Py_Finalize();
    }
}
