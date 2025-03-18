#include <library/cpp/threading/atomic_shared_ptr/atomic_shared_ptr.h>
#include <library/cpp/testing/unittest/registar.h>
#include <atomic>
#include <thread>

static constexpr auto MO_RELAXED = std::memory_order_relaxed;
static constexpr auto MO_RELEASE = std::memory_order_release;
static constexpr auto MO_ACQUIRE = std::memory_order_acquire;
static constexpr auto MO_SEQUENCE = std::memory_order_seq_cst;

namespace {

// encapsulate into anonymous namespace
// no need to debug the class
class TSyncStart {
public:
    explicit TSyncStart(size_t number_of_agents)
        : NumberOfAgents(number_of_agents)
    {}

    void WaitForGreenLight() {
        NumberOfWaitingAgents.fetch_add(1, MO_RELEASE);
        while (!GreenLight.load(MO_ACQUIRE))
            ;
    }

    void Start() {
        for (;;) {
            auto ready = NumberOfWaitingAgents.load(MO_RELAXED);
            if (ready == NumberOfAgents)
                break;
        }
        // never store GreenLight before acquiring right NumberOfWaitingAgents
        std::atomic_thread_fence(MO_SEQUENCE);
        GreenLight.store(true, MO_RELEASE);
    }

    void Reset() {
        NumberOfWaitingAgents.store(0, std::memory_order_relaxed);
        GreenLight.store(false, std::memory_order_relaxed);
    }

protected:
    size_t NumberOfAgents;
    std::atomic<size_t> NumberOfWaitingAgents = {0};
    std::atomic<bool> GreenLight = {false};
};

} // anonymous namespace

Y_UNIT_TEST_SUITE(SharedPtr) {

    Y_UNIT_TEST(CreateAndDestroyEmpty) {
        TTrueAtomicSharedPtr<int> ptr;

        UNIT_ASSERT(!ptr);
        UNIT_ASSERT_EQUAL(ptr.get(), nullptr);
    }


    Y_UNIT_TEST(CreateAndDestroy) {
        TTrueAtomicSharedPtr<int> ptr(new int);
        UNIT_ASSERT(ptr);
        UNIT_ASSERT_UNEQUAL(ptr.get(), nullptr);

        *ptr = 42;
        UNIT_ASSERT_EQUAL(*ptr, 42);
        UNIT_ASSERT_EQUAL(*ptr.get(), 42);
    }


    Y_UNIT_TEST(CreateAndDestroyStruct) {
        static int object_counter = 0;

        struct Tracking {
            Tracking() {
                ++object_counter;
            }

            ~Tracking() {
                value = 0;
                --object_counter;
            }

            int value = 42;
        };

        {
            TTrueAtomicSharedPtr<Tracking> ptr(new Tracking);
            UNIT_ASSERT(ptr);
            UNIT_ASSERT_UNEQUAL(ptr.get(), nullptr);
            UNIT_ASSERT_EQUAL(ptr->value, 42);
            UNIT_ASSERT_EQUAL(ptr.get()->value, 42);
            UNIT_ASSERT_EQUAL((*ptr).value, 42);
            UNIT_ASSERT_EQUAL(object_counter, 1);

            {
                TTrueAtomicSharedPtr<Tracking> ptr2 = ptr;
                UNIT_ASSERT(ptr2);
                UNIT_ASSERT_UNEQUAL(ptr2.get(), nullptr);
                UNIT_ASSERT_EQUAL(ptr2->value, 42);
                UNIT_ASSERT_EQUAL(ptr2.get()->value, 42);
                UNIT_ASSERT_EQUAL((*ptr2).value, 42);
                UNIT_ASSERT_EQUAL(object_counter, 1);
            }

            UNIT_ASSERT(ptr);
            UNIT_ASSERT_UNEQUAL(ptr.get(), nullptr);
            UNIT_ASSERT_EQUAL(ptr->value, 42);
            UNIT_ASSERT_EQUAL(ptr.get()->value, 42);
            UNIT_ASSERT_EQUAL((*ptr).value, 42);
            UNIT_ASSERT_EQUAL(object_counter, 1);
        }
        UNIT_ASSERT_EQUAL(object_counter, 0);
    }

    Y_UNIT_TEST(ManyThreads) {
        static constexpr uint64_t LOOP_COUNT = 1000000;
        static constexpr uint64_t NUMBER_OF_PRODUCERS = 2;
        static constexpr uint64_t NUMBER_OF_CONSUMERS = 2;
        static constexpr uint64_t VALUE_MASK = ((uintptr_t)1 << 32) - 1;
        static std::atomic<int> object_counter{0};
        TSyncStart greenlight(NUMBER_OF_PRODUCERS + NUMBER_OF_CONSUMERS);

        struct Tracking {
            Tracking() {
                object_counter.fetch_add(1, std::memory_order_relaxed);
            }

            ~Tracking() {
                value.store(0, std::memory_order_relaxed);
                object_counter.fetch_sub(1, std::memory_order_relaxed);
            }

            std::atomic<uint64_t> value;
        };

        TTrueAtomicSharedPtr<Tracking> shared_point;
        std::atomic<uint64_t> all_done{0};

        auto produce_lambda = [&](const uint64_t start) {
            greenlight.WaitForGreenLight();
            for (uint64_t i = 42 + start; i < LOOP_COUNT + start + 42; ++i) {
                TTrueAtomicSharedPtr<Tracking> local(new Tracking);
                local->value.store(i, std::memory_order_relaxed);
                shared_point = local;
            }
            all_done.fetch_add(1, std::memory_order_relaxed);
        };

        auto consume_lambda = [&]() {
            uint64_t last_value[NUMBER_OF_PRODUCERS];
            for (uint64_t i = 0; i < NUMBER_OF_PRODUCERS; ++i)
                last_value[i] = 42;

            greenlight.WaitForGreenLight();
            while (all_done.load(std::memory_order_relaxed) < NUMBER_OF_PRODUCERS) {
                TTrueAtomicSharedPtr<Tracking> local;
                local = shared_point;
                if (!local)
                    continue;
                uint64_t i = local->value.load(std::memory_order_relaxed);
                uint64_t producer = i >> 32;
                uint64_t value = i & VALUE_MASK;
                UNIT_ASSERT_LT(producer, NUMBER_OF_PRODUCERS);
                UNIT_ASSERT_LE(last_value[producer], value);
                last_value[producer] = value;
            }

            TTrueAtomicSharedPtr<Tracking> local;
            local = shared_point;
            UNIT_ASSERT(local);
            uint64_t i = local->value.load(std::memory_order_relaxed);
            uint64_t producer = i >> 32;
            uint64_t value = i & VALUE_MASK;
            UNIT_ASSERT_LT(producer, NUMBER_OF_PRODUCERS);
            UNIT_ASSERT_LE(last_value[producer], value);
            last_value[producer] = value;

            bool at_least_one = false;
            for (uint64_t i = 0; i < NUMBER_OF_PRODUCERS; ++i)
                if (last_value[i] == 42 + LOOP_COUNT - 1) {
                    at_least_one = true;
                    break;
                }
            UNIT_ASSERT(at_least_one);
        };

        std::thread producers[NUMBER_OF_PRODUCERS];
        for (uint64_t i = 0; i < NUMBER_OF_PRODUCERS; ++i)
            producers[i] = std::thread(produce_lambda, i << 32);

        std::thread consumers[NUMBER_OF_CONSUMERS];
        for (uint64_t i = 0; i < NUMBER_OF_CONSUMERS; ++i)
            consumers[i] = std::thread(consume_lambda);

        greenlight.Start();
        for (uint64_t i = 0; i < NUMBER_OF_PRODUCERS; ++i)
            producers[i].join();
        for (uint64_t i = 0; i < NUMBER_OF_CONSUMERS; ++i)
            consumers[i].join();
        UNIT_ASSERT_EQUAL(object_counter.load(std::memory_order_relaxed), 1);
    }
}
