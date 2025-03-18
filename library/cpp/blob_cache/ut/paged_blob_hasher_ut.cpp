#include "paged_blob_hasher.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/deque.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/xrange.h>
#include <util/random/mersenne.h>
#include <util/stream/buffer.h>
#include <util/thread/pool.h>
#include <library/cpp/threading/mtp_tasks/tasks.h>
#include <util/datetime/base.h>

namespace NOnePageHasher {
    struct THasherTraits {
        enum {
            NumPairs = 100 * 100,
            NumPages = 1,
            PageSizeInKb = 1000, // Never run out of space
            NumPageSlots = 1000,
            RetryCount = 100
        };
    };
}

namespace NTwoPageHasher {
    struct THasherTraits {
        enum {
            NumPairs = 100 * 100,
            NumPages = 2,
            PageSizeInKb = 1000, // Never run out of space
            NumPageSlots = 1000,
            RetryCount = 100
        };
    };
}

template <typename Traits>
class THasherChecker {
public:
    using THasher = TPagedBlobHasher<TStringBuf,
                                     NPagedBlobHasherDefaults::TStringBufCopier,
                                     TEqualTo<TStringBuf>,
                                     THash<TStringBuf>,
                                     Traits>;

    enum class EMode {
        RandomThreads,
        IdenticalThreads
    };

    struct TOptions {
        size_t NumKeys = 0;
        size_t NumRW = 0;
        EMode Mode = EMode::RandomThreads;
        size_t SeedInitializer = 42;
    };

    enum class EMessageType {
        Store,
        Load,
        Page
    };

    struct TCounters {
        size_t NumPageFail = 0;
        size_t NumStoreFail = 0;
        size_t NumLoadFail = 0;
    };

    struct TMessage {
        TInstant When;
        EMessageType Type = EMessageType::Store;
        bool Success = false;
        TString Key;
        TString ValueComputed;
        TString ValueFromCache;

        TMessage() {
            When = Now();
        }

        void Check() {
            Cdbg << ToString() << Endl;
            if (EMessageType::Load == Type && Success) {
                Cdbg << "EQUAL" << '\t' << ValueFromCache << '\t' << ValueComputed << Endl;
                UNIT_ASSERT_EQUAL(ValueComputed, ValueFromCache);
            }
        }

        TString ToString() {
            TStringStream out;
            switch (Type) {
                case EMessageType::Store: {
                    if (Success) {
                        out << "STORE" << '\t' << "SUCCESS" << '\t' << "@" << When.MicroSeconds() << '\t' << Key << '\t' << ValueComputed;
                    } else {
                        out << "STORE" << '\t' << "FAILURE" << '\t' << "@" << When.MicroSeconds() << '\t' << Key << '\t' << ValueComputed;
                    }
                    break;
                }
                case EMessageType::Load: {
                    if (Success) {
                        out << "LOAD" << '\t' << "SUCCESS" << '\t' << "@" << When.MicroSeconds() << '\t' << Key << '\t' << ValueFromCache;
                    } else {
                        out << "LOAD" << '\t' << "FAILURE" << '\t' << "@" << When.MicroSeconds() << '\t' << Key;
                    }
                    break;
                }
                case EMessageType::Page: {
                    if (Success) {
                        out << "PAGE" << '\t' << "SUCCESS" << '\t' << "@" << When.MicroSeconds();
                    } else {
                        out << "PAGE" << '\t' << "FAILURE" << '\t' << "@" << When.MicroSeconds();
                    }
                    break;
                }
            }
            return out.Str();
        }

        void UpdateCounters(TCounters& counters) {
            switch (Type) {
                case EMessageType::Store: {
                    if (Success) {
                    } else {
                        counters.NumStoreFail += 1;
                    }
                    break;
                }
                case EMessageType::Load: {
                    if (Success) {
                    } else {
                        counters.NumLoadFail += 1;
                    }
                    break;
                }
                case EMessageType::Page: {
                    if (Success) {
                    } else {
                        counters.NumPageFail += 1;
                    }
                    break;
                }
            }
        }
    };

    struct TState {
        size_t NumKeys = 0;
        size_t NumRW = 0;
        size_t Seed = 0;

        TDeque<TMessage> Messages;
    };

    class TTask {
    public:
        TTask(size_t index, THasher& hasher, TState& state)
            : Hasher(hasher)
            , State(state)
            , Rand(state.Seed)
            , Index(index)
        {
        }

        TBlob GetValue(const TStringBuf& s) {
            return TBlob::FromString(ToString(THash<TStringBuf>()(s)));
        }

        void ProcessTask() {
            for (size_t i = 0; i != State.NumRW; ++i) {
                size_t keyNum = Rand.Uniform(State.NumKeys);
                auto key = ToString(keyNum);
                auto value = GetValue(key);

                if (!Page) {
                    Page = Hasher.AllocPage(1);
                    TMessage msg;
                    msg.Type = EMessageType::Page;
                    msg.Success = !!Page;
                    State.Messages.push_back(msg);
                }

                if (Page) {
                    TBlob valueFromCache;

                    TMessage msg;
                    msg.Type = EMessageType::Load;
                    msg.Key = key;
                    msg.ValueComputed = TString(value.AsCharPtr(), value.Size());

                    bool succ = Page->GetBlobOrFail(key, valueFromCache);
                    msg.Success = succ;
                    State.Messages.push_back(msg);

                    if (succ) {
                        State.Messages.back().ValueFromCache = TString(valueFromCache.AsCharPtr(), valueFromCache.Size());
                        continue;
                    }
                }

                if (Page) {
                    TMessage msg;
                    msg.Type = EMessageType::Store;
                    msg.Key = key;
                    msg.ValueComputed = TString(value.AsCharPtr(), value.Size());

                    bool succ = Page->StoreBlobOrFail(key, value);
                    msg.Success = succ;
                    State.Messages.push_back(msg);
                }
            }
        }

    private:
        THasher& Hasher;
        TState& State;
        TMersenne<size_t> Rand;
        size_t Index = 0;
        IPagedBlobHasher<TStringBuf>::TPageUniqPtr Page = nullptr; // Do not release until all threads are done
    };

    THasherChecker(size_t numThreads = 1)
        : NumThreads(numThreads)
    {
        ResetHasher();
    }

    void ResetHasher() {
        Hasher.Reset(new THasher);
    }

    TVector<TCounters> Run(const TOptions& options) {
        THolder<IThreadPool> queue = CreateThreadPool(NumThreads);
        TVector<TState> states(NumThreads);
        TSimpleMtpTask<TTask> tasks(queue.Get());
        for (size_t i = 0; i != NumThreads; ++i) {
            states[i].NumKeys = options.NumKeys;
            states[i].NumRW = options.NumRW;
            switch (options.Mode) {
                case EMode::IdenticalThreads: {
                    states[i].Seed = options.SeedInitializer;
                    break;
                }
                case EMode::RandomThreads: {
                    states[i].Seed = (options.SeedInitializer + 3) * (i + 7) * 13;
                    break;
                }
            }
            tasks.AddAndOwn(new TTask(i, *Hasher, states[i]));
        }
        tasks.Process();
        TVector<TCounters> counters;
        counters.resize(NumThreads);
        for (size_t i = 0; i != NumThreads; ++i) {
            Cdbg << "THREAD " << i << Endl;
            for (auto& message : states[i].Messages) {
                message.Check();
                message.UpdateCounters(counters[i]);
            }
            Cdbg << "STATS FOR THREAD " << i << ": NUM_PAGE_FAIL=" << counters[i].NumPageFail
                 << " NUM_STORE_FAIL=" << counters[i].NumStoreFail
                 << " NUM_LOAD_FAIL=" << counters[i].NumLoadFail << Endl;
        }
        return counters;
    }

private:
    THolder<THasher> Hasher;
    size_t NumThreads = 0;
};

Y_UNIT_TEST_SUITE(TPagedBlobHasherTest){
    Y_UNIT_TEST(TestOnePageHasherSingleThreadWithinCapacity){
        using TChecker = THasherChecker<NOnePageHasher::THasherTraits>;
TChecker checker(1);

for (size_t i = 0; i != 10; ++i) {
    Cdbg << "RUN #" << i << Endl;
    TChecker::TOptions options;
    options.NumKeys = 100;
    options.NumRW = 1000;
    options.Mode = TChecker::EMode::RandomThreads;
    options.SeedInitializer = i % 5;
    // Do not reset hasher state here
    auto counters = checker.Run(options);
    UNIT_ASSERT_EQUAL(counters[0].NumPageFail, 0);
    UNIT_ASSERT_EQUAL(counters[0].NumStoreFail, 0);
    UNIT_ASSERT(counters[0].NumLoadFail > 0);
}
}

Y_UNIT_TEST(TestOnePageHasherSingleThreadOutOfCapacity) {
    using TChecker = THasherChecker<NOnePageHasher::THasherTraits>;
    TChecker checker(1);

    {
        TChecker::TOptions options;
        options.NumKeys = 10000;
        options.NumRW = 10000;
        options.Mode = TChecker::EMode::RandomThreads;
        checker.ResetHasher();
        auto counters = checker.Run(options);
        UNIT_ASSERT(counters[0].NumStoreFail > 0);
        UNIT_ASSERT(counters[0].NumLoadFail > 0);
    }
}

Y_UNIT_TEST(TestOnePageHasherTwoThreads) {
    using TChecker = THasherChecker<NOnePageHasher::THasherTraits>;
    TChecker checker(2);

    {
        TChecker::TOptions options;
        options.NumKeys = 10000;
        options.NumRW = 10000;
        options.Mode = TChecker::EMode::RandomThreads;
        checker.ResetHasher();
        auto counters = checker.Run(options);
        UNIT_ASSERT(10000 == counters[0].NumPageFail || 10000 == counters[1].NumPageFail);
    }
}

Y_UNIT_TEST(TestTwoPageHasherTwoRandomThreads) {
    using TChecker = THasherChecker<NTwoPageHasher::THasherTraits>;
    TChecker checker(2);

    {
        TChecker::TOptions options;
        options.NumKeys = 10000;
        options.NumRW = 10000;
        options.Mode = TChecker::EMode::RandomThreads;
        checker.ResetHasher();
        auto counters = checker.Run(options);
        UNIT_ASSERT_EQUAL(counters[0].NumPageFail, 0);
        UNIT_ASSERT_EQUAL(counters[1].NumPageFail, 0);
    }
}

Y_UNIT_TEST(TestTwoPageHasherTwoIdenticalThreads) {
    using TChecker = THasherChecker<NTwoPageHasher::THasherTraits>;
    TChecker checker(2);

    {
        TChecker::TOptions options;
        options.NumKeys = 10000;
        options.NumRW = 10000;
        options.Mode = TChecker::EMode::IdenticalThreads;
        checker.ResetHasher();
        auto counters = checker.Run(options);
        UNIT_ASSERT_EQUAL(counters[0].NumPageFail, 0);
        UNIT_ASSERT_EQUAL(counters[1].NumPageFail, 0);
    }
}

Y_UNIT_TEST(TestTwoPageHasherTenRandomThreads) {
    using TChecker = THasherChecker<NTwoPageHasher::THasherTraits>;
    TChecker checker(10);

    {
        TChecker::TOptions options;
        options.NumKeys = 10000;
        options.NumRW = 10000;
        options.Mode = TChecker::EMode::RandomThreads;
        checker.ResetHasher();
        auto counters = checker.Run(options);
        size_t totalPageFail = 0;
        for (auto& threadCounters : counters) {
            totalPageFail += threadCounters.NumPageFail;
        }
        UNIT_ASSERT_EQUAL(totalPageFail, 8 * 10000);
    }
}
}
;
