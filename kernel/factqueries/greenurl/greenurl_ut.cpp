#include "greenurl.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>


using namespace NFacts;

Y_UNIT_TEST_SUITE(TestSuite_FillUrlAndGreenUrl_Desktop)
{

Y_UNIT_TEST(Test_HttpsScheme)
{
    TStringBuf url = "https://yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_HttpScheme)
{
    TStringBuf url = "http://yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_MissedScheme)
{
    TStringBuf url = "yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = TString("http://") + url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_3dLevelDomain)
{
    TStringBuf url = "https://a.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "a.yandex-team.ru",
              "url": "https://a.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Www3dLevelDomain)
{
    TStringBuf url = "https://www.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://www.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Www4thLevelDomain)
{
    TStringBuf url = "https://www.a.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "a.yandex-team.ru",
              "url": "https://www.a.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Punycode)
{
    TStringBuf url = "http://xn--l1aej.xn--p1ai";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "мос.рф",
              "url": "http://xn--l1aej.xn--p1ai"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort80)
{
    TStringBuf url = "http://yandex-team.ru:80";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru:80"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort443)
{
    TStringBuf url = "https://yandex-team.ru:443";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:443"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_CustomPort)
{
    TStringBuf url = "https://yandex-team.ru:12345";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:12345"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPath)
{
    TStringBuf url = "https://yandex-team.ru/";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Path)
{
    TStringBuf url = "https://yandex-team.ru/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlug)
{
    TStringBuf url = "https://yandex-team.ru/path/";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path/"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort80WithPath)
{
    TStringBuf url = "http://yandex-team.ru:80/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "http://yandex-team.ru:80/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort443WithPath)
{
    TStringBuf url = "https://yandex-team.ru:443/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru:443/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_CustomPortWithPath)
{
    TStringBuf url = "https://yandex-team.ru:12345/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:12345"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru:12345/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Query)
{
    TStringBuf url = "https://yandex-team.ru?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlugAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path/?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path/?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlugAndQuery_SpecialCase)
{
    TStringBuf url = "https://yandex.ru/q/question/computers/kak_udalit_stranitsu_v_vkontakte_67112781/?utm_source=yandex&utm_medium=facts&answer_id=84efdc3b-f5ed-4839-b3ff-0e19310f35ee#84efdc3b-f5ed-4839-b3ff-0e19310f35ee";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex.ru",
              "url": "https://yandex.ru"
            },
            {
              "text": "kak udalit stranitsu v vkontakte 67112781",
              "url": "https://yandex.ru/q/question/computers/kak_udalit_stranitsu_v_vkontakte_67112781/?utm_source=yandex&utm_medium=facts&answer_id=84efdc3b-f5ed-4839-b3ff-0e19310f35ee#84efdc3b-f5ed-4839-b3ff-0e19310f35ee"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Fragment)
{
    TStringBuf url = "https://yandex-team.ru#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/path#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathQueryFragment)
{
    TStringBuf url = "https://yandex-team.ru/path?attr1=value1&attr2=value2#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "path",
              "url": "https://yandex-team.ru/path?attr1=value1&attr2=value2#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPath)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "resourse.html",
              "url": "https://yandex-team.ru/path/to/some/resourse.html"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "resourse.html",
              "url": "https://yandex-team.ru/path/to/some/resourse.html?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "resourse.html",
              "url": "https://yandex-team.ru/path/to/some/resourse.html#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PrettyPath)
{
    TStringBuf url = "https://yandex-team.ru/resourse_name_with_underscores";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "resourse name with underscores",
              "url": "https://yandex-team.ru/resourse_name_with_underscores"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "путь к ресурсу",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathWithSlashSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%2F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "путь к/ресурсу",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%2F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathWithQuestionSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%3F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "путь к?ресурсу",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%3F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedQueryWithSlashSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%2F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "путь к ресурсу",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%2F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedQueryWithQuestionSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%3F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            },
            {
              "text": "путь к ресурсу",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%3F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData);

    UNIT_ASSERT(serpData == expectedSerpData);
}

}


Y_UNIT_TEST_SUITE(TestSuite_FillUrlAndGreenUrl_Touch)
{

Y_UNIT_TEST(Test_HttpsScheme)
{
    TStringBuf url = "https://yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_HttpScheme)
{
    TStringBuf url = "http://yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_MissedScheme)
{
    TStringBuf url = "yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = TString("http://") + url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_3dLevelDomain)
{
    TStringBuf url = "https://a.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "a.yandex-team.ru",
              "url": "https://a.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Www3dLevelDomain)
{
    TStringBuf url = "https://www.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://www.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Www4thLevelDomain)
{
    TStringBuf url = "https://www.a.yandex-team.ru";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "a.yandex-team.ru",
              "url": "https://www.a.yandex-team.ru"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Punycode)
{
    TStringBuf url = "http://xn--l1aej.xn--p1ai";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "мос.рф",
              "url": "http://xn--l1aej.xn--p1ai"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort80)
{
    TStringBuf url = "http://yandex-team.ru:80";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru:80"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort443)
{
    TStringBuf url = "https://yandex-team.ru:443";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:443"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_CustomPort)
{
    TStringBuf url = "https://yandex-team.ru:12345";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:12345"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPath)
{
    TStringBuf url = "https://yandex-team.ru/";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Path)
{
    TStringBuf url = "https://yandex-team.ru/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlug)
{
    TStringBuf url = "https://yandex-team.ru/path/";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path/"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort80WithPath)
{
    TStringBuf url = "http://yandex-team.ru:80/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "http://yandex-team.ru:80/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_DefaultPort443WithPath)
{
    TStringBuf url = "https://yandex-team.ru:443/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:443/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_CustomPortWithPath)
{
    TStringBuf url = "https://yandex-team.ru:12345/path";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru:12345/path"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Query)
{
    TStringBuf url = "https://yandex-team.ru?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlugAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path/?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path/?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathWithEmptySlugAndQuery_SpecialCase)
{
    TStringBuf url = "https://yandex.ru/q/question/computers/kak_udalit_stranitsu_v_vkontakte_67112781/?utm_source=yandex&utm_medium=facts&answer_id=84efdc3b-f5ed-4839-b3ff-0e19310f35ee#84efdc3b-f5ed-4839-b3ff-0e19310f35ee";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex.ru",
              "url": "https://yandex.ru/q/question/computers/kak_udalit_stranitsu_v_vkontakte_67112781/?utm_source=yandex&utm_medium=facts&answer_id=84efdc3b-f5ed-4839-b3ff-0e19310f35ee#84efdc3b-f5ed-4839-b3ff-0e19310f35ee"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_Fragment)
{
    TStringBuf url = "https://yandex-team.ru#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_EmptyPathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/path#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PathQueryFragment)
{
    TStringBuf url = "https://yandex-team.ru/path?attr1=value1&attr2=value2#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path?attr1=value1&attr2=value2#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPath)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path/to/some/resourse.html"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html?attr1=value1&attr2=value2";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path/to/some/resourse.html?attr1=value1&attr2=value2"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_SegmentedPathAndFragment)
{
    TStringBuf url = "https://yandex-team.ru/path/to/some/resourse.html#fragment";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/path/to/some/resourse.html#fragment"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_PrettyPath)
{
    TStringBuf url = "https://yandex-team.ru/resourse_name_with_underscores";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/resourse_name_with_underscores"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathAndQuery)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathWithSlashSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%2F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%2F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedPathWithQuestionSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%3F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%3F%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%20%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedQueryWithSlashSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%2F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%2F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

Y_UNIT_TEST(Test_UrlEncodedQueryWithQuestionSign)
{
    TStringBuf url = "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%3F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0";

    NSc::TValue expectedSerpData = NSc::TValue::FromJsonThrow(R"SENTINEL(
      {
        "path": {
          "items": [
            {
              "text": "yandex-team.ru",
              "url": "https://yandex-team.ru/%D0%BF%D1%83%D1%82%D1%8C%20%D0%BA%20%D1%80%D0%B5%D1%81%D1%83%D1%80%D1%81%D1%83?attr=%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5%3F%D0%B0%D1%82%D1%80%D0%B8%D0%B1%D1%83%D1%82%D0%B0"
            }
          ]
        }
      }
    )SENTINEL");

    expectedSerpData["url"] = url;

    NSc::TValue serpData;
    FillUrlAndGreenUrl(url, serpData, /*touch*/ true);

    UNIT_ASSERT(serpData == expectedSerpData);
}

}
