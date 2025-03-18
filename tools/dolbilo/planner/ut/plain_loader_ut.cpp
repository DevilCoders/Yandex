#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/dolbilo/planner/plain.h>

namespace {
    class MockPlannerOutput: public IReqsLoader::IOutputter {
    public:
        using PlanItems = TVector<TDevastateItem>;

        void Add(const TDevastateItem& item) override {
            Items.push_back(item);
        }
        const PlanItems& GetItems() const { return Items; }

    private:
        PlanItems Items;

    };
}  // namespace

Y_UNIT_TEST_SUITE(DolbiloPlanner) {
    Y_UNIT_TEST(MultipleHeaders) {
        TStringStream input("0\thttp://test.com/rest/api\tHeader1: Value1\\nHeader2: Value2\n");
        TPlainLoader loader;
        MockPlannerOutput output;
        IReqsLoader::TParams params(&input, &output);
        loader.Process(&params);

        const auto& res = output.GetItems();
        UNIT_ASSERT_VALUES_EQUAL(res.size(), 1);
        UNIT_ASSERT_C(res[0].Data().Contains("Header1: Value1\r\nHeader2: Value2\r\n"), res[0].Data());
    }
}
