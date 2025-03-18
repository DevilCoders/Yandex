#pragma once
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSnippets {

class ILineProcessor {
public:
    virtual void ProcessLine(const TString& line, size_t lineNum) = 0;
};

class ILineProcessorFactory {
public:
    virtual ILineProcessor* CreateProcessor() = 0;
    virtual void DestroyProcessor(ILineProcessor* processor) = 0;
};

class TProcessLineJobQueue {
    class TImpl;
    THolder<TImpl> Impl;
public:
    TProcessLineJobQueue(ILineProcessorFactory& processorFactory, int nthreads);
    ~TProcessLineJobQueue();
    void AddLine(const TString& line, size_t lineNum);
    void AddAllLines(const TString& inputFile);
};

} // namespace NSnippets
