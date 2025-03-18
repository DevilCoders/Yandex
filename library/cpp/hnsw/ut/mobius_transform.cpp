#include <library/cpp/hnsw/index_builder/dense_vector_storage.h>
#include <library/cpp/hnsw/index_builder/mobius_transform.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(THnswMobiusTransformTest) {
    template<class T>
    class TStorage{
    public:
        using TItem = const T*;

        TStorage(TVector<TVector<T>> vectors) : dim(vectors[0].size()) {
            itemStorage = std::move(vectors);
        }

        TStorage(size_t dimension) : dim(dimension) {}

        const T* GetItem(ui32 id) const {
            return itemStorage[id].data();
        }

        size_t GetNumItems() const {
            return itemStorage.size();
        }

        size_t GetDimension() const {
            return dim;
        }
    private:
        TVector<TVector<T>> itemStorage;
        size_t dim;
    };

    template<class T>
    void check(const TVector<TVector<T>>& expectedVectors, const NHnsw::TDenseVectorStorage<T>& transformedVectors){
        double eps = 1e-6;
        UNIT_ASSERT_EQUAL(expectedVectors.size(), transformedVectors.GetNumItems());
        for(size_t i = 0; i < expectedVectors.size(); i++){
            UNIT_ASSERT_EQUAL(expectedVectors[i].size(), transformedVectors.GetDimension());
            for(size_t j = 0; j < expectedVectors[i].size(); j++){
                UNIT_ASSERT_DOUBLES_EQUAL(expectedVectors[i][j], transformedVectors.GetItem(i)[j], eps);
            }
        }
    }

    Y_UNIT_TEST(DoubleTest) {
        TVector<TVector<double>> vectors = {
            {0.1, 0.1, 0.1, 0.1},
            {10, 10, 0, 0},
            {1, 0, 0, 0},
            {-1, -1, -1, -1}
        };
        TStorage<double> dataset(vectors);
        TVector<TVector<double>> expectedVectors = {
            {2.5, 2.5, 2.5, 2.5},
            {0.05, 0.05, 0, 0},
            {1, 0, 0, 0},
            {-0.25, -0.25, -0.25, -0.25}
        };

        auto transformedVectors = NHnsw::TransformMobius(dataset);
        check(expectedVectors, transformedVectors);
    }

    Y_UNIT_TEST(I8Test) {
        TVector<TVector<i8>> vectors = {
            {12, 13, 14, 15},
            {125, 125, 125, 125},
            {1, 0, 0, 0},
            {-1, -1, -1, -1}
        };
        TStorage<i8> dataset(vectors);
        TVector<TVector<float>> expectedVectors = {
            {0.0163487738, 0.0177111717, 0.0190735695, 0.0204359673},
            {0.002, 0.002, 0.002, 0.002},
            {1, 0, 0, 0},
            {-0.25, -0.25, -0.25, -0.25}
        };

        auto transformedVectors = NHnsw::TransformMobius(dataset);
        check(expectedVectors, transformedVectors);
    }

    Y_UNIT_TEST(I32Test) {
        TVector<TVector<i32>> vectors = {
            {12, 13, -14, 15},
            {1000000000, 1000000000, 1000000000, 1000000000},
            {1, 0, 0, 0},
            {-1, -1, -1, -1}
        };
        TStorage<i32> dataset(vectors);
        TVector<TVector<float>> expectedVectors = {
            {0.0163487738, 0.0177111717, -0.0190735695, 0.0204359673},
            {1.25e-10, 1.25e-10, 1.25e-10, 1.25e-10},
            {1, 0, 0, 0},
            {-0.25, -0.25, -0.25, -0.25}
        };
        auto transformedVectors = NHnsw::TransformMobius(dataset);
        check(expectedVectors, transformedVectors);
    }

    Y_UNIT_TEST(FloatTest) {
        TVector<TVector<float>> vectors = {
            {0.1, 0.1, 0.1, 0.1},
            {250, 250, 250, 250},
            {0.01, 0.01, 0.01, 0.01},
            {-1, -1, -1, -1}
        };
        TStorage<float> dataset(vectors);

        TVector<TVector<float>> expectedVectors = {
            {2.5, 2.5, 2.5, 2.5},
            {0.001, 0.001, 0.001, 0.001},
            {25, 25, 25, 25},
            {-0.25, -0.25, -0.25, -0.25}
        };

        auto transformedVectors = NHnsw::TransformMobius(dataset);
        check(expectedVectors, transformedVectors);
    }

    Y_UNIT_TEST(EmptyTest) {
        TStorage<float> dataset(10);
        TVector<TVector<float>> expectedVectors;
        auto transformedVectors = NHnsw::TransformMobius(dataset);
        check(expectedVectors, transformedVectors);
    }
}
