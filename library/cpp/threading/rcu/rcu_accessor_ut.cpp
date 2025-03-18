#include "rcu_accessor.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NThreading {
    Y_UNIT_TEST_SUITE(RcuAccessor) {
        Y_UNIT_TEST(Constructor) {
            TRcuAccessor<int> rcu(5);
            UNIT_ASSERT_VALUES_EQUAL(*rcu.Get(), 5);
        }

        Y_UNIT_TEST(SetValue) {
            TRcuAccessor<int> rcu;
            rcu.Set(5);
            UNIT_ASSERT_VALUES_EQUAL(*rcu.Get(), 5);
        }

        Y_UNIT_TEST(ReferenceAliveAfterSet) {
            TRcuAccessor<int> rcu;

            TVector<TRcuAccessor<int>::TReference> refs;
            for (int i = 0; i < 10; ++i) {
                rcu.Set(i);
                refs.push_back(rcu.Get());
            }

            for (size_t i = 0; i < refs.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(*refs[i], i);
            }
        }

        enum EObjectState {
            OS_NOT_CREATED,
            OS_CREATED,
            OS_DESTROYED,
        };

        struct TObject {
            EObjectState& State;

            TObject(EObjectState& state)
                : State(state)
            {
                State = OS_CREATED;
            }

            ~TObject() {
                State = OS_DESTROYED;
            }
        };

        Y_UNIT_TEST(ObjectIsDestroyed) {
            EObjectState stateOne = OS_NOT_CREATED;
            EObjectState stateTwo = OS_NOT_CREATED;

            TRcuAccessor<THolder<TObject>> rcu{MakeHolder<TObject>(stateOne)};
            UNIT_ASSERT_EQUAL(stateOne, OS_CREATED);

            {
                auto ref = rcu.Get();
                UNIT_ASSERT_EQUAL(stateOne, OS_CREATED);
                rcu.Set(MakeHolder<TObject>(stateTwo));
                UNIT_ASSERT_EQUAL(stateOne, OS_CREATED);
            }
            UNIT_ASSERT_EQUAL(stateOne, OS_DESTROYED);
        }

        Y_UNIT_TEST(PointerRcu) {
            struct TRec {
                int A, B;
            };

            TRcuAccessor<TRec*> rcu;
            UNIT_ASSERT(!rcu.Get());

            rcu.Set(THolder<const TRec>(new TRec{1, 2}));

            auto ref = rcu.Get();
            UNIT_ASSERT(ref);
            UNIT_ASSERT_VALUES_EQUAL(ref->A, 1);
            UNIT_ASSERT_VALUES_EQUAL(ref->B, 2);
        }

        Y_UNIT_TEST(SimpleStruct) {
            struct Foo {
                int A, B;
            };

            {
                TRcuAccessor<Foo> rcu;
            }
            {
                TRcuAccessor<Foo> rcu({3, 4});
                UNIT_ASSERT_VALUES_EQUAL(rcu.Get()->A, 3);
                UNIT_ASSERT_VALUES_EQUAL(rcu.Get()->B, 4);

                rcu.Set({1, 2});
                UNIT_ASSERT_VALUES_EQUAL(rcu.Get()->A, 1);
                UNIT_ASSERT_VALUES_EQUAL(rcu.Get()->B, 2);
            }
        }
    }

}
