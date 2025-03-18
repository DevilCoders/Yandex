#include "jobqueue.h"

#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/thread/pool.h>

namespace NSnippets {

class TProcessLineJob : public IObjectInQueue {
    TString Line;
    size_t LineNum;
public:
    TProcessLineJob(const TString& line, size_t lineNum)
        : Line(line)
        , LineNum(lineNum)
    {
    }
    void Process(void* threadSpecificResource) override {
        THolder<TProcessLineJob> self(this);
        ILineProcessor* processor = (ILineProcessor*)threadSpecificResource;
        processor->ProcessLine(Line, LineNum);
    }
};

class TProcessLineJobQueue::TImpl : public TThreadPool {
    ILineProcessorFactory& ProcessorFactory;
public:
    TImpl(ILineProcessorFactory& processorFactory, int nthreads)
        : TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(true))
        , ProcessorFactory(processorFactory)
    {
        Start(nthreads, nthreads);
    }
    ~TImpl() override {
        Stop();
    }
    void* CreateThreadSpecificResource() override {
        return ProcessorFactory.CreateProcessor();
    }
    void DestroyThreadSpecificResource(void* resource) override {
        ProcessorFactory.DestroyProcessor((ILineProcessor*)resource);
    }
    void AddLine(const TString& line, size_t lineNum) {
        THolder<TProcessLineJob> job(new TProcessLineJob(line, lineNum));
        if (Add(job.Get())) {
            Y_UNUSED(job.Release());
        }
    }
    void AddAllLines(const TString& inputFile) {
        THolder<TFileInput> inputFileStream;
        if (inputFile) {
            inputFileStream.Reset(new TFileInput(inputFile));
        }
        IInputStream& input = inputFileStream ? *inputFileStream.Get() : Cin;
        TString line;
        size_t lineNum = 0;
        while (input.ReadLine(line)) {
            ++lineNum;
            AddLine(line, lineNum);
        }
    }
};

TProcessLineJobQueue::TProcessLineJobQueue(
        ILineProcessorFactory& processorFactory, int nthreads)
    : Impl(new TImpl(processorFactory, nthreads))
{
}

TProcessLineJobQueue::~TProcessLineJobQueue() {
}

void TProcessLineJobQueue::AddLine(const TString& line, size_t lineNum) {
    Impl->AddLine(line, lineNum);
}

void TProcessLineJobQueue::AddAllLines(const TString& inputFile) {
    Impl->AddAllLines(inputFile);
}

} // namespace NSnippets
