#include "outputqueue.h"

namespace NSnippets
{
void TOutputQueue::AddResultImpl(size_t index,
        const TVector<TString>& outputLines, bool isCompleted)
{
    TRec rec;
    rec.Index = index;
    rec.OutputLines = outputLines;
    rec.IsCompleted = isCompleted;
    Queue.push(rec);
    while (!Queue.empty() && Queue.top().Index == NextIndex) {
        if (Queue.top().IsCompleted) {
            for (const TString& line : Queue.top().OutputLines) {
                *OutputStream << line << "\n";
            }
            OutputStream->Flush();
            Progress->MarkCompleted(NextIndex);
        }
        Queue.pop();
        ++NextIndex;
    }
}

TOutputQueue::TOutputQueue(const TString& outputFile, const TString& resumeFile)
    : NextIndex(1)
{
    if (outputFile) {
        OutputFileStream.Reset(new TFixedBufferFileOutput(outputFile));
    }
    OutputStream = OutputFileStream ? OutputFileStream.Get() : &Cout;

    THolder<TProgressSaverStub> progress;
    if (resumeFile) {
        Progress.Reset(new TProgressSaver(resumeFile));
    } else {
        Progress.Reset(new TProgressSaverStub());
    }
}

bool TOutputQueue::CheckCompleted(size_t index)
{
    TGuard<TMutex> g(&OutputMutex);
    if (Progress->IsCompleted(index)) {
        AddResultImpl(index, TVector<TString>(), false);
        return true;
    }
    return false;
}

void TOutputQueue::AddCompletedResult(size_t index, const TVector<TString>& outputLines)
{
    TGuard<TMutex> g(&OutputMutex);
    AddResultImpl(index, outputLines, true);
}

void TOutputQueue::AddFailedResult(size_t index)
{
    TGuard<TMutex> g(&OutputMutex);
    AddResultImpl(index, TVector<TString>(), false);
}

} // namespace NSnippets
