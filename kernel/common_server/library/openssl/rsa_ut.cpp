#include "rsa.h"
#include "aes.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/file.h>
#include <util/string/hex.h>

using namespace NOpenssl;

namespace {
    void InitLog() {
        if (!GlobalLogInitialized())
            DoInitGlobalLog("cout", 7, false, false);
    }
}

Y_UNIT_TEST_SUITE(OpenSSLRSATestSuite) {
    Y_UNIT_TEST(AES) {
        TString AESKey = HexDecode("47BCE5C74F589F4867DBD57E9CA9F80820EC512F5551A1A790C6A77479AF6033");
        INFO_LOG << HexEncode(AESKey) << Endl;
        INFO_LOG << Base64Encode(AESKey) << Endl;
        TString message = R"({"sf":[{"name":"car_registrator","public_icon":"https://carsharing.s3.yandex.net/drive/static/tag-icons/v4/registrator.png","index":78,"comment":"Регистратор в машине","is_important":false,"display_name":"Регистратор в машине"}],"user":{"credit_card":{"pan":{"suffix":"0000","prefix":"0"},"paymethod_id":""},"details":{"username":"sofya.nv","status":"active","last_name":"Новожилова","first_name":"Софья","preliminary_payments":{"enabled":false,"amount":0},"setup":{"phone":{"verified":true,"number":"+79265398739"},"email":{"verified":true,"address":"a@b.ru"}},"pn":""},"settings":[{"name":"Отключить кросс-логин","value":"0","type":"bool","id":"disable_cross_login"}],"user_id":"b468399a-d09b-4438-aa12-edbb4a5f55d6","billing":{"special_accounts":[{"name":"Бонусы","icon":"","balance":9299,"id":"bonus"},{"name":"Зарплатный кошелек заправщика","icon":"","balance":10000000,"id":"fueling-salary"},{"name":"Тройка","icon":"","balance":98900,"id":"troika"}],"bonuses":{"amount":9299},"accounts":[{"name":"За свои","icon":"","balance":0,"id":"card"},{"name":"Драйв","icon":"","balance":-134812,"id":"y.drive"}]},"plus":{"is_activated":true},"is_first_riding":false},"flags":{"fakefake":"veryfake","enable_cl_switcher":true,"enable_chat":true,"button_width":42,"enable_transportation":true,"enable_troika":true,"enable_modern_menu":true,"enable_new_support_chat":true,"enable_promo":true,"fueling_delivery_enabled":true,"old_design":null,"new_design":true},"fueling_ability":false,"feedback":{"set_id":"feedback_reservation"},"device_diff":{"start":{"timestamp":1567940635,"longitude":37.76142883,"latitude":55.70595169,"course":5,"type":"GPSCurrent","since":1567915765,"precision":200},"finish":{"timestamp":1567940645,"longitude":37.76142883,"latitude":55.70595169,"course":5,"type":"GPSCurrent","since":1567915765,"precision":200},"mileage":0,"hr_mileage":"0 м"},"device_status":"Verified","property_patches":[],"models":{"porsche_carrera":{"image_map_url_2x":"https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche911-Carrera4S-red-top-2@2x.png","name":"Porsche 911 Carrera 4S","code":"porsche_carrera","manufacturer":"Porsche","image_angle_url":"https://carsharing.s3.yandex.net/drive/car-models/Porsche911-Carrera4S-red-3-4.png","cars_count":1,"image_map_url_3x":"https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche911-Carrera4S-red-top-2@3x.png","registry_manufacturer":"","short_name":"911","image_large_url":" https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche911-Carrera4S-red-side_large.png","visual":{"background":{"gradient":{"bottom_color":"#232323","top_color":"#232323"}},"title":{"color":"#ffffff"}},"image_pin_url_3x":"https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche-map-pin-new-2@3x.png","image_small_url":"https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche911-Carrera4S-red-side_small.png","specifications":[{"name":"Коробка","position":0,"value":"автомат","id":"67b55588-82fc-444e-9e1e-caa74ad7947d"},{"name":"Привод","position":1000000,"value":"полный","id":"a1048bd8-1ec0-422a-b49c-726dadf7d162"},{"name":"Мощность","position":2000000,"value":"420 л.с.","id":"b72e27f8-f294-4306-9815-25487815d118"},{"name":"Разгон","position":3000000,"value":"4,2 с","id":"065d584e-a87c-410b-929b-e8a8eab34d1a"},{"name":"Мест","position":4000000,"value":"1+1","id":"7a105b9d-81db-40ae-9e1e-72780e76009b"},{"name":"Бак","position":5000000,"value":"68 л","id":"e84def26-fa15-41e9-b77e-493257099909"}],"fuel_type":"98","fuel_cap_side":"right","image_pin_url_2x":"https://carsharing.s3.yandex.net/drive/car-models/Porsche/Porsche-map-pin-new-2@2x.png","registry_model":"","fuel_icon_url":"https://carsharing.s3.yandex.net/fuel_finger.png"}},"segment":{"session":{"specials":{"current_offer_state":null,"current_offer":{"name":"Минутки","switchable":true,"device_id":"8322****7D20","group_name":"","from_scanner":false,"visual":{"bottom_color":"#DC143C","top_color":"#DC143C"},"localizations":{},"short_description":["Ожидание — 0,45 ₽/мин","20 мин бесплатного ожидания"],"constructor_id":"minutki_test","discounts":[{"discount":0.05,"title":"Скидка Promo","icon":"https://carsharing.s3.yandex.net/drive/discounts/icons/discount-staff__28-11.png","id":"test_discount_promo","details":[],"visible":true,"description":"русский текст staff_discount_description","small_icon":"https://carsharing.s3.yandex.net/drive/discounts/icons/discount-staff__28-11.png"},{"discount":0.05,"title":"Скидка","icon":"http://carsharing.s3.yandex.net/drive/discounts/icons/plus.png","id":"plus_discount_base","details":{"old_state_parking":{"tag_name":"old_state_parking","additional_duration":0}},"visible":true,"description":"русский текст yandex_plus_discount_description","small_icon":"http://carsharing.s3.yandex.net/drive/discounts/icons/plus.png"},{"discount":0.5,"title":"Тест скидки","icon":"https://carsharing.s3.yandex.net/drive/discounts/icons/discount-staff__28-11.png","id":"test_discount","details":[],"visible":true,"description":"Это просто скидка","small_icon":"https://carsharing.s3.yandex.net/drive/discounts/icons/discount-staff__28-11.png"}],"parent_id":"","offer_id":"99f88a92-a1f42515-c7c3f38f-a45ec2c5","wallets":[{"id":"bonus"},{"selected":true,"id":"card"},{"id":"y.drive"}],"debt":{"threshold":2000},"type":"standart_offer","list_priority":0,"detailed_description":"","summary_discount":{"discount":0.6,"visible":true,"details":{"old_state_parking":{"tag_name":"old_state_parking","additional_duration":0},"old_state_reservation":{"tag_name":"old_state_reservation","additional_duration":1200}},"title":"","id":""},"is_corp_session":false,"prices":{"discount":{"discount":0.6,"visible":true,"details":{"old_state_parking":{"tag_name":"old_state_parking","additional_duration":0},"old_state_reservation":{"tag_name":"old_state_reservation","additional_duration":1200}},"title":"","id":""},"use_deposit":false,"parking_discounted":45,"deposit":0,"riding_discounted":267,"riding":666,"parking":111},"deadline":1567940914},"free_time":1189,"bill":[{"type":"total","title":"Итого","cost":0}],"total_price":0,"is_cancelled":true,"total_price_hr":"0₽","total_duration":11,"durations_by_tags":{"old_state_reservation":11}}},"meta":{"start":1567940638,"corrupted":false,"session_id":"99f88a92-a1f42515-c7c3f38f-a45ec2c5","user_id":"b468399a-d09b-4438-aa12-edbb4a5f55d6","object_id":"46776b2f-80bb-3874-fa14-70c9000001af","finished":true,"inconsistency":false,"finish":1567940649}},"area_common_user_info":{"support_phone":"8 (495) 410-68-22","phone":"+7 (495) 410-68-22","support_whatsapp":"+7 (925) 411-17-65","support_telegram":"YandexDriveSupportBot","support_feedback_form":"https://yandex.ru/support/drive/feedback.html"},"server_time":1567949049,"car":{"number":"а795вс799","model_id":"porsche_carrera","id":"46776b2f-80bb-3874-fa14-70c9000001af","sf":[78]}})";
        TString encoded;
//        TString message = "Test\n";
        UNIT_ASSERT(AESEncrypt(AESKey, message, encoded));
        INFO_LOG << Base64Encode(encoded) << Endl;
        TString decoded;
        UNIT_ASSERT(AESDecrypt(AESKey, encoded, decoded));
        UNIT_ASSERT_VALUES_EQUAL(message, decoded);
    }

    Y_UNIT_TEST(RSASign) {
        InitLog();
        TString publicKeyStr = "-----BEGIN PUBLIC KEY-----\n"\
            "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy8Dbv8prpJ/0kKhlGeJY\n"\
            "ozo2t60EG8L0561g13R29LvMR5hyvGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+\n"\
            "vw1HocOAZtWK0z3r26uA8kQYOKX9Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQAp\n"\
            "fc9jB9nTzphOgM4JiEYvlV8FLhg9yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68\n"\
            "i6T4nNq7NWC+UNVjQHxNQMQMzU6lWCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoV\n"\
            "PpY72+eVthKzpMeyHkBn7ciumk5qgLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUy\n"\
            "wQIDAQAB\n"\
            "-----END PUBLIC KEY-----\n";
        TString privateKeyStr = "-----BEGIN RSA PRIVATE KEY-----\n"\
            "MIIEowIBAAKCAQEAy8Dbv8prpJ/0kKhlGeJYozo2t60EG8L0561g13R29LvMR5hy\n"\
            "vGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+vw1HocOAZtWK0z3r26uA8kQYOKX9\n"\
            "Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQApfc9jB9nTzphOgM4JiEYvlV8FLhg9\n"\
            "yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68i6T4nNq7NWC+UNVjQHxNQMQMzU6l\n"\
            "WCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoVPpY72+eVthKzpMeyHkBn7ciumk5q\n"\
            "gLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUywQIDAQABAoIBADhg1u1Mv1hAAlX8\n"\
            "omz1Gn2f4AAW2aos2cM5UDCNw1SYmj+9SRIkaxjRsE/C4o9sw1oxrg1/z6kajV0e\n"\
            "N/t008FdlVKHXAIYWF93JMoVvIpMmT8jft6AN/y3NMpivgt2inmmEJZYNioFJKZG\n"\
            "X+/vKYvsVISZm2fw8NfnKvAQK55yu+GRWBZGOeS9K+LbYvOwcrjKhHz66m4bedKd\n"\
            "gVAix6NE5iwmjNXktSQlJMCjbtdNXg/xo1/G4kG2p/MO1HLcKfe1N5FgBiXj3Qjl\n"\
            "vgvjJZkh1as2KTgaPOBqZaP03738VnYg23ISyvfT/teArVGtxrmFP7939EvJFKpF\n"\
            "1wTxuDkCgYEA7t0DR37zt+dEJy+5vm7zSmN97VenwQJFWMiulkHGa0yU3lLasxxu\n"\
            "m0oUtndIjenIvSx6t3Y+agK2F3EPbb0AZ5wZ1p1IXs4vktgeQwSSBdqcM8LZFDvZ\n"\
            "uPboQnJoRdIkd62XnP5ekIEIBAfOp8v2wFpSfE7nNH2u4CpAXNSF9HsCgYEA2l8D\n"\
            "JrDE5m9Kkn+J4l+AdGfeBL1igPF3DnuPoV67BpgiaAgI4h25UJzXiDKKoa706S0D\n"\
            "4XB74zOLX11MaGPMIdhlG+SgeQfNoC5lE4ZWXNyESJH1SVgRGT9nBC2vtL6bxCVV\n"\
            "WBkTeC5D6c/QXcai6yw6OYyNNdp0uznKURe1xvMCgYBVYYcEjWqMuAvyferFGV+5\n"\
            "nWqr5gM+yJMFM2bEqupD/HHSLoeiMm2O8KIKvwSeRYzNohKTdZ7FwgZYxr8fGMoG\n"\
            "PxQ1VK9DxCvZL4tRpVaU5Rmknud9hg9DQG6xIbgIDR+f79sb8QjYWmcFGc1SyWOA\n"\
            "SkjlykZ2yt4xnqi3BfiD9QKBgGqLgRYXmXp1QoVIBRaWUi55nzHg1XbkWZqPXvz1\n"\
            "I3uMLv1jLjJlHk3euKqTPmC05HoApKwSHeA0/gOBmg404xyAYJTDcCidTg6hlF96\n"\
            "ZBja3xApZuxqM62F6dV4FQqzFX0WWhWp5n301N33r0qR6FumMKJzmVJ1TA8tmzEF\n"\
            "yINRAoGBAJqioYs8rK6eXzA8ywYLjqTLu/yQSLBn/4ta36K8DyCoLNlNxSuox+A5\n"\
            "w6z2vEfRVQDq4Hm4vBzjdi3QfYLNkTiTqLcvgWZ+eX44ogXtdTDO7c+GeMKWz4XX\n"\
            "uJSUVL5+CVjKLjZEJ6Qc2WZLl94xSwL71E41H4YciVnSCQxVc4Jw\n"\
            "-----END RSA PRIVATE KEY-----\n";

        TRSAPublicKey publicK1;
        publicK1.Init(publicKeyStr);

        TRSAPtr rsa = GenerateRSA();

        TRSAPublicKey publicK2(rsa);
        {
            TString encoded;
            TRSAPrivateKey privateK(rsa);
            privateK.Sign("test", encoded);
            UNIT_ASSERT(publicK2.Verify("test", encoded));
            UNIT_ASSERT(!publicK2.Verify("testtest", encoded));
        }
        {
            TString encoded;
            TRSAPrivateKey privateK;
            privateK.Init(privateKeyStr);
            privateK.Sign("test", encoded);
            UNIT_ASSERT(publicK1.Verify("test", encoded));
            UNIT_ASSERT(!publicK1.Verify("testtest", encoded));
            UNIT_ASSERT(!publicK2.Verify("test", encoded));
        }
    };
}
