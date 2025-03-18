#pragma once

#include "workflow.h"

#include <util/generic/ptr.h>
#include <util/network/sock.h>
#include <util/system/thread.h>
#include <util/thread/pool.h>

#include <functional>

class TListeningThread {
public:
    using TWorkflowFactory = std::function<THolder<IWorkflow>(THolder<TStreamSocket>)>;

public:
    TListeningThread(const char* host, const TIpPort port, TWorkflowFactory workflowFactory);
    ~TListeningThread();

private:
    void ThreadProc();

private:
    TAdaptiveThreadPool Pool;
    TWorkflowFactory Factory;
    THolder<TStreamSocket> Socket;
    TThread Thread;
};
