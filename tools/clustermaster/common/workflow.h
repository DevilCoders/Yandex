#pragma once

struct IWorkflow {
    virtual ~IWorkflow() = default;
    virtual void Run() noexcept = 0;
};
