#include <library/cpp/nirvana/job_context/job_context.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/system/env.h>

Y_UNIT_TEST_SUITE(JobContext) {
    Y_UNIT_TEST(LoadTestFile) {
        SetEnv("JWD", ArcadiaSourceRoot() + "/library/cpp/nirvana/job_context/ut/data");
        const auto jobContext = NNirvana::LoadJobContext<NNirvana::TJobContextData>(true);
    }

    Y_UNIT_TEST(LoadMrTestFile) {
        SetEnv("JWD", ArcadiaSourceRoot() + "/library/cpp/nirvana/job_context/ut/mr_data");
        const auto jobContext = NNirvana::LoadJobContext<NNirvana::TMrJobContextData>(false); // not using strict validation (which would be really great for a unit test) because it's broken in domscheme for structures with inheritance
        UNIT_ASSERT_VALUES_EQUAL(jobContext->Context().MrCluster().Name(), "banach");
    }
}
