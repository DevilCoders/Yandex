#include <library/cpp/config/config.h>
#include <library/cpp/config/extra/yconf.h>

#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

#include <util/stream/str.h>
#include <util/generic/algorithm.h>
#include <util/string/cast.h>

static inline TString Touch(TStringBuf str) {
    TString ret;

    for (size_t i = 0; i < str.size(); ++i) {
        switch (str[i]) {
            case ' ':
            case '\t':
            case '\n':
            case ';':
            case ',':
                continue;
        }

        ret += str[i];
    }

    // kekeke
    Sort(ret.begin(), ret.vend());

    return ret;
}

class TConfigTest: public TTestBase {
    UNIT_TEST_SUITE(TConfigTest);
    UNIT_TEST(TestConfigJSON)
    UNIT_TEST(TestConfigLua)
    UNIT_TEST(TestConfigYandexCfg)
    UNIT_TEST(TestConfigOps)
    UNIT_TEST(TestEmptyIniSection)
    UNIT_TEST(TestIniSections)
    UNIT_TEST(TestTypeMismatch)
    UNIT_TEST_SUITE_END();

public:
    void TestConfigOps() {
        using namespace NConfig;
        {
            TConfig c = TConfig::ReadJson("{}");
            UNIT_ASSERT(c.IsA<TDict>());
        }
        {
            TConfig c = TConfig::ReadJson("[]");
            UNIT_ASSERT(c.IsA<TArray>());
        }
        {
            TConfig c = TConfig::ReadJson("\"\"");
            UNIT_ASSERT(c.IsA<TString>());
        }
        {
            TConfig c = TConfig::ReadJson("0");
            UNIT_ASSERT(c.IsNumeric());
        }
        {
            TConfig c = TConfig::ReadJson("0.0");
            UNIT_ASSERT(c.IsA<double>());
        }
        {
            TConfig c = TConfig::ReadJson("0");
            UNIT_ASSERT(c.IsA<i64>());
        }
        {
            TConfig c = TConfig::ReadJson("false");
            UNIT_ASSERT(c.IsA<bool>());
        }
        {
            TConfig c = TConfig::ReadJson("null");
            UNIT_ASSERT(c.IsA<TNull>());
        }
    }

    void DoTestConfigJSON(TStringBuf in, TStringBuf jsonout, TStringBuf luaout) {
        using namespace NConfig;
        TGlobals globals;
        globals["x"] = "x_value";
        TConfig c = TConfig::ReadJson(in, globals);

        {
            TStringStream s;
            c.DumpJson(s);
            UNIT_ASSERT_VALUES_EQUAL(Touch(s.Str()), Touch(jsonout));
        }

        {
            TStringStream s;
            c.DumpLua(s);

            try {
                UNIT_ASSERT_VALUES_EQUAL(Touch(s.Str()), Touch("return " + ToString(luaout)));
            } catch (...) {
            }
        }

        {
            TString ser;
            {
                TStringOutput out(ser);
                Save(&out, c);
            }

            TConfig c2;

            {
                TStringInput in(ser);
                Load(&in, c2);
            }

            TStringStream s;
            c2.DumpJson(s);
            UNIT_ASSERT_VALUES_EQUAL(Touch(s.Str()), Touch(jsonout));
        }

        {
            TString ser;
            const TVector<TConfig> v = {c};

            {
                TStringOutput out(ser);
                Save(&out, v);
            }

            TVector<TConfig> v2;

            {
                TStringInput in(ser);
                Load(&in, v2);
            }

            TStringStream s;
            v2[0].DumpJson(s);
            UNIT_ASSERT_VALUES_EQUAL(Touch(s.Str()), Touch(jsonout));
        }
    }

    void DoTestConfigLua(TStringBuf in, TStringBuf jsonout, TStringBuf luaout, TStringBuf = TStringBuf()) {
        using namespace NConfig;
        TGlobals globals;
        globals["x"] = "x_value";
        TConfig c = TConfig::ReadLua(in, globals);
        TStringStream s;
        c.DumpJson(s);

        try {
            DoTestConfigJSON(s.Str(), jsonout, luaout);
        } catch (...) {
            Cerr << ToString(in).Quote() << " " << ToString(jsonout).Quote() << " " << ToString(luaout).Quote() << Endl;

            throw;
        }
    }

    void DoTestConfigYandexCfg(TStringBuf in, TStringBuf jsonout, TStringBuf cfgout) {
        using namespace NConfig;
        TGlobals globals;
        globals["x"] = "x_value";
        TConfig c = NConfig::ReadYandexCfg(in, globals);
        {
            TStringStream s;
            c.DumpJson(s);
            NSc::NUt::AssertJsonJson(jsonout, s.Str());
        }
        {
            TStringStream s;
            NConfig::DumpYandexCfg(c, s);
            UNIT_ASSERT_STRINGS_EQUAL(NEscJ::EscapeJ<true>(cfgout), NEscJ::EscapeJ<true>(s.Str()));

            TConfig cc = NConfig::ReadYandexCfg(s.Str());

            TStringStream ss;
            cc.DumpJson(ss);

            NSc::NUt::AssertJsonJson(jsonout, ss.Str());
        }
    }

    void TestConfigYandexCfg() {
        DoTestConfigYandexCfg("", "{}", "");
        DoTestConfigYandexCfg("#junk", "{}", "");
        DoTestConfigYandexCfg("!junk", "{}", "");
        DoTestConfigYandexCfg(";junk", "{}", "");
        DoTestConfigYandexCfg("<!--junk-->", "{}", "");
        DoTestConfigYandexCfg("Key : val", "{Directives:{key:val}}", "key val\n");
        DoTestConfigYandexCfg("Key : ${x}", "{Directives:{key:x_value}}", "key x_value\n");
        DoTestConfigYandexCfg("<section>\n"
                              "</section>",
                              "{Sections:{section:[{}]}}",
                              "<section>\n"
                              "</section>\n");
        DoTestConfigYandexCfg("<section attr1=`val1` attr2='val2'>\n"
                              "  data = blabla\n"
                              "</section>\n",
                              "{Sections : {"
                              "  section : [{"
                              "    Attributes : { attr1 : val1, attr2 : val2 },"
                              "    Directives : { data : blabla }"
                              "  }]"
                              "}}",
                              "<section attr1=\"val1\" attr2=\"val2\">\n"
                              "  data blabla\n"
                              "</section>\n");

        DoTestConfigYandexCfg("<Server>\n"
                              "    ServerLog : ./logs/webds-server.log\n"
                              "</Server>\n"
                              "\n"
                              "<Collection id=\"webds\" class=\"\">\n"
                              "    IndexDir : ./indexs/webds-index\n"
                              "    TempDir : ./indexs/webds-temp\n"
                              "    <IndexLog>\n"
                              "        FileName : ./logs/webds-index.log\n"
                              "        Level : moredebug verbose\n"
                              "    </IndexLog>\n"
                              "    <DataSrc id=\"webds\">\n"
                              "        <Webds>\n"
                              "            StartUrls : http://help.yandex.ru\n"
                              "            <Extensions>\n"
                              "                text/html : .html, .htm, .shtml\n"
                              "                text/xml : .xml\n"
                              "                application/pdf : .pdf\n"
                              "                application/msword : .doc\n"
                              "            </Extensions>\n"
                              "        </Webds>\n"
                              "        <Webds a='aaa' b=`bbb`>\n"
                              "            StartUrls : http://www.yandex.ru\n"
                              "        </Webds>\n"
                              "    </DataSrc>\n"
                              "</Collection>\n",
                              "{"
                              "  Sections : { "
                              "    collection : [{"
                              "      Attributes : {"
                              "        class : '',"
                              "        id : webds"
                              "      },"
                              "      Directives : {"
                              "        indexdir : './indexs/webds-index',"
                              "        tempdir : './indexs/webds-temp'"
                              "      },"
                              "      Sections : {"
                              "        datasrc : ["
                              "          {"
                              "            Attributes : {"
                              "              id : webds"
                              "            },"
                              "            Sections : {"
                              "              webds : ["
                              "                {"
                              "                  Directives : {"
                              "                    starturls : 'http://help.yandex.ru'"
                              "                  },"
                              "                  Sections : {"
                              "                    extensions : [{"
                              "                      Directives : {"
                              "                        'application/msword' : '.doc',"
                              "                        'application/pdf' : '.pdf',"
                              "                        'text/html' : '.html, .htm, .shtml',"
                              "                        'text/xml' : '.xml'"
                              "                      }"
                              "                    }]"
                              "                  }"
                              "                },"
                              "                {"
                              "                  Directives : {"
                              "                    starturls : 'http://www.yandex.ru'"
                              "                  },"
                              "                  Attributes : {"
                              "                    a : aaa, b: bbb"
                              "                  }"
                              "                }"
                              "              ]"
                              "            }"
                              "          }"
                              "        ],"
                              "        indexlog : [{"
                              "          Directives : {"
                              "            filename : './logs/webds-index.log',"
                              "            level : 'moredebug verbose'"
                              "          }"
                              "        }]"
                              "      }"
                              "    }],"
                              "    server : [{"
                              "      Directives : {"
                              "        serverlog : './logs/webds-server.log'"
                              "      }"
                              "    }]"
                              "  }"
                              "}",
                              "<collection class=\"\" id=\"webds\">\n"
                              "  indexdir ./indexs/webds-index\n"
                              "  tempdir ./indexs/webds-temp\n"
                              "  <datasrc id=\"webds\">\n"
                              "    <webds>\n"
                              "      starturls http://help.yandex.ru\n"
                              "      <extensions>\n"
                              "        application/msword .doc\n"
                              "        application/pdf .pdf\n"
                              "        text/html .html, .htm, .shtml\n"
                              "        text/xml .xml\n"
                              "      </extensions>\n"
                              "    </webds>\n"
                              "    <webds a=\"aaa\" b=\"bbb\">\n"
                              "      starturls http://www.yandex.ru\n"
                              "    </webds>\n"
                              "  </datasrc>\n"
                              "  <indexlog>\n"
                              "    filename ./logs/webds-index.log\n"
                              "    level moredebug verbose\n"
                              "  </indexlog>\n"
                              "</collection>\n"
                              "<server>\n"
                              "  serverlog ./logs/webds-server.log\n"
                              "</server>\n");
    }

    void TestConfigJSON() {
        using namespace NConfig;

        //DoTestConfigJSON("", "null", "nil");
        DoTestConfigJSON("null", "null", "nil");
        DoTestConfigJSON("true", "true", "true");
        DoTestConfigJSON("1", "1", "1");
        DoTestConfigJSON("0.1", "0.1", "0.1");
        DoTestConfigJSON("\"${x or 'def'}\"", "\"x_value\"", "\"x_value\"");
        DoTestConfigJSON("\"${y or 'def'}\"", "\"def\"", "\"def\"");
        DoTestConfigJSON("\"test\"", "\"test\"", "\"test\"");

        if (0)
            DoTestConfigJSON("{\"\\u0010\\u0005\":\"\\u0020\\u0018\"}", "{\n  \"\\u0010\\u0005\" : \" \\u0018\"\n}", "{\n  [\"\\016\\005\"] = \" \\024\"\n}");
        DoTestConfigJSON("[]", "[ ]", "{ }");
        DoTestConfigJSON("{}", "{ }", "{ }");
        DoTestConfigJSON("[1]", "[\n  1\n]", "{\n  1\n}");
        DoTestConfigJSON("{\"a\":1}", "{\n  \"a\" : 1\n}", "{\n  a = 1\n}");
        DoTestConfigJSON("{\"b\":2,\"a\":1}", "{\n  \"a\" : 1,\n  \"b\" : 2\n}", "{\n  a = 1,\n  b = 2\n}");
        DoTestConfigJSON("[1,2,3]", "[\n  1,\n  2,\n  3\n]", "{\n  1,\n  2,\n  3\n}");
        DoTestConfigJSON("{\"a\":1,\"b\":2,\"c\":3}", "{\n  \"a\" : 1,\n  \"b\" : 2,\n  \"c\" : 3\n}", "{\n  a = 1,\n  b = 2,\n  c = 3\n}");
        DoTestConfigJSON("[{\"a\":[{\"b\":[\"c\",\"d\"], \"e\":\"f\"},\"g\"],\"h\":\"i\"},\"j\"]",
                         "[\n"
                         "  {\n"
                         "    \"a\" : [\n"
                         "      {\n"
                         "        \"b\" : [\n"
                         "          \"c\",\n"
                         "          \"d\"\n"
                         "        ],\n"
                         "        \"e\" : \"f\"\n"
                         "      },\n"
                         "      \"g\"\n"
                         "    ],\n"
                         "    \"h\" : \"i\"\n"
                         "  },\n"
                         "  \"j\"\n"
                         "]",
                         "{\n"
                         "  {\n"
                         "    a = {\n"
                         "      {\n"
                         "        b = {\n"
                         "          \"c\",\n"
                         "          \"d\"\n"
                         "        },\n"
                         "        e = \"f\"\n"
                         "      },\n"
                         "      \"g\"\n"
                         "    },\n"
                         "    h = \"i\"\n"
                         "  },\n"
                         "  \"j\"\n"
                         "}");
    }

    void TestConfigLua() {
        using namespace NConfig;

        DoTestConfigLua("", "null", "nil");
        DoTestConfigLua("nil", "null", "nil");
        DoTestConfigLua("true", "true", "true");
        DoTestConfigLua("false", "false", "false");
        DoTestConfigLua("1", "1", "1");
        DoTestConfigLua("1.1", "1.1", "1.1");

        DoTestConfigLua("x or 'def'", "\"x_value\"", "\"x_value\"");
        DoTestConfigLua("y or 'def'", "\"def\"", "\"def\"");

        DoTestConfigLua("{}", "[ ]", "{ }");

        DoTestConfigLua("main=nil", "null", "nil", "main");
        DoTestConfigLua("main=true", "true", "true", "main");
        DoTestConfigLua("main=false", "false", "false", "main");
        DoTestConfigLua("main=1", "1", "1", "main");
        DoTestConfigLua("main=1.1", "1.1", "1.1", "main");

        DoTestConfigLua("main = x or 'def'", "\"x_value\"", "\"x_value\"", "main");
        DoTestConfigLua("main = y or 'def'", "\"def\"", "\"def\"", "main");

        DoTestConfigLua("main = {}", "[ ]", "{ }", "main");

        DoTestConfigLua("\"foo'bar\"", "\"foo'bar\"", "\"foo'bar\"");

        if (0)
            DoTestConfigLua(
                "'\\0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\10\\11\\12\\13\\14\\15\\16\\17\\18\\19\\20\\21\\22\\23\\24\\25\\26\\27\\28\\29\\30\\31'",
                "\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b"
                "\\t"
                "\\n"
                "\\u000B\\f"
                "\\r"
                "\\u000E\\u000F"
                "\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001A\\u001B\\u001C\\u001D\\u001E\\u001F\"",
                "\"\\000\\001\\002\\003\\004\\005\\006\\a"
                "\\b"
                "\\t"
                "\\n"
                "\\v"
                "\\f"
                "\\r"
                "\\014\\015"
                "\\016\\017\\018\\019\\020\\021\\022\\023\\024\\025\\026\\027\\028\\029\\030\\031\"");

        if (0)
            DoTestConfigLua("{'a','b',junk=\'trash\',12,}", "[\n  \"a\",\n  \"b\",\n  12\n]", "{\n  \"a\",\n  \"b\",\n  12\n}");

        if (0)
            DoTestConfigLua("{a='b',c='d',['\\0\\1'] = {'x','y','z'}}", "{\n"
                                                                        "  \"\\u0000\\u0001\" : [\n"
                                                                        "    \"x\",\n"
                                                                        "    \"y\",\n"
                                                                        "    \"z\"\n"
                                                                        "  ],\n"
                                                                        "  \"a\" : \"b\",\n"
                                                                        "  \"c\" : \"d\"\n"
                                                                        "}",
                            "{\n"
                            "  [\"\\000\\001\"] = {\n"
                            "    \"x\",\n"
                            "    \"y\",\n"
                            "    \"z\"\n"
                            "  },\n"
                            "  a = \"b\",\n"
                            "  c = \"d\"\n"
                            "}");

        if (0)
            DoTestConfigLua(
                "local function Balancer()\n"
                "    return {\n"
                "        robust = {\n"
                "            {\n"
                "                weight = 1.0;\n"
                "\n"
                "                proxy = {\n"
                "                    timeout = \"1s\";\n"
                "                    backend_timeout = {\"1s\", \"10s\"};\n"
                "                    host = \"porter048\";\n"
                "                    port = \"11002\";\n"
                "                };\n"
                "            };\n"
                "\n"
                "            {\n"
                "                weight = 1.0;\n"
                "\n"
                "                proxy = {\n"
                "                    timeout = \"1s\";\n"
                "                    host = \"porter047\";\n"
                "                    port = \"11002\";\n"
                "                };\n"
                "            };\n"
                "        };\n"
                "    };\n"
                "end\n"
                "\n"
                "local function Arc()\n"
                "    return {\n"
                "        robust = {\n"
                "            {\n"
                "                weight = 1.0;\n"
                "\n"
                "                proxy = {\n"
                "                    host = \"porter047\";\n"
                "                    port = \"20001\";\n"
                "                };\n"
                "            };\n"
                "        };\n"
                "    };\n"
                "end\n"
                "\n"
                "local function Log()\n"
                "    return \"/hol/www/logs/golovan-8080.log\";\n"
                "end\n"
                "\n"
                "instance = {\n"
                "    port = 8080;\n"
                "    log = Log();\n"
                "\n"
                "    http = {\n"
                "        accesslog = {\n"
                "            regexp = {\n"
                "                admin = {\n"
                "                    match = \'/admin/\';\n"
                "\n"
                "                    errorlog = {\n"
                "                        tag = \"admin -> \";\n"
                "\n"
                "                        admin = {\n"
                "                        };\n"
                "                    };\n"
                "                };\n"
                "\n"
                "                srv = {\n"
                "                    match = \"/srv/\";\n"
                "\n"
                "                    errorlog = {\n"
                "                        tag = \"srv -> \";\n"
                "                        balancer = Balancer();\n"
                "                    };\n"
                "                };\n"
                "\n"
                "                time = {\n"
                "                    match = \"/time/\";\n"
                "\n"
                "                    errorlog = {\n"
                "                        tag = \"time -> \";\n"
                "                        balancer = Balancer();\n"
                "                    };\n"
                "                };\n"
                "\n"
                "                arc = {\n"
                "                    match = \"/arc/\";\n"
                "\n"
                "                    errorlog = {\n"
                "                        tag = \"arc -> \";\n"
                "                        balancer = Arc();\n"
                "                    };\n"
                "                };\n"
                "\n"
                "                media = {\n"
                "                    match = \"/media/\";\n"
                "\n"
                "                    errorlog = {\n"
                "                        tag = \"media -> \";\n"
                "\n"
                "                        static = {\n"
                "                            dir = \"/place/yasm/misc\";\n"
                "                        };\n"
                "                    };\n"
                "                };\n"
                "\n"
                "                default = {\n"
                "                    errorlog = {\n"
                "                        tag = \"static -> \";\n"
                "\n"
                "                        static = {\n"
                "                            index = \"index.xhtml\";\n"
                "                            ---dir = \"/place/yasm/wwwdata\";\n"
                "                            dir = \"./wwwdata\";\n"
                "                        };\n"
                "                    };\n"
                "                };\n"
                "            };\n"
                "        };\n"
                "    };\n"
                "}\n",
                "{\n"
                "  \"http\" : {\n"
                "    \"accesslog\" : {\n"
                "      \"regexp\" : {\n"
                "        \"admin\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"admin\" : { },\n"
                "            \"tag\" : \"admin -> \"\n"
                "          },\n"
                "          \"match\" : \"/admin/\"\n"
                "        },\n"

                "        \"arc\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"balancer\" : {\n"
                "              \"robust\" : [\n"
                "                {\n"
                "                  \"proxy\" : {\n"
                "                    \"host\" : \"porter047\",\n"
                "                    \"port\" : \"20001\"\n"
                "                  },\n"
                "                  \"weight\" : 1\n"
                "                }\n"
                "              ]\n"
                "            },\n"
                "            \"tag\" : \"arc -> \"\n"
                "          },\n"
                "          \"match\" : \"/arc/\"\n"
                "        },\n"

                "        \"default\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"static\" : {\n"
                "              \"dir\" : \"./wwwdata\",\n"
                "              \"index\" : \"index.xhtml\"\n"
                "            },\n"
                "            \"tag\" : \"static -> \"\n"
                "          }\n"
                "        },\n"

                "        \"media\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"static\" : {\n"
                "              \"dir\" : \"/place/yasm/misc\"\n"
                "            },\n"
                "            \"tag\" : \"media -> \"\n"
                "          },\n"
                "          \"match\" : \"/media/\"\n"
                "        },\n"

                "        \"srv\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"balancer\" : {\n"
                "              \"robust\" : [\n"
                "                {\n"
                "                  \"proxy\" : {\n"
                "                    \"backend_timeout\" : [\n"
                "                      \"1s\",\n"
                "                      \"10s\"\n"
                "                    ],\n"
                "                    \"host\" : \"porter048\",\n"
                "                    \"port\" : \"11002\",\n"
                "                    \"timeout\" : \"1s\"\n"
                "                  },\n"
                "                  \"weight\" : 1\n"
                "                },\n"
                "                {\n"
                "                  \"proxy\" : {\n"
                "                    \"host\" : \"porter047\",\n"
                "                    \"port\" : \"11002\",\n"
                "                    \"timeout\" : \"1s\"\n"
                "                  },\n"
                "                  \"weight\" : 1\n"
                "                }\n"
                "              ]\n"
                "            },\n"
                "            \"tag\" : \"srv -> \"\n"
                "          },\n"
                "          \"match\" : \"/srv/\"\n"
                "        },\n"

                "        \"time\" : {\n"
                "          \"errorlog\" : {\n"
                "            \"balancer\" : {\n"
                "              \"robust\" : [\n"
                "                {\n"
                "                  \"proxy\" : {\n"
                "                    \"backend_timeout\" : [\n"
                "                      \"1s\",\n"
                "                      \"10s\"\n"
                "                    ],\n"
                "                    \"host\" : \"porter048\",\n"
                "                    \"port\" : \"11002\",\n"
                "                    \"timeout\" : \"1s\"\n"
                "                  },\n"
                "                  \"weight\" : 1\n"
                "                },\n"
                "                {\n"
                "                  \"proxy\" : {\n"
                "                    \"host\" : \"porter047\",\n"
                "                    \"port\" : \"11002\",\n"
                "                    \"timeout\" : \"1s\"\n"
                "                  },\n"
                "                  \"weight\" : 1\n"
                "                }\n"
                "              ]\n"
                "            },\n"
                "            \"tag\" : \"time -> \"\n"
                "          },\n"
                "          \"match\" : \"/time/\"\n"
                "        }\n"
                "      }\n"
                "    }\n"
                "  },\n"
                "  \"log\" : \"/hol/www/logs/golovan-8080.log\",\n"
                "  \"port\" : 8080\n"
                "}",
                "{\n"
                "  http = {\n"
                "    accesslog = {\n"
                "      regexp = {\n"
                "        admin = {\n"
                "          errorlog = {\n"
                "            admin = { },\n"
                "            tag = \"admin -> \"\n"
                "          },\n"
                "          match = \"/admin/\"\n"
                "        },\n"

                "        arc = {\n"
                "          errorlog = {\n"
                "            balancer = {\n"
                "              robust = {\n"
                "                {\n"
                "                  proxy = {\n"
                "                    host = \"porter047\",\n"
                "                    port = \"20001\"\n"
                "                  },\n"
                "                  weight = 1\n"
                "                }\n"
                "              }\n"
                "            },\n"
                "            tag = \"arc -> \"\n"
                "          },\n"
                "          match = \"/arc/\"\n"
                "        },\n"

                "        default = {\n"
                "          errorlog = {\n"
                "            static = {\n"
                "              dir = \"./wwwdata\",\n"
                "              index = \"index.xhtml\"\n"
                "            },\n"
                "            tag = \"static -> \"\n"
                "          }\n"
                "        },\n"

                "        media = {\n"
                "          errorlog = {\n"
                "            static = {\n"
                "              dir = \"/place/yasm/misc\"\n"
                "            },\n"
                "            tag = \"media -> \"\n"
                "          },\n"
                "          match = \"/media/\"\n"
                "        },\n"

                "        srv = {\n"
                "          errorlog = {\n"
                "            balancer = {\n"
                "              robust = {\n"
                "                {\n"
                "                  proxy = {\n"
                "                    backend_timeout = {\n"
                "                      \"1s\",\n"
                "                      \"10s\"\n"
                "                    },\n"
                "                    host = \"porter048\",\n"
                "                    port = \"11002\",\n"
                "                    timeout = \"1s\"\n"
                "                  },\n"
                "                  weight = 1\n"
                "                },\n"
                "                {\n"
                "                  proxy = {\n"
                "                    host = \"porter047\",\n"
                "                    port = \"11002\",\n"
                "                    timeout = \"1s\"\n"
                "                  },\n"
                "                  weight = 1\n"
                "                }\n"
                "              }\n"
                "            },\n"
                "            tag = \"srv -> \"\n"
                "          },\n"
                "          match = \"/srv/\"\n"
                "        },\n"

                "        time = {\n"
                "          errorlog = {\n"
                "            balancer = {\n"
                "              robust = {\n"
                "                {\n"
                "                  proxy = {\n"
                "                    backend_timeout = {\n"
                "                      \"1s\",\n"
                "                      \"10s\"\n"
                "                    },\n"
                "                    host = \"porter048\",\n"
                "                    port = \"11002\",\n"
                "                    timeout = \"1s\"\n"
                "                  },\n"
                "                  weight = 1\n"
                "                },\n"
                "                {\n"
                "                  proxy = {\n"
                "                    host = \"porter047\",\n"
                "                    port = \"11002\",\n"
                "                    timeout = \"1s\"\n"
                "                  },\n"
                "                  weight = 1\n"
                "                }\n"
                "              }\n"
                "            },\n"
                "            tag = \"time -> \"\n"
                "          },\n"
                "          match = \"/time/\"\n"
                "        }\n"
                "      }\n"
                "    }\n"
                "  },\n"
                "  log = \"/hol/www/logs/golovan-8080.log\",\n"
                "  port = 8080\n"
                "}",
                "instance");
    }

    void TestEmptyIniSection() {
        TString input = "[foo]\nfoo = 123\n\n[bar]\n";
        TStringInput inputStream(input);
        auto config = NConfig::TConfig::FromIni(inputStream);

        const auto& sections = config.Get<NConfig::TDict>();
        sections.At("foo").Get<NConfig::TDict>();
        sections.At("bar").Get<NConfig::TDict>();
    }

    void TestIniSections() {
        TString input = "[foo.bar]\nfoo = 123\n\n[foo.bar1]\nfoo = 234\n";
        TStringInput inputStream(input);
        auto config = NConfig::TConfig::FromIni(inputStream);

        const auto& sections = config.Get<NConfig::TDict>();
        const auto& fooSection = sections.At("foo").Get<NConfig::TDict>();
        UNIT_ASSERT(fooSection.contains("bar"));
        UNIT_ASSERT(fooSection.contains("bar1"));
    }

    void TestTypeMismatch() {
        using namespace NConfig;
        {
            TConfig c = TConfig::ReadJson("{}");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c.As<short>(), TTypeMismatch, "dict");
        }
        {
            TConfig c = TConfig::ReadJson("[]");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c.As<unsigned short>(), TTypeMismatch, "array");
        }
        {
            TConfig c = TConfig::ReadJson("null");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c.At("key"), TTypeMismatch, "null");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c.At("key"), TTypeMismatch, "dict");
        }
        {
            TConfig c = TConfig::ReadJson("null");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c[4], TTypeMismatch, "null");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c[4], TTypeMismatch, "array");
        }
        {
            enum class EEnum {};
            TConfig c = TConfig::ReadJson("null");
            UNIT_ASSERT_EXCEPTION_CONTAINS(c.Get<EEnum>(), TTypeMismatch, "null");
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TConfigTest);
