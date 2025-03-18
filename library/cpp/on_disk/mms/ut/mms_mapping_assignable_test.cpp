#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/writer.h>
#include <library/cpp/on_disk/mms/mapping.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/folder/tempdir.h>
#include <util/folder/path.h>

namespace {
    template <class StoragePolicy>
    class TObjectFirst {
    public:
        explicit TObjectFirst(
            const TString& firstString,
            const TString& secondString)
            : FirstString(firstString)
            , SecondString(secondString)
        {
        }

        const NMms::TStringType<StoragePolicy>& GetFirstString() const {
            return FirstString;
        }

        const NMms::TStringType<StoragePolicy>& GetSecondString() const {
            return SecondString;
        }

        template <class A>
        void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
            a(FirstString)(SecondString);
        }

    protected:
        NMms::TStringType<StoragePolicy> FirstString;
        NMms::TStringType<StoragePolicy> SecondString;
    };

    template <class StoragePolicy>
    class TObjectSecond: public TObjectFirst<StoragePolicy> {
    public:
        explicit TObjectSecond(
            const TString& firstString,
            const TString& secondString,
            const TString& thirdString,
            const TString& fourthString)
            : TObjectFirst<StoragePolicy>(firstString, secondString)
            , ThirdString(thirdString)
            , FourthString(fourthString)
        {
        }

        const NMms::TStringType<StoragePolicy>& GetThirdString() const {
            return ThirdString;
        }

        const NMms::TStringType<StoragePolicy>& GetFourthString() const {
            return FourthString;
        }

        template <class A>
        void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
            a(TObjectFirst<StoragePolicy>::FirstString)(TObjectFirst<StoragePolicy>::SecondString)(ThirdString)(FourthString);
        }

    protected:
        NMms::TStringType<StoragePolicy> ThirdString;
        NMms::TStringType<StoragePolicy> FourthString;
    };

    template <class StoragePolicy>
    class TObjectThird: public TObjectSecond<StoragePolicy> {
    public:
        explicit TObjectThird(
            const TString& firstString,
            const TString& secondString,
            const TString& thirdString,
            const TString& fourthString,
            const TString& fifthString,
            const TString& sixthString)
            : TObjectSecond<StoragePolicy>(firstString, secondString, thirdString, fourthString)
            , FifthString(fifthString)
            , SixthString(sixthString)
        {
        }

        const NMms::TStringType<StoragePolicy>& GetFifthString() const {
            return FifthString;
        }
        const NMms::TStringType<StoragePolicy>& GetSixthString() const {
            return SixthString;
        }

        template <class A>
        void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
            a(TObjectFirst<StoragePolicy>::FirstString)(TObjectFirst<StoragePolicy>::SecondString)(TObjectSecond<StoragePolicy>::ThirdString)(TObjectSecond<StoragePolicy>::FourthString)(FifthString)(SixthString);
        }

    protected:
        NMms::TStringType<StoragePolicy> FifthString;
        NMms::TStringType<StoragePolicy> SixthString;
    };

    const TString firstString("This is my first string");
    const TString secondString("This is my second string");
    const TString thirdString("This is my third string");
    const TString fourthString("This is my fourth string");
    const TString fifthString("This is my fifth string");
    const TString sixthString("This is my sixth string");

    class TAncillaryFiles {
    public:
        TAncillaryFiles() {
            TObjectFirst<NMms::TStandalone> firstObject(
                firstString, secondString);
            TObjectSecond<NMms::TStandalone> secondObject(
                firstString, secondString, thirdString, fourthString);
            TObjectThird<NMms::TStandalone> thirdObject(
                firstString, secondString, thirdString, fourthString, fifthString, sixthString);

            TOFStream firstOutput(FirstFileName());
            TOFStream secondOutput(SecondFileName());
            TOFStream thirdOutput(ThirdFileName());

            NMms::TWriter firstWriter(firstOutput);
            NMms::TWriter secondWriter(secondOutput);
            NMms::TWriter thirdWriter(thirdOutput);

            NMms::SafeWrite<TObjectFirst<NMms::TStandalone>>(firstWriter, firstObject);
            NMms::SafeWrite<TObjectSecond<NMms::TStandalone>>(secondWriter, secondObject);
            NMms::SafeWrite<TObjectThird<NMms::TStandalone>>(thirdWriter, thirdObject);
        }

        TString FirstFileName() const {
            return JoinFsPaths(Dir(), "first.mms.1");
        }
        TString SecondFileName() const {
            return JoinFsPaths(Dir(), "second.mms.1");
        }
        TString ThirdFileName() const {
            return JoinFsPaths(Dir(), "third.mms.1");
        }

    private:
        TTempDir Dir;
    };

}

Y_UNIT_TEST_SUITE(TMmsMappingAssignableTest) {
    Y_UNIT_TEST(MmsMappingAssignCtorTest) {
        TAncillaryFiles files;

        NMms::TMapping<TObjectFirst<NMms::TMmapped>> firstMapping;
        NMms::TMapping<TObjectSecond<NMms::TMmapped>> secondMapping;
        UNIT_ASSERT_EXCEPTION(firstMapping.Reset(files.SecondFileName()), std::exception);

        UNIT_ASSERT_NO_EXCEPTION(secondMapping.Reset(files.SecondFileName()));

        const TObjectSecond<NMms::TMmapped>& secondObject = *secondMapping;

        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetFirstString(), firstString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetSecondString(), secondString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetThirdString(), thirdString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetFourthString(), fourthString);

        UNIT_ASSERT_NO_EXCEPTION(firstMapping = secondMapping);
        const TObjectFirst<NMms::TMmapped>& firstObject = *firstMapping;
        UNIT_ASSERT_STRINGS_EQUAL(firstObject.GetFirstString(), firstString);
        UNIT_ASSERT_STRINGS_EQUAL(firstObject.GetSecondString(), secondString);
    }

    Y_UNIT_TEST(MmsMappingAssignOperatorTest) {
        TAncillaryFiles files;

        NMms::TMapping<TObjectFirst<NMms::TMmapped>> firstMapping;
        NMms::TMapping<TObjectSecond<NMms::TMmapped>> secondMapping;

        UNIT_ASSERT_EXCEPTION(firstMapping = NMms::TMapping<TObjectFirst<NMms::TMmapped>>(files.SecondFileName()), std::exception);

        UNIT_ASSERT_NO_EXCEPTION(secondMapping = NMms::TMapping<TObjectSecond<NMms::TMmapped>>(files.SecondFileName()));

        const TObjectSecond<NMms::TMmapped>& secondObject = *secondMapping;
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetFirstString(), firstString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetSecondString(), secondString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetThirdString(), thirdString);
        UNIT_ASSERT_STRINGS_EQUAL(secondObject.GetFourthString(), fourthString);

        UNIT_ASSERT_NO_EXCEPTION(firstMapping = secondMapping);
        const TObjectFirst<NMms::TMmapped>& firstObject = *firstMapping;
        UNIT_ASSERT_STRINGS_EQUAL(firstObject.GetFirstString(), firstString);
        UNIT_ASSERT_STRINGS_EQUAL(firstObject.GetSecondString(), secondString);
    }

    Y_UNIT_TEST(MmsMappingDoubleAssignTest) {
        TAncillaryFiles files;

        NMms::TMapping<TObjectThird<NMms::TMmapped>> thirdMapping(files.ThirdFileName());
        NMms::TMapping<TObjectSecond<NMms::TMmapped>> secondMapping(thirdMapping);
        NMms::TMapping<TObjectFirst<NMms::TMmapped>> firstMapping(secondMapping);

        const TObjectFirst<NMms::TMmapped>* firstObject = nullptr;
        UNIT_ASSERT_NO_EXCEPTION(firstObject = firstMapping.Get());
        UNIT_ASSERT_STRINGS_EQUAL(firstObject->GetFirstString(), firstString);
        UNIT_ASSERT_STRINGS_EQUAL(firstObject->GetSecondString(), secondString);

        UNIT_ASSERT_NO_EXCEPTION(firstMapping = secondMapping);
    }
}
