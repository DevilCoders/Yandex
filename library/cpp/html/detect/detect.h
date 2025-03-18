#pragma once

#include "result.h"
#include <library/cpp/html/face/event.h>
#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>

namespace NHtmlDetect {
    class TDetector : TNonCopyable {
    public:
        TDetector();
        ~TDetector();

        void Reset();
        void OnEvent(const THtmlChunk& e);
        const TDetectResult& GetResult();
        bool HasProperty(const char* name) {
            const TDetectResult& result = GetResult();
            return result.find(name) != result.end();
        }

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

}
