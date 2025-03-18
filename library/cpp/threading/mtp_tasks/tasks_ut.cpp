#include "tasks.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/datetime.h>
#include <util/digest/numeric.h>
#include <util/generic/vector.h>

class TStatCounter {
public:
    inline static void IncCreated() {
        AtomicIncrement(Created_);
    }

    inline static void IncProceed() {
        AtomicIncrement(Proceed_);
    }

    inline static void IncThrows() {
        AtomicIncrement(Throws_);
    }

    inline static size_t Created() {
        return Created_;
    }

    inline static size_t Proceed() {
        return Proceed_;
    }

    inline static size_t Throws() {
        return Throws_;
    }

private:
    static TAtomic Created_;
    static TAtomic Proceed_;
    static TAtomic Throws_;
};

TAtomic TStatCounter::Created_ = 0;
TAtomic TStatCounter::Proceed_ = 0;
TAtomic TStatCounter::Throws_ = 0;

class TMtpTaskTest: public TTestBase {
    UNIT_TEST_SUITE(TMtpTaskTest);
    UNIT_TEST(TestMtpTaskPool);
    UNIT_TEST(TestMtpTask);
    UNIT_TEST_SUITE_END();

    class TCheckTaskContext: public TSimpleRefCount<TCheckTaskContext> {
    public:
        TCheckTaskContext(bool throwException)
            : Name_(ToString(TStatCounter::Created()))
            , ThrowException_(throwException)
        {
            TStatCounter::IncCreated();
        }

        inline void ProcessTask() {
            usleep(IntHash(MicroSeconds()) % 1000000);

            if (ThrowException_) {
                TStatCounter::IncThrows();

                throw std::exception();
            }

            TStatCounter::IncProceed();
        }

    private:
        const TString Name_;
        const bool ThrowException_;
    };

    class TCheckTask {
    public:
        TCheckTask(size_t contextCount, size_t throwsException) {
            for (size_t i = 0; i < contextCount; ++i) {
                Contexts_.push_back(new TCheckTaskContext(i < throwsException));
            }
        }

        ~TCheckTask() {
            for (size_t i = 0; i < Contexts_.size(); ++i) {
                delete Contexts_[i];
            }
        }

        inline void Process(TMtpTasksPool::TMtpTaskFromPoolRef task) {
            TTaskToMultiThreadProcessing<TCheckTaskContext> mtp;

            task->Process(&mtp, (void**)Contexts_.data(), Contexts_.size());
        }

    private:
        TVector<TCheckTaskContext*> Contexts_;
    };

public:
    inline void TestMtpTaskPool() {
        TMtpTasksPool pool;

        TMtpTasksPool::TMtpTaskFromPoolRef task = pool.Acquire();

        UNIT_ASSERT(task.Get() != nullptr);
    }

    inline void TestMtpTask() {
        TMtpTasksPool pool;

        TMtpTasksPool::TMtpTaskFromPoolRef task = pool.Acquire();
        TCheckTask checkTask(15, 3);

        checkTask.Process(task);

        UNIT_ASSERT_EQUAL(TStatCounter::Created(), TStatCounter::Proceed() + TStatCounter::Throws());
    }
};

UNIT_TEST_SUITE_REGISTRATION(TMtpTaskTest);
