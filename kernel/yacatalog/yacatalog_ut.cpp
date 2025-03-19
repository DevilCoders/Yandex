#include "yacatalog.h"
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/tempfile.h>
#include <util/stream/file.h>


Y_UNIT_TEST_SUITE(TYaCatalogTest) {
    Y_UNIT_TEST(DummyTest) {
        TString data = "<?xml version='1.0' standalone='yes'?> <YaCatalog> <cat>"
    "<cat hide=\"0\" id=\"0\" name=\"Каталог\" parent_id=\"\" rcat=\"-1\" size=\"126894\" type=\"0\" url_name=\"cat\" />"
    "<cat hide=\"0\" id=\"5\" name=\"Интернет\" parent_id=\"4\" rcat=\"-1\" size=\"4462\" type=\"0\" url_name=\"Internet\" />"
    "<cat hide=\"0\" id=\"311\" name=\"Мобильная связь\" parent_id=\"4\" rcat=\"-1\" size=\"3094\" type=\"0\" url_name=\"Mobile\" />"
    "<cat hide=\"0\" id=\"2\" name=\"Учёба\" parent_id=\"0\" rcat=\"-1\" size=\"10254\" type=\"0\" url_name=\"Science\" />"
    "<cat hide=\"0\" id=\"4\" name=\"Hi-Tech\" parent_id=\"0\" rcat=\"-1\" size=\"12527\" type=\"0\" url_name=\"Computers\" />"
    "<cat hide=\"0\" id=\"24\" name=\"Интернет-кафе\" parent_id=\"5\" rcat=\"-1\" size=\"22\" type=\"0\" url_name=\"Internet-Cafes\" />"
    "<cat hide=\"0\" id=\"54\" name=\"Доступ в интернет\" parent_id=\"5\" rcat=\"-1\" size=\"946\" type=\"0\" url_name=\"Providing\" />"
    "<cat hide=\"0\" id=\"90\" name=\"Спорт\" parent_id=\"0\" rcat=\"-1\" size=\"6585\" type=\"0\" url_name=\"Sports\" />"
    "<cat hide=\"0\" id=\"8\" name=\"Развлечения\" parent_id=\"0\" rcat=\"-1\" size=\"5832\" type=\"0\" url_name=\"Entertainment\" />"
    "<cat hide=\"1\" id=\"124\" name=\"Целительство\" parent_id=\"12\" rcat=\"-1\" size=\"2\" type=\"0\" url_name=\"Healing\" />"
    "<cat hide=\"0\" id=\"152\" name=\"НЛО\" parent_id=\"12\" rcat=\"-1\" size=\"16\" type=\"0\" url_name=\"UFO\" />"
    "<cat hide=\"0\" id=\"12\" name=\"Непознанное\" parent_id=\"8\" rcat=\"-1\" size=\"401\" type=\"0\" url_name=\"Occultism\" />"
    "</cat></YaCatalog>";


        TString xmlFileName = MakeTempName();
        TUnbufferedFileOutput xmlFile(xmlFileName);
        xmlFile.Write(data);
        xmlFile.Flush();


        TCatalogParser catalog(xmlFileName);


        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(2).Parent, 0);
        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(2).Name, "Учёба");
        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(2).Hide, 0);
        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(2).UrlName, "Science");
        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(2).Type, 0);

        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(311).Parent, 4);
        UNIT_ASSERT_EQUAL(catalog.GetCategoryDescr(5).Parent, 4);

        {
            TCatalogCategory arr[] =  {5, 4, 0};
            UNIT_ASSERT_EQUAL(catalog.GetCategoryPath(5), TVector<TCatalogCategory>(arr, arr + sizeof(arr) / sizeof(arr[0])));
        }

        UNIT_ASSERT_EQUAL(catalog.SelectGeneral("5,311,2"), 4);
        UNIT_ASSERT_EQUAL(catalog.SelectGeneral("24,54"), 5);
        UNIT_ASSERT_EQUAL(catalog.SelectGeneral("24,90"), 90);
        UNIT_ASSERT_EQUAL(catalog.SelectGeneral("24,54,124,152"), 8);


    }
}

