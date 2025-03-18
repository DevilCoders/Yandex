#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/blackbox2/blackbox2.h>
#include <library/cpp/blackbox2/src/utils.h>
#include <library/cpp/blackbox2/src/xconfig.h>

#include <util/folder/path.h>
#include <util/stream/file.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace NBlackbox2;

Y_UNIT_TEST_SUITE(CheckReq) {
    // supported methods
    const TString strInfo("info");
    const TString strLogin("login");
    const TString strSession("session");
    const TString strOAuth("oauth");

    // mailhost operations
    const TString strOpCreate("create");
    const TString strOpDelete("delete");
    const TString strOpAssign("assign");
    const TString strOpSetPrio("setprio");
    const TString strOpFind("find");

    void read_opts(TOptions & opts, xmlConfig::Parts & xml_opts) {
        for (int i = 0; i < xml_opts.Size(); ++i) {
            xmlConfig::Part item(xml_opts[i]);
            TString key, val;
            if (item.GetIfExists("@key", key)) {
                item.GetIfExists("@val", val);
                opts << TOption(key, val);
            }
        }
    }

    TDBFields read_dbfields(xmlConfig::Parts & xml_fields) {
        TDBFields out;
        for (int i = 0; i < xml_fields.Size(); ++i) {
            TString field = xml_fields[i].asString();

            if (!field.empty())
                out << field;
        }

        return out;
    }

    TOptAliases read_aliases(xmlConfig::Parts & xml_aliases) {
        TOptAliases out;
        for (int i = 0; i < xml_aliases.Size(); ++i) {
            TString alias = xml_aliases[i].asString();

            if (!alias.empty())
                out << alias;
        }

        return out;
    }

    TAttributes read_attributes(xmlConfig::Parts & xml_attributes) {
        TAttributes out;
        for (int i = 0; i < xml_attributes.Size(); ++i) {
            TString attribute = xml_attributes[i].asString();

            if (!attribute.empty())
                out << attribute;
        }

        return out;
    }

    // for now, just compareTStrings
    bool matchUri(const TString& uri, const TString& ref) {
        if (uri != ref) {
            Cout << "Request uri do not match!" << Endl;
            Cout << "Request  : '" << uri << "'" << Endl;
            Cout << "Reference: '" << ref << "'" << Endl;

            return false;
        }

        return true; // matches
    }

    void foo(const TString& filename) {
        // Cmd-line arg - input file name, stdin if no args
        TString strConfig;

        {
            TFileInput file(filename.c_str());
            strConfig = file.ReadAll();
        }

        xmlConfig::XConfig conf;
        conf.Parse(strConfig);

        // common options
        TString type, uid, suid, sid, login, userip;
        conf.GetIfExists("/doc/type", type);
        conf.GetIfExists("/doc/uid", uid);
        conf.GetIfExists("/doc/suid", suid);
        conf.GetIfExists("/doc/sid", sid);
        conf.GetIfExists("/doc/login", login);
        conf.GetIfExists("/doc/userip", userip);

        UNIT_ASSERT(!type.empty());

        // options
        TOptions opts;
        TString val;

        if (conf.GetIfExists("/doc/regname", val))
            opts << OPT_REGNAME;

        if (conf.GetIfExists("/doc/email/getall", val))
            opts << OPT_GET_ALL_EMAILS;

        if (conf.GetIfExists("/doc/email/getyandex", val))
            opts << OPT_GET_YANDEX_EMAILS;

        if (conf.GetIfExists("/doc/email/testone", val))
            opts << TOptTestEmail(val);

        if (conf.GetIfExists("/doc/email/getdefault", val))
            opts << OPT_GET_DEFAULT_EMAIL;

        if (conf.GetIfExists("/doc/aliases/getsocial", val))
            opts << OPT_GET_SOCIAL_ALIASES;

        if (conf.GetIfExists("/doc/ver2", val))
            opts << OPT_VERSION2;

        if (conf.GetIfExists("/doc/authid", val))
            opts << OPT_AUTH_ID;

        if (conf.GetIfExists("/doc/multisession", val))
            opts << OPT_MULTISESSION;

        if (conf.GetIfExists("/doc/fullinfo", val))
            opts << OPT_FULL_INFO;

        xmlConfig::Parts xml_opts = conf.GetParts("/doc/option");
        xmlConfig::Parts xml_fields = conf.GetParts("/doc/dbfield");
        xmlConfig::Parts xml_aliases = conf.GetParts("/doc/aliases/alias");
        xmlConfig::Parts xml_attributes = conf.GetParts("/doc/attribute");

        read_opts(opts, xml_opts);

        TDBFields fields = read_dbfields(xml_fields);
        opts << fields;

        TOptAliases aliases = read_aliases(xml_aliases);
        opts << aliases;

        if (conf.GetIfExists("/doc/aliases/all", val))
            opts << OPT_GET_ALL_ALIASES;

        TAttributes attributes = read_attributes(xml_attributes);
        opts << attributes;

        // login && session specific params
        TString password, authtype, sessionid, hostname, token;
        conf.GetIfExists("/doc/password", password);
        conf.GetIfExists("/doc/authtype", authtype);
        conf.GetIfExists("/doc/sessionid", sessionid);
        conf.GetIfExists("/doc/session_host", hostname);
        conf.GetIfExists("/doc/token", token);

        // mailhost specific params
        TString operation, scope, dbid, priority, mx, olddbid;
        conf.GetIfExists("/doc/operation", operation);
        conf.GetIfExists("/doc/scope", scope);
        conf.GetIfExists("/doc/dbid", dbid);
        conf.GetIfExists("/doc/priority", priority);
        conf.GetIfExists("/doc/mx", mx);
        conf.GetIfExists("/doc/olddbid", olddbid);

        // reference request
        TString ref_request, ref_body;
        conf.GetIfExists("/doc/request", ref_request);
        conf.GetIfExists("/doc/reqbody", ref_body);

        if (type == strLogin) {
            UNIT_ASSERT(!((login.empty() && uid.empty()) || password.empty()));

            TLoginReqData login_data = uid.empty() ? LoginRequest(TLoginSid(login, sid), password, authtype, userip, opts)
                                                   : LoginRequestUid(uid, password, authtype, userip, opts);

            UNIT_ASSERT(matchUri(login_data.Uri_, ref_request));

            UNIT_ASSERT_C(login_data.PostData_ == ref_body,
                          "Login request bodies do not match!" << Endl
                                                               << "Request  :'" << login_data.PostData_ << "'" << Endl
                                                               << "Reference:'" << ref_body << "'" << Endl);

        } else {
            TString req;

            if (type == strInfo) {
                if (!uid.empty()) // info by uid
                    req = InfoRequest(uid, userip, opts);
                else if (!login.empty()) // info by login/sid
                    req = InfoRequest(TLoginSid(login, sid), userip, opts);
                else if (!suid.empty() && !sid.empty()) // info by suid/sid
                    req = InfoRequest(TSuidSid(suid, sid), userip, opts);
                else {
                    xmlConfig::Parts uidspart(conf.GetParts("/doc/uids/uid"));
                    UNIT_ASSERT(uidspart.Size() != 0);
                    TVector<TString> uids;

                    for (int i = 0; i < uidspart.Size(); ++i)
                        uids.push_back(uidspart[i].asString());

                    req = InfoRequest(uids, userip, opts);
                }
            } else if (type == strSession) {
                req = SessionIDRequest(sessionid, hostname, userip, opts);
            } else if (type == strOAuth) {
                req = OAuthRequest(token, userip, opts);
            }

            UNIT_ASSERT_C(matchUri(req, ref_request), type);
        }
    }

    Y_UNIT_TEST(req) {
        TString dir = ArcadiaSourceRoot() + "/library/cpp/blackbox2/ut/request/";
        TFsPath path(dir);

        TVector<TString> childes;
        path.ListNames(childes);

        for (const TString& f : childes) {
            foo(dir + f);
        }
    }
}
