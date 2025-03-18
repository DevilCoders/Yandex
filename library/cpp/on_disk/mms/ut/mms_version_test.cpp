#include <contrib/libs/mms/impl/fwd.h>
#include <contrib/libs/mms/version.h>

#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/maybe.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/vector.h>

#include <library/cpp/testing/unittest/registar.h>

// Names "traverseFields", "formatVersion", "enforceVersion" are not styleguided
// because they are enforced by original MMS library (contrib/libs/mms).

#define DUPLICATE_STRUCT_ENTIRE                                                \
    int a, b, c;                                                               \
    double d, e;                                                               \
    NMms::TVectorType<P, NMms::TVectorType<P, int>> f, g;                      \
    NMms::TSetType<P, NMms::TVectorType<P, int>> h, i;                         \
    NMms::TMaybe<P, NMms::TVectorType<P, int>> j, k;                           \
    NMms::TStringType<P> l, m;                                                 \
    NMms::TMapType<P, NMms::TVectorType<P, int>, NMms::TSetType<P, int>> n, o; \
                                                                               \
    template <class Op>                                                        \
    void traverseFields(Op op) const Y_NO_SANITIZE("undefined") {              \
        op(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)(o);                       \
    }

template <class P>
struct TDuplicateStruct {
    DUPLICATE_STRUCT_ENTIRE
};

template <class P>
struct TDuplicateStructWithOldStyleFormatVersion {
    static mms::FormatVersion formatVersion() {
        return 21093;
    }
    DUPLICATE_STRUCT_ENTIRE
};

template <class P>
struct TDuplicateStructWithNewStyleFormatVersion {
    static mms::FormatVersion formatVersion(mms::Versions&) {
        return 21093;
    }
    DUPLICATE_STRUCT_ENTIRE
};

template <class P>
struct TDuplicateStructWithEnforceVersion {
    static mms::FormatVersion enforceMmsFormatVersion() {
        return 10000;
    }
};

Y_UNIT_TEST_SUITE(TMmsVersionTest) {
    Y_UNIT_TEST(Test) {
        typedef NMms::TStandalone S;

#ifdef __GNUC__ // version calculation partially relies on typeid(x).name() which is platform-dependent

        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<int>()),
                                 1269558899428530646ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<double>()),
                                 1269558897037139485ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<std::pair<int, double>>()),
                                 3769863283953809474ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<NMms::TVectorType<S, int>>()),
                                 5908932320964122251ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<NMms::TSetType<S, int>>()),
                                 7610443259246909065ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<NMms::TMapType<S, int, double>>()),
                                 3018027871432005466ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<NMms::TMaybe<S, int>>()),
                                 2964192378110927965ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<NMms::TStringType<S>>()),
                                 493519961992996882ull);

        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<
                                     NMms::TMaybe<S, NMms::TVectorType<S, int>>>()),
                                 9268741120498963650ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<
                                     NMms::TVectorType<S, NMms::TSetType<S, NMms::TVectorType<S, int>>>>()),
                                 1285385650391235107ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<TDuplicateStruct<S>>()),
                                 5848163006848953414ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<
                                     TDuplicateStructWithOldStyleFormatVersion<S>>()),
                                 5848173095171722083ull);
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<
                                     TDuplicateStructWithNewStyleFormatVersion<S>>()),
                                 5848173095171722083ull);
#endif
        UNIT_ASSERT_VALUES_EQUAL((mms::impl::formatVersion<
                                     TDuplicateStructWithEnforceVersion<S>>()),
                                 10000ull);
    }
}
