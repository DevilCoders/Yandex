#include "vec4.h"

#include <util/generic/yexception.h>

struct TSSE2Validator {
    TSSE2Validator() {
        TVec4f v1(2.f);
        TVec4f v2(2.f);
        TVec4f v = v1 / v2;
        float data[4];
        v.Store(data);
        for (auto i : data)
            Check(i);
    }

    void Check(float x) {
        if (x < 0.999f || x > 1.001f)
            ythrow yexception() << "throw away your gcc " << x << " != 1";
    }
};

void ValidateSSE2() {
    static const TSSE2Validator SSE2Validator;
}
