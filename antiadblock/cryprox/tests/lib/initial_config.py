INITIAL_CONFIG = r"""{
  "autoru":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [],
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPT_BODY_RE": {
          "\\bgwd-ad\\b": null
        },
        "CRYPT_RELATIVE_URL_RE": [
          "/[a-z]+?/sale/iframe/[\\w/]+?\\?.+?"
        ],
        "CRYPT_SECRET_KEY": "shae7UoRe7Aex9ahthai3Kaig7oo4juP",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/",
        "CRYPT_URL_RE": [
          "avatars\\.mds\\.yandex\\.net/get\\-(?:auto|autoru|autoru-all|autoparts|autoru-office)/.*?"
        ],
        "BYPASS_URL_RE": [],
        "DETECT_LINKS": [
          {
            "src": "https://an.yandex.ru/resource/context_static_r_4061.js",
            "type": "get"
          }
        ],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [
          "los"
        ],
        "EXTUID_COOKIE_NAMES": [
          "uid"
        ],
        "EXTUID_TAG": "partner_tag_name",
        "FOLLOW_REDIRECT_URL_RE": [],
        "INTERNAL": true,
        "PARTNER_TOKENS": [
          "test_token1"
        ],
        "PROXY_URL_RE": [
          "(?:[\\w\\-]*\\.)*auto\\.ru/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active", "test"],
      "version": "test"
    },
  "test_local":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [
          "/accel-images/\\w+\\.png$"
        ],
        "ADFOX_DEBUG": true,
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPT_BODY_RE": {"\\bsmi2adblock[-\\w]+?\\b": null},
        "REPLACE_BODY_RE": {"(Ya)\\[\\d+\\]\\(": "Ya "},
        "CRYPT_ENABLE_TRAILING_SLASH": true,
        "CRYPT_RELATIVE_URL_RE": [
          "/static/[\\w/.-]+\\.(?:js|css|gif|png)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?",
          "/scripts/.*?",
          "(?:\\.{0,2}/)*testing/.*?"
        ],
        "CRYPT_SECRET_KEY": "duoYujaikieng9airah4Aexai4yek4qu",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/",
        "CRYPT_URL_OLD_PREFFIXES": ["/testprefix/", "/test_prefix_2/"],
        "CRYPT_URL_RE": [
          "(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?"
        ],
        "BYPASS_URL_RE": [],
        "DETECT_CUSTOM": [
          {
            "div": {
              "class": "ring top",
              "div": {
                "class": "inner__banner"
              }
            }
          }
        ],
        "DETECT_ELEMS": {
          "id": [
            "mlph-banner",
            "popup_box"
          ]
        },
        "DETECT_LINKS": [
          {
            "src": "http://test.local/some.js",
            "type": "get"
          }
        ],
        "DETECT_HTML":[
          "<div class=\"banner\"><span>ok</span></div>"
        ],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [
          "test_cookie"
        ],
        "EXTUID_COOKIE_NAMES": [
          "extuid",
          "second-ext-uid"
        ],
        "EXTUID_TAG": "test_extuid_tag",
        "FOLLOW_REDIRECT_URL_RE": [
          "(?:https?://)?(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?"
        ],
        "PARTNER_TOKENS": [
          "test_token2"
        ],
        "PROXY_URL_RE": [
          "test\\.local/.*",
          "this-is-a-fake-address\\.com(?::\\d+)?/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active"],
      "version": "test"
    },
  "yandex_mail":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [],
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPT_BODY_RE": {
          "[\\\"\\'.](?:b-)?(banner)\\b": null,
          "\\bjs-(?:main-rtb(?:-2)?|messages-direct)\\b": null,
          "\\bmail-DirectLine(?:-Content|_setup|Container)?\\b": null,
          "\\bmessages-direct\\b": null,
          "\\byap-(?:adtune__image|layout|logo-block__text|title-block__text|body-text)\\b": null
        },
        "CRYPT_SECRET_KEY": "Dee7chahjooz4eiwaequaifie9ooghei",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/u2709/",
        "CRYPT_URL_RE": [
          "(?:[\\w-]+\\.)?mailfront\\d+\\.yandex(?:-team)?\\.(?:\\w+|com\\.tr)/[\\w/.-]+\\.(?:js|css)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?",
          "ub(?:corp)?\\d+-qa\\.yandex\\.(?:\\w+|com\\.tr)/[\\w/.-]+\\.(?:js|css)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?"
        ],
        "BYPASS_URL_RE": [],
        "DETECT_ELEMS": {
          "class": []
        },
        "DETECT_LINKS": [
          {
            "src": "https://yastatic.net/daas/atom.js",
            "type": "get"
          }
        ],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [],
        "EXTUID_COOKIE_NAMES": [
          "yandexuid"
        ],
        "EXTUID_TAG": "yandex-mail",
        "FOLLOW_REDIRECT_URL_RE": [],
        "INTERNAL": true,
        "PARTNER_TOKENS": [
          "test_token3"
        ],
        "PROXY_URL_RE": [
          "(?:[\\w-]+\\.)?mailfront\\d+\\.yandex(?:-team)?\\.(?:\\w+|com\\.tr)/.*",
          "ub(?:corp)?\\d+-qa\\.yandex\\.(?:\\w+|com\\.tr)/.*",
          "mail\\.yandex\\.(?:\\w+|com\\.tr)/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active"],
      "version": "test"
    },
  "yandex_morda":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [],
        "ADB_ENABLED": false,
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPTED_HOST": "yastatic.net",
        "CRYPT_BODY_RE": {
          "\\bhtml5_container\\b": null,
          "\\bhtml5_enable:  \".*\\b": "html5_enable: 0"
        },
        "CRYPT_SECRET_KEY": "uafiengei7ioThahv9poong7Nibingoh",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/www/_/",
        "CRYPT_URL_RE": [
          "(?:ar\\.)?tns-counter\\.ru/.*?",
          "secure-it\\.imrworldwide\\.com/.*?"
        ],
        "BYPASS_URL_RE": [],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [],
        "EXTUID_COOKIE_NAMES": [
          "yandexuid"
        ],
        "EXTUID_TAG": "yandex-morda",
        "FOLLOW_REDIRECT_URL_RE": [],
        "INTERNAL": true,
        "PARTNER_TOKENS": [
          "test_token4"
        ],
        "PROXY_URL_RE": [
          "(?:www\\.)?ya(?:ndex|static)\\.(?:ru|com|net|ua|kz|by)/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active"],
      "version": "test"
    },
  "autoredirect.turbo":
    {
      "config": {
        "CRYPT_SECRET_KEY": "duoYujaikieng9airah4Aexai4yek4qu",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "ENCRYPTION_STEPS": [
          0
        ],
        "EXCLUDE_COOKIE_FORWARD": [
          "test_cookie"
        ],
        "EXTUID_COOKIE_NAMES": [
          "extuid",
          "second-ext-uid"
        ],
        "EXTUID_TAG": "turbo_extuid_tag",
        "PARTNER_TOKENS": [
          "test_token5"
        ],
        "PROXY_URL_RE": [
          "aabturbo\\.gq/.*"
        ],
        "CRYPT_URL_RE": [
          "(?:\\w+\\.)*aabturbo\\.gq/.*?"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi",
        "WEBMASTER_DATA": {
            "aabturbo-gq": "aabturbo.gq"
        }
      },
      "statuses": ["active"],
      "version": "test"
    },
  "test_local_2::active::None::None":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [
          "/accel-images/\\w+\\.png$"
        ],
        "ADFOX_DEBUG": true,
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPT_BODY_RE": {"\\bsmi2adblock[-\\w]+?\\b": null},
        "REPLACE_BODY_RE": {"(Ya)\\[\\d+\\]\\(": "Ya "},
        "CRYPT_ENABLE_TRAILING_SLASH": true,
        "CRYPT_RELATIVE_URL_RE": [
          "/static/[\\w/.-]+\\.(?:js|css|gif|png)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?",
          "/scripts/.*?",
          "(?:\\.{0,2}/)*testing/.*?"
        ],
        "CRYPT_SECRET_KEY": "duoYujaikieng9airah4Aexai4yek4qu",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/",
        "CRYPT_URL_OLD_PREFFIXES": ["/testprefix/", "/test_prefix_2/"],
        "CRYPT_URL_RE": [
          "(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?"
        ],
        "BYPASS_URL_RE": [],
        "DETECT_CUSTOM": [
          {
            "div": {
              "class": "ring top",
              "div": {
                "class": "inner__banner"
              }
            }
          }
        ],
        "DETECT_ELEMS": {
          "id": [
            "mlph-banner",
            "popup_box"
          ]
        },
        "DETECT_LINKS": [
          {
            "src": "http://test.local/some.js",
            "type": "get"
          }
        ],
        "DETECT_HTML":[
          "<div class=\"banner\"><span>ok</span></div>"
        ],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [
          "test_cookie"
        ],
        "EXTUID_COOKIE_NAMES": [
          "extuid",
          "second-ext-uid"
        ],
        "EXTUID_TAG": "test_extuid_tag",
        "FOLLOW_REDIRECT_URL_RE": [
          "(?:https?://)?(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?"
        ],
        "PARTNER_TOKENS": [
          "test_token6"
        ],
        "PROXY_URL_RE": [
          "test\\.local/.*",
          "this-is-a-fake-address\\.com(?::\\d+)?/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active"],
      "version": "test"
    },
  "zen.yandex.ru":
    {
      "config": {
        "ACCEL_REDIRECT_URL_RE": [
          "/accel-images/\\w+\\.png$"
        ],
        "ADFOX_DEBUG": true,
        "CLIENT_REDIRECT_URL_RE": [],
        "CM_TYPE": 0,
        "CRYPT_BODY_RE": {"\\bsmi2adblock[-\\w]+?\\b": null},
        "REPLACE_BODY_RE": {"(Ya)\\[\\d+\\]\\(": "Ya "},
        "CRYPT_ENABLE_TRAILING_SLASH": true,
        "CRYPT_RELATIVE_URL_RE": [
          "/static/[\\w/.-]+\\.(?:js|css|gif|png)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?",
          "/scripts/.*?",
          "(?:\\.{0,2}/)*testing/.*?"
        ],
        "CRYPT_SECRET_KEY": "duoYujaikieng9airah4Aexai4yek4qu",
        "COOKIE_CRYPT_KEY": "CookieCryptKey123456",
        "CRYPT_URL_PREFFIX": "/",
        "CRYPT_URL_OLD_PREFFIXES": ["/testprefix/", "/test_prefix_2/"],
        "BYPASS_URL_RE": [],
        "DETECT_CUSTOM": [
          {
            "div": {
              "class": "ring top",
              "div": {
                "class": "inner__banner"
              }
            }
          }
        ],
        "DETECT_ELEMS": {
          "id": [
            "mlph-banner",
            "popup_box"
          ]
        },
        "DETECT_LINKS": [
          {
            "src": "http://test.local/some.js",
            "type": "get"
          }
        ],
        "DETECT_HTML":[
          "<div class=\"banner\"><span>ok</span></div>"
        ],
        "ENCRYPTION_STEPS": [
          0,
          1,
          2,
          3
        ],
        "EXCLUDE_COOKIE_FORWARD": [
          "test_cookie"
        ],
        "EXTUID_COOKIE_NAMES": [
          "extuid",
          "second-ext-uid"
        ],
        "EXTUID_TAG": "test_extuid_tag",
        "FOLLOW_REDIRECT_URL_RE": [
          "(?:https?://)?(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?"
        ],
        "PARTNER_TOKENS": [
          "test_token_zen"
        ],
        "PROXY_URL_RE": [],
        "CRYPT_URL_RE": [
          "zen\\.yandex\\.ru/.*"
        ],
        "PUBLISHER_SECRET_KEY": "eyJhbGciOi"
      },
      "statuses": ["active"],
      "version": "test"
    }
}
"""
