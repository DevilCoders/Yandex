#pragma once
#include <util/stream/file.h>
#include <util/system/file.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NSnippets
{
    class TProgressSaverStub
    {
    public:
        virtual ~TProgressSaverStub() {
        }
        virtual void MarkCompleted(size_t tag) {
            Y_UNUSED(tag);
        }
        virtual bool IsCompleted(size_t tag) {
            Y_UNUSED(tag);
            return false;
        }
    };

    class TProgressSaver: public TProgressSaverStub
    {
        TFile ProgressFile;
        TUnbufferedFileOutput Out;
        THashSet<size_t> WorkDone;
    public:
        TProgressSaver(const TString& fileName);
        void MarkCompleted(size_t tag) override;
        bool IsCompleted(size_t tag) override;
    };
}
