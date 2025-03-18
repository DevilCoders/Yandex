#include "config_global.h"
#include "instance_hashing.h"

#include <antirobot/lib/addr.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/digest/numeric.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/stream/output.h>

#include <utility>

namespace NAntiRobot {

namespace {

class TTestSubnetHashing : public TTestBase {
public:
    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "IpV4SubnetBitsSizeForHashing = 16\n"
                                                       "IpV6SubnetBitsSizeForHashing = 48\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "CbbApiHost = ::\n"
                                                       "</Daemon>");
    }
};

template <size_t SubnetBits>
class TIp4Subnets {
public:
    static_assert(SubnetBits <= 32, "SubnetBits must be <= 32");

    class TIterator {
    public:
        explicit TIterator(size_t value)
            : Value(value)
        {
        }

        TAddr operator *() const {
            return TAddr::FromIp(Value << (32 - SubnetBits));
        }

        TIterator& operator ++() {
            ++Value;
            return *this;
        }

        bool operator ==(const TIterator& other) const {
            return Value == other.Value;
        }

        bool operator !=(const TIterator& other) const {
            return !(*this == other);
        }

    private:
        ui32 Value;
    };

    TIterator begin() const {
        return TIterator(0);
    }

    TIterator end() const {
        return TIterator(1 << SubnetBits);
    }
};

template <typename T>
bool IsUniformlyDistributed(const TVector<T>& data, T expectedAverage, size_t ratio) {
//    Cout << "expectedAverage " << expectedAverage << Endl;
//    for (const auto& x : data) {
//        Cout << x << " ";
//    }
//    Cout << Endl;

    return AllOf(data, [=](T value) {
        const auto diff = value > expectedAverage ? value - expectedAverage
                                                  : expectedAverage - value;
        return diff < ratio * expectedAverage / 100;
    });
}

using THasher = std::function<size_t(const TAddr&, size_t, size_t, const TIpRangeMap<size_t>&)>;

const TVector<TAddr>& Ip6SubnetsFromDaemonLog() {
    static const char* IPv6_ADDRS[] = {
#include "instance_hashing_ut.inc"
    };
    static const TVector<TAddr> result(IPv6_ADDRS, IPv6_ADDRS + Y_ARRAY_SIZE(IPv6_ADDRS));
    return result;
}

TVector<TAddr> BuildAddrData() {
    TVector<TAddr> result = Ip6SubnetsFromDaemonLog();
    for (const TAddr& addr : TIp4Subnets<16>()) {
        result.push_back(addr);
    }

    return result;
}

const TVector<TAddr>& GetAddrData() {
    static const TVector<TAddr> data = BuildAddrData();
    return data;
}

const size_t INSTANCE_COUNT[] = {15, 79, 120};

void TestReturnValue(THasher hasher) {
    for (auto instCount : INSTANCE_COUNT) {
        for (const TAddr& addr : GetAddrData()) {
            for (size_t attempt = 0; attempt < instCount; ++attempt) {
                UNIT_ASSERT(hasher(addr, attempt, instCount, {}) < instCount);
            }
        }
    }
}

void TestUniformDistribution(THasher hasher) {
    for (auto instCount : INSTANCE_COUNT) {
        TVector<size_t> hitCount(instCount);
        for (const TAddr& addr : GetAddrData()) {
            ++hitCount[hasher(addr, 0, instCount, {})];
        }

        const size_t expectedAverage = GetAddrData().size() / instCount;
        UNIT_ASSERT(IsUniformlyDistributed(hitCount, expectedAverage, 20));
    }
}

void TestDiversity(THasher hasher) {
    for (auto instCount : INSTANCE_COUNT) {
        THashSet<size_t> hashValues;
        for (const TAddr& addr : GetAddrData()) {
            hashValues.insert(hasher(addr, 0, instCount, {}));
        }
        UNIT_ASSERT(hashValues.size() >= 2 * instCount / 3);
    }
}

TVector<bool> MakeDeadInstances(size_t instanceCount, TReallyFastRng32& randGen) {
    TVector<bool> dead(instanceCount, false);

    const size_t deadInstances = 10 * instanceCount / 100 + randGen.Uniform(instanceCount / 2);
    for (size_t i = 0; i < deadInstances; ++i) {
        size_t instId;

        do {
            instId = randGen.Uniform(instanceCount);
        } while (dead[instId]);

        dead[instId] = true;
    }

    return dead;
}

void TestFaultTolerance(THasher hasher) {
    // Check that if some backends go down the hash-function
    // distributes addresses uniformly among the remaining backends.
    TReallyFastRng32 randGen(0);

    for (auto instCount : INSTANCE_COUNT) {
        const TVector<bool> dead = MakeDeadInstances(instCount, randGen);
        TVector<size_t> hitCount(instCount);

        for (const TAddr& addr : GetAddrData()) {
            for (size_t attempt = 0; attempt < instCount; ++attempt) {
                const auto hashValue = hasher(addr, attempt, instCount, {});
                if (!dead[hashValue]) {
                    ++hitCount[hashValue];
                    break;
                }
            }
        }

        TVector<size_t> liveHits;
        for (size_t i = 0; i < instCount; ++i) {
            if (!dead[i]) {
                liveHits.push_back(hitCount[i]);
            }
        }

        const auto expectedAverage = GetAddrData().size() / liveHits.size();
        UNIT_ASSERT(IsUniformlyDistributed(liveHits, expectedAverage, 40));
    }
}

template <typename SubnetSequence>
void TestSubnets(THasher hasher, const SubnetSequence& subnetSequence) {
    const size_t INSTANCE_COUNT = 79;
    const size_t TEST_IPS_FOR_SUBNET = 5;
    const size_t TEST_ATTEMPTS = 3;

    TReallyFastRng32 randGen(1);

    for (const TAddr& subnetAddr : subnetSequence) {
        for (size_t attempt = 0; attempt < TEST_ATTEMPTS; ++attempt) {
            const auto subnetHash = hasher(subnetAddr, attempt, INSTANCE_COUNT, {});
            for (size_t i = 0; i < TEST_IPS_FOR_SUBNET; ++i) {
                size_t testAddrBits = subnetAddr.IsIp4() ? 16 : 64;
                testAddrBits += 1 + randGen.Uniform(testAddrBits);

                TAddr minAddr, maxAddr;
                subnetAddr.GetIntervalForMask(testAddrBits, minAddr, maxAddr);

                UNIT_ASSERT_VALUES_EQUAL(subnetHash, hasher(minAddr, attempt, INSTANCE_COUNT, {}));
                UNIT_ASSERT_VALUES_EQUAL(subnetHash, hasher(maxAddr, attempt, INSTANCE_COUNT, {}));
            }
        }
    }
}

void TestIp4Subnets(THasher hasher) {
    TestSubnets(hasher, TIp4Subnets<16>());
}

void TestIp6Subnets(THasher hasher) {
    TestSubnets(hasher, Ip6SubnetsFromDaemonLog());
}

}

Y_UNIT_TEST_SUITE_IMPL(InstanceHashing, TTestSubnetHashing) {

// This macro simplifies test creation for different hash functions
#define CREATE_TESTS(HashFunc) \
    Y_UNIT_TEST(HashFunc##_ReturnValue) { \
        TestReturnValue(HashFunc); \
    } \
    Y_UNIT_TEST(HashFunc##_UniformDistribution) { \
        TestUniformDistribution(HashFunc); \
    } \
    Y_UNIT_TEST(HashFunc##_Diversity) { \
        TestDiversity(HashFunc); \
    } \
    Y_UNIT_TEST(HashFunc##_FaultTolerance) { \
        TestFaultTolerance(HashFunc); \
    } \
    Y_UNIT_TEST(HashFunc##_Ip4Subnets) { \
        TestIp4Subnets(HashFunc); \
    } \
    Y_UNIT_TEST(HashFunc##_Ip6Subnets) { \
        TestIp6Subnets(HashFunc); \
    }

CREATE_TESTS(ChooseInstance)

#undef CREATE_TESTS

    std::pair<TAddr, TAddr> GenerateTwoIpsFromSameSubnet(size_t subnetSize) {
        Y_ENSURE(subnetSize < 32);
        Y_ENSURE(subnetSize > 0);
        auto ip1AsNumber = RandomNumber<unsigned int>();
        auto ip2AsNumber = ip1AsNumber & ((~0U) << (32 - subnetSize)) + RandomNumber<unsigned int>(1U << (32 - subnetSize));
        return {TAddr::FromIp(ip1AsNumber), TAddr::FromIp(ip2AsNumber)};
    }

    Y_UNIT_TEST(CustomHashing) {
        const auto instanceCount = 124;
        for (auto _ : xrange(1000)) {
            Y_UNUSED(_);
            auto subnetSize = RandomNumber<size_t>(31) + 1;
            const auto& [ip1, ip2] = GenerateTwoIpsFromSameSubnet(subnetSize);
            TIpRangeMap<size_t> customHashingMap;
            customHashingMap.Insert({TIpInterval{ip1, ip1}, subnetSize});
            customHashingMap.Insert({TIpInterval{ip2, ip2}, subnetSize});

            for (auto attempt : xrange(1000)) {
                UNIT_ASSERT_VALUES_EQUAL(ChooseInstance(ip1, attempt, instanceCount, customHashingMap),
                                         ChooseInstance(ip2, attempt, instanceCount, customHashingMap));
            }
        }
    }

}

}
