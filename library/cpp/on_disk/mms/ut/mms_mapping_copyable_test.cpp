#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/on_disk/mms/mapping.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <util/generic/typetraits.h>
#include <util/stream/file.h>
#include <util/system/tempfile.h>

namespace {
    struct TTestClass {
        int Value;
    };
}

Y_DECLARE_PODTYPE(TTestClass);

namespace {
    const TString mmappedFileName1("mmappedfile1");
    const TString mmappedFileName2("mmappedfile2");

    class TAncillaryFiles {
    public:
        TAncillaryFiles()
            : First(mmappedFileName1)
            , Second(mmappedFileName2)
        {
            MakeFile(First.Name(), 1);
            MakeFile(Second.Name(), 2);
        }

    private:
        void MakeFile(const TString& name, int value) {
            TOFStream fout(name);
            NMms::Write(fout, TTestClass{value});
            fout.Finish();
        }

    private:
        TTempFile First;
        TTempFile Second;
    };

    template <typename MappingClass>
    void MmsMappingCopyCtor() {
        TAncillaryFiles files;

        MappingClass mapping1(mmappedFileName1);
        MappingClass mapping2(std::move(mapping1));

        UNIT_ASSERT_VALUES_EQUAL(mapping2->Value, 1);
    }

    template <typename MappingClass>
    void MmsMappingCopyAssignment() {
        TAncillaryFiles files;

        MappingClass mapping1(mmappedFileName1), mapping2;
        mapping2 = std::move(mapping1);

        UNIT_ASSERT_VALUES_EQUAL(mapping2->Value, 1);

        mapping1.Reset(mmappedFileName2);
        UNIT_ASSERT_VALUES_EQUAL(mapping1->Value, 2);
        mapping2 = mapping1;
        UNIT_ASSERT_VALUES_EQUAL(mapping2->Value, 2);
    }

    template <typename MappingClass>
    void MmsMappingSwap() {
        TAncillaryFiles files;

        MappingClass mapping1(mmappedFileName1),
            mapping2(mmappedFileName2);

        UNIT_ASSERT_VALUES_EQUAL(mapping1->Value, 1);
        UNIT_ASSERT_VALUES_EQUAL(mapping2->Value, 2);

        DoSwap(mapping1, mapping2);

        UNIT_ASSERT_VALUES_EQUAL(mapping1->Value, 2);
        UNIT_ASSERT_VALUES_EQUAL(mapping2->Value, 1);
    }
}

Y_UNIT_TEST_SUITE(TMmsMappingMovableTest) {
    Y_UNIT_TEST(MmsMappingCopyCtor) {
        ::MmsMappingCopyCtor<NMms::TMapping<TTestClass>>();
    }

    Y_UNIT_TEST(MmsMappingCopyAssignment) {
        ::MmsMappingCopyAssignment<NMms::TMapping<TTestClass>>();
    }

    Y_UNIT_TEST(MmsMappingSwap) {
        ::MmsMappingSwap<NMms::TMapping<TTestClass>>();
    }
}
