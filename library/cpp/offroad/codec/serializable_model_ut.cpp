#include "model_16.h"
#include "model_64.h"
#include "sampler_16.h"
#include "sampler_64.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/buffer.h>

#include <array>

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TModelTypeCheckingTest) {
    template <class Sampler, class Model>
    Model TrainModel() {
        Sampler sampler;
        for (size_t i = 0; i < 500; ++i) {
            std::array<ui32, Sampler::BlockSize> v;
            for (size_t j = 0; j < Sampler::BlockSize; ++j) {
                v[j] = j * j + i;
            }
            sampler.Write(0, v);
        }
        return sampler.Finish();
    }

    Y_UNIT_TEST(TestModel16ToModel16) {
        TModel16 learn16 = TrainModel<TSampler16, TModel16>();
        TBufferStream modelStream;
        ::Save(&modelStream, learn16);
        TModel16 test16;
        ::Load(&modelStream, test16);
    }

    Y_UNIT_TEST(TestModel16ToModel64) {
        try {
            TModel16 learn16 = TrainModel<TSampler16, TModel16>();
            TBufferStream modelStream;
            ::Save(&modelStream, learn16);
            TModel64 test64;
            ::Load(&modelStream, test64);
            Y_FAIL();
        } catch (...) {
        }
    }

    Y_UNIT_TEST(TestModel64ToModel16) {
        try {
            TModel64 learn64 = TrainModel<TSampler64, TModel64>();
            TBufferStream modelStream;
            ::Save(&modelStream, learn64);
            TModel16 test16;
            ::Load(&modelStream, test16);
            Y_FAIL();
        } catch (...) {
        }
    }
}
