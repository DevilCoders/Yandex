// test for check attribute filtering after doc had indexed.
#include <kernel/docindexer/idstorage.h>

#include <yweb/protos/indexeddoc.pb.h> // for TIndexedDoc

#include <kernel/groupattrs/config.h> // for NGroupingAttrs::TConfig
#include <kernel/indexer/faceproc/docattrs.h> // for TFullDocAttrs

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/util.h> // for Sprintf
#include <util/system/defaults.h> // for  ARRAY_SIZE

TFullDocAttrs::EAttrType testAttrsTypes[] = {
    TFullDocAttrs::AttrSearchLiteral,
    TFullDocAttrs::AttrSearchUrl,
    TFullDocAttrs::AttrSearchDate,
    TFullDocAttrs::AttrSearchInteger,
    TFullDocAttrs::AttrGrName,
    TFullDocAttrs::AttrGrInt,
    TFullDocAttrs::AttrArcText,
    TFullDocAttrs::AttrArcFull,
    TFullDocAttrs::AttrAuxPars,
    TFullDocAttrs::AttrErf,
    TFullDocAttrs::AttrSearchZones,
    TFullDocAttrs::AttrArcZones,
    TFullDocAttrs::AttrArcAttrs,
    TFullDocAttrs::AttrSearchLitPos,
    TFullDocAttrs::AttrSearchIntPos,
    TFullDocAttrs::AttrSearchLemma
};

Y_UNIT_TEST_SUITE(AttributeFilteringSuite) {
    Y_UNIT_TEST(SimpleTestAttributeFill) {
        TFullDocAttrs extAttrs;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testAttrsTypes); ++i)
            extAttrs.AddAttr(Sprintf("test_%016" PRIx64, i), "1", testAttrsTypes[i]);

        NRealTime::TIndexedDoc indexedDoc;
        TIndexedDocStorage::FillAttributes(nullptr, extAttrs, indexedDoc);

        size_t filteredAttrSize = indexedDoc.AttrsSize();

        // allatibutes until AttrSearchZones
        UNIT_ASSERT_EQUAL(filteredAttrSize, 10);

        for (size_t i = 0; i < indexedDoc.AttrsSize(); ++i) {
            const NRealTime::TIndexedDoc::TAttr& attr = indexedDoc.GetAttrs(i);
            UNIT_ASSERT_EQUAL(attr.GetType(), testAttrsTypes[i]);
        }
    }

    Y_UNIT_TEST(CombinedTypesAttributeFill) {
        TFullDocAttrs extAttrs;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testAttrsTypes); ++i)
            for (size_t j = i + 1; j < Y_ARRAY_SIZE(testAttrsTypes); ++j)
                extAttrs.AddAttr(
                    Sprintf("test_%" PRIx64 "_%" PRIx64, i, j),
                    /*attrValue = */ "1",
                    testAttrsTypes[i] | testAttrsTypes[j]
                );

        NRealTime::TIndexedDoc indexedDoc;
        TIndexedDocStorage::FillAttributes(nullptr, extAttrs, indexedDoc);

        for (size_t i = 0; i < indexedDoc.AttrsSize(); ++i) {
            const NRealTime::TIndexedDoc::TAttr& attr = indexedDoc.GetAttrs(i);
            UNIT_ASSERT_C(
                attr.GetType() <= ((ui32)TFullDocAttrs::AttrErf << 1) - 1,
                "Wrong atribute."
            );
        }
    }

    struct TAttrsRelevance {
        NGroupingAttrs::TConfig::Type IndexerType;
        NRealTime::TIndexedDoc::TAttr::TAttrType IndexedDocType;

        TAttrsRelevance()
        {}

        TAttrsRelevance(NGroupingAttrs::TConfig::Type indexerType, NRealTime::TIndexedDoc::TAttr::TAttrType indexedDocType)
            : IndexerType(indexerType)
            , IndexedDocType(indexedDocType)
        {}
    };

    typedef TMap<TString, TAttrsRelevance> TName2Type;

    void SetAttr(
        TFullDocAttrs& extAttrs,
        TName2Type& name2Type,
        NGroupingAttrs::TConfig* groupConfig,
        const TString& name,
        NGroupingAttrs::TConfig::Type indexerType,
        NRealTime::TIndexedDoc::TAttr::TAttrType indexedDocType)
    {
        extAttrs.AddAttr(name, /*attrValue = */ "1", TFullDocAttrs::AttrGrInt);
        if (groupConfig) {
            name2Type[name] = TAttrsRelevance(indexerType, indexedDocType);
            groupConfig->AddAttr(name.data(), indexerType);
        }
    }

    Y_UNIT_TEST(AttributeSizeFill) {
        NGroupingAttrs::TConfig groupAttrConfig;

        TName2Type Name2Type;

        TFullDocAttrs extAttrs;
        SetAttr(extAttrs, Name2Type, &groupAttrConfig, "test_1", NGroupingAttrs::TConfig::I16, NRealTime::TIndexedDoc::TAttr::I16);
        SetAttr(extAttrs, Name2Type, &groupAttrConfig, "test_2", NGroupingAttrs::TConfig::I32, NRealTime::TIndexedDoc::TAttr::I32);
        SetAttr(extAttrs, Name2Type, &groupAttrConfig, "test_3", NGroupingAttrs::TConfig::I64, NRealTime::TIndexedDoc::TAttr::I64);

        NRealTime::TIndexedDoc indexedDoc;
        TIndexedDocStorage::FillAttributes(&groupAttrConfig, extAttrs, indexedDoc);

        size_t filteredAttrSize = indexedDoc.AttrsSize();

        // allatibutes until AttrSearchZones
        UNIT_ASSERT_EQUAL(filteredAttrSize, static_cast<size_t>(std::distance(extAttrs.Begin(), extAttrs.End())));


        for (size_t i = 0; i < filteredAttrSize; ++i) {
            const NRealTime::TIndexedDoc::TAttr& attr = indexedDoc.GetAttrs(i);
            const TString& name = attr.GetName();
            NRealTime::TIndexedDoc::TAttr::TAttrType attrType = attr.GetSizeOfInt();
            UNIT_ASSERT_C(
                attrType == Name2Type[name].IndexedDocType,
                Sprintf("Attr does not correspond: attrName: %s, attrType: %d, must be type: %d", name.data(), (int)attrType, (int)Name2Type[name].IndexedDocType).data()
            );
        }

    }
}

