#pragma once

namespace NAapi {
namespace {

class IQueueEvent {
public:
    virtual ~IQueueEvent() = default;

    /// Execute an action defined by implementation.
    virtual bool Execute(bool ok) = 0;

    /// Finish and destroy request.
    virtual void DestroyRequest() {
        delete this;
    }
};

} // namespace
} // namespace NAapi
