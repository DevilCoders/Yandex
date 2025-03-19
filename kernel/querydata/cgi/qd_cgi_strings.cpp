#include "qd_cgi_strings.h"
#include "qd_cgi_utils.h"

#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/singleton.h>
#include <util/string/builder.h>
#include <util/string/vector.h>

namespace NQueryData {

    struct TCgiHelpRegistry {
        TVector<std::pair<TString, TString>> CgiHelp;
        TVector<std::pair<TString, TString>> JsonHelp;

        TCgiHelpRegistry()
        {

            AddCgi(CGI_QSREQ_JSON_1,
                   "JsonRequestV1",
                   "querysearch json request v.1 (see below), overrides its packed counterpart");
            AddCgi(CGI_QSREQ_JSON_ZLIB_1,
                   "Base64(Zlib(JsonRequestV1))",
                   "querysearch json request, packed with zlib/base64, overrides its cgi counterpart");
            AddCgi(CGI_REQ_ID,
                   "value",
                   "request id, to match relevant log entries");
            AddCgi(CGI_YANDEX_TLD,
                   "value",
                   "yandex tld, the top level domain the request came from");
            AddCgi(CGI_TEXT,
                   "value",
                   "user request text, required by web search core unless in info= mode");
            AddCgi(CGI_IGNORE_TEXT,
                   "1",
                   "the flag to ignore the dummy text supplied in 'text' cgi");
            AddCgi(CGI_USER_REQUEST,
                   "value",
                   "user request text, overrides text=");

            AddCgiRearr(CGI_STRUCT_KEYS,
                        "{ \"trie namespace\" : { \"key_foo\" : null, \"key_bar\" : null, ... } }",
                        "custom keys, json");
            AddCgiRearr(CGI_USER_REGIONS,
                        "[numid_N, numid_M, ... numid_0]",
                        "user regions hierarchy, comma-separated list of region codes from the most precise to the most general");
            AddCgiRearr(CGI_USER_REGIONS_IPREG,
                        "[numid_N, numid_M, ... numid_0]",
                        "user regions hierarchy by ipreg, comma-separated list of region codes from the most precise to the most general");
            AddCgiRearr(CGI_USER_REGIONS_IPREG_FULLPATH,
                        "[numid_N, numid_M, ... numid_0]",
                        "user regions hierarchy by ipreg, comma-separated list of region codes from the most precise to the most general, full depth");
            AddCgiRearr(CGI_USER_ID,
                        "value",
                        "user id");
            AddCgiRearr(CGI_USER_LOGIN,
                        "value",
                        "user login, plaintext");
            AddCgiRearr(CGI_USER_LOGIN_HASH,
                        "value",
                        "user login, hashed");
            AddCgiRearr(CGI_USER_IP_MOBILE_OP,
                        "mobile operator string as in ipgsm or 0 if none",
                        "user mobile operator");
            AddCgiRearr(CGI_DEVICE,
                        "value",
                        "serp type flag (" + JoinStrings(GetAllSerpTypes().begin(), GetAllSerpTypes().end(), "|") + ")");
            AddCgiRearr(CGI_IS_MOBILE,
                        "value",
                        "mobilesearch and touchsearch flag");
            AddCgiRearr(CGI_DOC_IDS,
                        "ZF000F000F000F000,ZB001B001B001B001,...",
                        "websearch docids (hashed urls), comma separated");
            AddCgiRearr(CGI_SNIP_DOC_IDS,
                        "ZF000F000F000F000,ZB001B001B001B001,...",
                        "websearch docids (hashed urls), comma separated, only for snippets");
            AddCgiRearr(CGI_URL_DEPRECATED,
                        "foo-owner.com,bar-owner.com,...",
                        "websearch categs (owners), comma separated");
            AddCgiRearr(CGI_CATEGS,
                        "foo-owner.com,bar-owner.com,...",
                        "websearch categs (owners), comma separated");
            AddCgiRearr(CGI_SNIP_CATEGS,
                        "foo-owner.com,bar-owner.com,...",
                        "websearch categs (owners), comma separated, only for snippets");
            AddCgiRearr(FormCompressedCgiPropName(CGI_URLS, CC_PLAIN), "{"
                        "'foo-owner.com/bar':'foo-owner.com',"
                        "'foo-owner.com/xyz':'foo-owner.com'" //
                    "}",
                        "urls descriptions, url -> categ");
            AddCgiRearr(FormCompressedCgiPropName(CGI_URLS, CC_ZLIB), "base64", TStringBuilder() << CGI_URLS << " + zlib + base64");
            AddCgiRearr(FormCompressedCgiPropName(CGI_SNIP_URLS, CC_PLAIN), "{"
                        "'foo-owner.com/bar':'foo-owner.com',"
                        "'foo-owner.com/xyz':'foo-owner.com'" //
                    "}",
                        "urls descriptions, url -> categ");
            AddCgiRearr(FormCompressedCgiPropName(CGI_SNIP_URLS, CC_PLAIN), "base64", TStringBuilder() << CGI_SNIP_URLS << " + zlib + base64");
            AddCgiRearr(CGI_FILTER_NAMESPACES,
                        "foo,bar,...", "search only these namespaces");
            AddCgiRearr(CGI_FILTER_FILES,
                        "foo.trie,bar.trie,...", "search only these files");
            AddCgiRearr(CGI_SKIP_NAMESPACES,
                        "foo,bar,...", "do not search these namespaces");
            AddCgiRearr(CGI_SKIP_FILES,
                        "foo.trie,bar.trie,...", "do not search these files");

            AddCgiRelev(CGI_DOPPEL_NORM,
                        "value",
                        "doppelgangers request normalization");
            AddCgiRelev(CGI_DOPPEL_NORM_W,
                        "value",
                        "doppelgangers request normalization for wildcards");
            AddCgiRelev(CGI_STRONG_NORM,
                        "value",
                        "strong request normalization");
            AddCgiRelev(CGI_UI_LANG,
                        "value",
                        "yandex ui language");

        }

        void AddCgiNested(TStringBuf top, TStringBuf param, TStringBuf type, TString descr) {
            AddCgi(TString::Join(top, param), type, descr);
        }

        void AddCgiRearr(TStringBuf param, TStringBuf type, TString descr) {
            AddCgiNested("rearr=", param, type, descr);
        }

        void AddCgiRelev(TStringBuf param, TStringBuf type, TString descr) {
            AddCgiNested("relev=", param, type, descr);
        }

        void AddCgi(TStringBuf param, TStringBuf type, TString descr) {
            CgiHelp.push_back(std::pair<TString, TString>(TString::Join(param, '=', type), descr));
        }

        void AddJsonStr(TStringBuf param, TString descr) {
            AddJson(param, "\"string value\"", descr);
        }

        void AddJson(TStringBuf param, TStringBuf type, TString descr) {
            JsonHelp.push_back(std::pair<TString, TString>(TString::Join(param, " : ", type), descr));
        }
    };

    TVector<std::pair<TString, TString>> GetCgiHelp() {
        return Default<TCgiHelpRegistry>().CgiHelp;
    }

}
