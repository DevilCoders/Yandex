#pragma once

#include <util/generic/ptr.h>
#include <util/thread/pool.h>

#include <functional>

namespace NAntiRobot {

template <typename T>
class TDataProcessQueue {
public:
    using TProcessor = std::function<void(const T&)>;

    TDataProcessQueue(TAtomicSharedPtr<IThreadPool> slave, TProcessor processor)
        : Slave(slave)
        , Processor(std::move(processor))
    {
    }

    bool Add(const T& data) {
        // take a local reference to Processor to enable copy-capturing it by lambda
        const auto& func = Processor;
        return Slave->AddFunc([func, data]() {
            func(data);
        });
    }

    const TProcessor& GetProcessor() const {
        return Processor;
    }

    size_t Size() const {
        return Slave->Size();
    }

private:
    TAtomicSharedPtr<IThreadPool> Slave;
    TProcessor Processor;
};

}
