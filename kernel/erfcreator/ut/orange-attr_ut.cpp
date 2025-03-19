#include <kernel/erfcreator/orangeattrs.h>

#include <kernel/indexer/faceproc/docattrs.h> //for TFullDocAttrs
#include <kernel/tarc/iface/tarcface.h> // for ARCHIVE_FIELD_VALUE_LIST_SEP
#include <yweb/protos/docfactors.pb.h> // for TOrangeDocFactors

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/printf.h> // for Sprintf

Y_UNIT_TEST_SUITE(OrangeAttrTestSuite) {
    using namespace NRealTime;

    const TFullDocAttrs::TAttr* GetAttr(const TFullDocAttrs& attrs, const TString& attrName) {
        for (TFullDocAttrs::TConstIterator i = attrs.Begin(); i != attrs.End(); ++i) {
            if (i->Name == attrName)
                return &(*i);
        }
        return nullptr;
    }

    void CompareAttrs(TFullDocAttrs docAttrs, const TString& attrName, const TString& compareValue) {
        const TFullDocAttrs::TAttr* docAttr = nullptr;

        UNIT_ASSERT_C(docAttr = GetAttr(docAttrs, attrName), Sprintf("Can't find attribute with name \"%s\"", attrName.data()).data());
        UNIT_ASSERT_EQUAL_C(
            docAttr->Value,
            compareValue,
            Sprintf("Attribute with name \"%s\" (value: \"%s\") not equal with test value \"%s\"",
                attrName.data(),
                docAttr->Value.data(),
                compareValue.data()).data()
        );
    }

    Y_UNIT_TEST(ParseOrangeAttributes) {
        TAnchorText anchorText;
        NOrangeData::TAttribute* attr = anchorText.AddSearchAttribute();
        attr->SetName("orange");
        attr->SetValue("time=123456;docTate=12.14;CrawlRank=124");

        // without semicolon at the end
        attr = anchorText.AddSearchAttribute();
        attr->SetName("name2");
        attr->SetValue("value2");

        // with semicolon at the end
        attr = anchorText.AddSearchAttribute();
        attr->SetName("name3");
        attr->SetValue("value3;");

        TFullDocAttrs docAttrs;
        AddOrangeAttrFromSearchAttrs(anchorText, &docAttrs);

        CompareAttrs(docAttrs, "orange", TString("time=123456") + ARCHIVE_FIELD_VALUE_LIST_SEP +"docTate=12.14" + ARCHIVE_FIELD_VALUE_LIST_SEP + "CrawlRank=124");
        CompareAttrs(docAttrs, "name2", "value2");
        CompareAttrs(docAttrs, "name3", TString("value3") + ARCHIVE_FIELD_VALUE_LIST_SEP);

    }

    Y_UNIT_TEST(AttrsWithoutValueTest) {
        TAnchorText anchorText;
        NOrangeData::TAttribute* attr = anchorText.AddSearchAttribute();
        attr->SetName("name");
        attr->SetValue("");

        TFullDocAttrs docAttrs;
        AddOrangeAttrFromSearchAttrs(anchorText, &docAttrs);

        CompareAttrs(docAttrs, "name", TString(""));
    }
}
