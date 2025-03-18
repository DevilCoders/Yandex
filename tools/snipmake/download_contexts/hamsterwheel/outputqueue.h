#pragma once
#include "progress.h"

#include <util/generic/ptr.h>
#include <util/generic/queue.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/system/mutex.h>

namespace NSnippets {

class TOutputQueue {
    class TRec {
    public:
        size_t Index;
        TVector<TString> OutputLines;
        bool IsCompleted;

        bool operator>(const TRec& other) const {
            return Index > other.Index;
        }
    };

    TPriorityQueue<TRec, TDeque<TRec>, TGreater<TRec>> Queue;
    THolder<TFixedBufferFileOutput> OutputFileStream;
    IOutputStream* OutputStream;
    THolder<TProgressSaverStub> Progress;
    size_t NextIndex;
    TMutex OutputMutex;

    void AddResultImpl(size_t index, const TVector<TString>& outputLines, bool isCompleted);

public:
    TOutputQueue(const TString& outputFile, const TString& resumeFile);
    bool CheckCompleted(size_t index);
    void AddCompletedResult(size_t index, const TVector<TString>& outputLines);
    void AddFailedResult(size_t index);
};

} // namespace NSnippets
