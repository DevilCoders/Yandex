#include <library/cpp/testing/unittest/registar.h>

#include "doc_data_holder.h"

Y_UNIT_TEST_SUITE(TDocDataHolderTest) {
    class TLoader
        : public NDoom::IDocLumpLoader
    {
    public:
        bool HasDocLump(size_t /*docId*/) const override {
            return false;
        }

        void LoadDocRegions(TConstArrayRef<size_t> /*mapping*/, TArrayRef<TConstArrayRef<char>> /*regions*/) const override  {

        }

        TBlob DataHolder() const override {
            return {};
        }
    };

    Y_UNIT_TEST(TestPreLoad) {
        TSearchDocDataHolder holder;

        {
            NDoom::TSearchDocLoader searchLoader;

            UNIT_ASSERT(!holder.Contains(0));
            holder.PreLoad(0, std::move(searchLoader));
            UNIT_ASSERT(holder.Contains(0));
        }

        {
            auto loader = MakeHolder<TLoader>();
            auto loaderPtr = loader.Get();

            UNIT_ASSERT(!holder.Contains(1));
            holder.PreLoad(1, std::move(loader));
            UNIT_ASSERT(holder.Contains(1));
            UNIT_ASSERT(&holder.Get(1) == loaderPtr);
        }
    }
}
