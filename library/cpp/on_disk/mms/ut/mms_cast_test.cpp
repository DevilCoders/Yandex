#include "write_to_blob.h"

#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/type_traits.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

template <class P>
struct TUnversionedClass {
    typedef TUnversionedClass<NMms::TMmapped> MmappedType;

    TUnversionedClass() {
    }

    template <class A>
    void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
        a(i)(v)(b);
    }

    int i;
    NMms::TVectorType<P, int> v;
    bool b;
};

template <class P, size_t Version>
struct TVersionedClass {
    //Version is calculated by MmappedType
    typedef TVersionedClass<NMms::TMmapped, Version> MmappedType;

    TVersionedClass() {
    }

    //so format version is actual here
    static mms::FormatVersion formatVersion() {
        return Version;
    }

    template <class A>
    void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
        a(i)(v)(b);
    }

    int i;
    NMms::TVectorType<P, int> v;
    bool b;
};

//Derived class, methods of this class has another type, than TVersionedClass
template <size_t V>
struct TMutableVersionedClass: public TVersionedClass<NMms::TStandalone, V> {
};

template <class T>
TBlob MakeTest(T* object) {
    object->i = 7;
    object->v.push_back(1);
    object->v.push_back(5);
    object->v.push_back(4);
    object->v.push_back(3);
    object->b = true;

    return WriteToBlob(*object);
}

template <class S, class M>
void CheckMmapped(const S& standalone, const M& mmapped) {
    UNIT_ASSERT_VALUES_EQUAL(standalone.i, mmapped.i);
    UNIT_ASSERT_VALUES_EQUAL(standalone.v.size(), mmapped.v.size());
    UNIT_ASSERT(Equal(standalone.v.begin(), standalone.v.end(), mmapped.v.begin()));
    UNIT_ASSERT_VALUES_EQUAL(standalone.b, mmapped.b);
}

Y_UNIT_TEST_SUITE(TMmsCastTest) {
    Y_UNIT_TEST(UnversionedTest) {
        TUnversionedClass<NMms::TStandalone> object;
        const TBlob blob = MakeTest<TUnversionedClass<NMms::TStandalone>>(&object);

        TUnversionedClass<NMms::TMmapped> mmappedObject = Cast<TUnversionedClass<NMms::TMmapped>>(blob);

        CheckMmapped(object, mmappedObject);
    }

    Y_UNIT_TEST(VersionedTest) {
        TVersionedClass<NMms::TStandalone, 1> object;
        const TBlob blob = MakeTest(&object);

        UNIT_ASSERT_EXCEPTION((NMms::SafeCast<TVersionedClass<NMms::TMmapped, 1>>(
                                  blob.AsCharPtr(),
                                  sizeof(TVersionedClass<NMms::TMmapped, 1>) - 1)),
                              yexception);

        UNIT_ASSERT_NO_EXCEPTION((NMms::SafeCast<TVersionedClass<NMms::TMmapped, 1>>(
            blob.AsCharPtr(), blob.Size())));

        UNIT_ASSERT_EXCEPTION((NMms::SafeCast<TVersionedClass<NMms::TMmapped, 2>>(
                                  blob.AsCharPtr(), blob.Size())),
                              yexception);
        UNIT_ASSERT_EXCEPTION((NMms::SafeCast<TVersionedClass<NMms::TMmapped, 43>>(
                                  blob.AsCharPtr(), blob.Size())),
                              yexception);

        const TVersionedClass<NMms::TMmapped, 1> mmappedObject =
            NMms::SafeCast<TVersionedClass<NMms::TMmapped, 1>>(
                blob.AsCharPtr(), blob.Size());

        CheckMmapped(object, mmappedObject);
        CheckMmapped(object,
                     NMms::Cast<TVersionedClass<NMms::TMmapped, 1>>(
                         blob.AsCharPtr(), blob.Size()));
    }

    Y_UNIT_TEST(DerivedVersionedTest) {
        TMutableVersionedClass<1> object;
        const TBlob blob = MakeTest(&object);

        UNIT_ASSERT_NO_EXCEPTION((SafeCast<TVersionedClass<NMms::TMmapped, 1>>(blob)));
    }

    Y_UNIT_TEST(PrimitivesVersionTest) {
        {
            int x = 15;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_VALUES_EQUAL(SafeCast<int>(blob), x);
            UNIT_ASSERT_EXCEPTION(SafeCast<double>(blob), yexception);
        }
        {
            double x = 15.0;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_VALUES_EQUAL(SafeCast<double>(blob), 15.0);
            UNIT_ASSERT_EXCEPTION(SafeCast<int>(blob), yexception);
        }
    }

    Y_UNIT_TEST(ContainerVersionTest) {
        {
            typedef NMms::TVectorType<NMms::TStandalone, int> TVectorType;
            TVectorType x;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_NO_EXCEPTION(SafeCast<NMms::TMmappedType<TVectorType>::Type>(blob));

            UNIT_ASSERT_EXCEPTION((SafeCast<NMms::TVectorType<NMms::TMmapped, double>>(blob)), yexception);
            UNIT_ASSERT_EXCEPTION((SafeCast<NMms::TStringType<NMms::TMmapped>>(blob)), yexception);
        }
        {
            typedef NMms::TVectorType<NMms::TStandalone,
                                      TVersionedClass<NMms::TStandalone, 15>>
                TVectorType;
            TVectorType x;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_NO_EXCEPTION(SafeCast<NMms::TMmappedType<TVectorType>::Type>(blob));

            //Inner class in vector change its version, but typeid of vector
            //is not changed
            typedef NMms::TVectorType<NMms::TStandalone,
                                      TVersionedClass<NMms::TStandalone, 12>>
                TVectorAnotherVersion;
            UNIT_ASSERT_EXCEPTION((SafeCast<NMms::TMmappedType<TVectorAnotherVersion>::Type>(blob)), yexception);
        }
        {
            typedef NMms::TSetType<NMms::TStandalone,
                                   TVersionedClass<NMms::TStandalone, 15>>
                TVectorType;
            TVectorType x;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_NO_EXCEPTION(SafeCast<NMms::TMmappedType<TVectorType>::Type>(blob));

            //Inner class in vector change its version, but typeid of vector
            //is not changed
            typedef NMms::TSetType<NMms::TStandalone,
                                   TVersionedClass<NMms::TStandalone, 12>>
                TVectorAnotherVersion;
            UNIT_ASSERT_EXCEPTION((SafeCast<NMms::TMmappedType<TVectorAnotherVersion>::Type>(blob)), yexception);
        }
        {
            typedef NMms::TMapType<NMms::TStandalone, int,
                                   TVersionedClass<NMms::TStandalone, 15>>
                TVectorType;
            TVectorType x;
            const TBlob blob = SafeWriteToBlob(x);
            UNIT_ASSERT_NO_EXCEPTION(SafeCast<NMms::TMmappedType<TVectorType>::Type>(blob));

            //Inner class in vector change its version, but typeid of vector
            //is not changed
            typedef NMms::TMapType<NMms::TStandalone, int,
                                   TVersionedClass<NMms::TStandalone, 12>>
                TVectorAnotherVersion;
            UNIT_ASSERT_EXCEPTION((SafeCast<NMms::TMmappedType<TVectorAnotherVersion>::Type>(blob)), yexception);
        }
    }
}
