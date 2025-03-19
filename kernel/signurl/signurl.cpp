#include "signurl.h"

#include <contrib/libs/openssl/include/openssl/blowfish.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/memory/tempbuf.h>

#include <utility>

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
}

using namespace std;

#define ROBOT_KEYNO "8"

namespace  NSignUrl {
    static size_t blowfish(const char* in, size_t datasize, char* out, const BF_KEY& bfkey, int mode) {
        unsigned char ivec[8] = {'a', 'r', 'c', 'a', 'd', 'i', 'a', '+'};
        size_t padded_datasize;

        if (Y_UNLIKELY(!datasize)) {
            datasize = strlen((char*)in);
        }

        padded_datasize = CryptedSize(datasize);
        memset(out, 0, padded_datasize);
        if (padded_datasize > datasize) {
            //actually pad data
            TTempBuf input(padded_datasize);
            input.Append(in, datasize);
            memset(input.Current(), '\0', padded_datasize - datasize);
            BF_cbc_encrypt((unsigned char*)input.Data(), (unsigned char*)out, padded_datasize, &bfkey, ivec, mode);
        } else {
            BF_cbc_encrypt((unsigned char*)in, (unsigned char*)out, padded_datasize, &bfkey, ivec, mode);
        }

        int skipped = 0;
        if ((mode == BF_DECRYPT) && padded_datasize) { // eliminate dummy padded spaces
            char* s = &out[padded_datasize - 1];
            while (s >= out && (*s-- == '\0') && (skipped < 8))
                skipped++;
        }
        return (padded_datasize - skipped);
    }

    size_t Crypt(const char* in, char* out, const char* key, size_t key_sz) {
        BF_KEY bfkey;
        BF_set_key(&bfkey, (int)key_sz, (const unsigned char*)key);
        return Crypt(in, out, bfkey);
    }

    size_t Crypt(const char* in, char* out, const TKey& key) {
        return Crypt(in, out, key.second);
    }

    size_t Crypt(const char* in, char* out, const BF_KEY& key) {
        return blowfish(in, strlen(in), out, key, BF_ENCRYPT);
    }

    size_t CryptBinary(const char* in, size_t in_sz, char* out, const char* key, size_t key_sz) {
        BF_KEY bfkey;
        BF_set_key(&bfkey, (int)key_sz, (const unsigned char*)key);
        return CryptBinary(in, in_sz, out, bfkey);
    }

    size_t CryptBinary(const char* in, size_t in_sz, char* out, const TKey& key) {
        return CryptBinary(in, in_sz, out, key.second);
    }

    size_t CryptBinary(const char* in, size_t in_sz, char* out, const BF_KEY& bfkey) {
        return blowfish(in, in_sz, out, bfkey, BF_ENCRYPT);
    }

    size_t Decrypt(const char* in, size_t in_sz, char* out, const char* key, size_t key_sz) {
        BF_KEY bfkey;
        BF_set_key(&bfkey, (int)key_sz, (const unsigned char*)key);
        return Decrypt(in, in_sz, out, bfkey);
    }

    size_t Decrypt(const char* in, size_t in_sz, char* out, const TKey& key) {
        return Decrypt(in, in_sz, out, key.second);
    }

    size_t Decrypt(const char* in, size_t in_sz, char* out, const BF_KEY& bfkey) {
        return blowfish(in, in_sz, out, bfkey, BF_DECRYPT);
    }

    static void DoSign(const char* in, size_t txt_sz, char* out, const TKey& key) {
        Y_ASSERT(in);
        Y_ASSERT(txt_sz > 0);
        Y_ASSERT(out);
        Y_ASSERT(!key.first.empty());

        MD5 md5;
        md5.Init();
        md5.Update(in, txt_sz);
        md5.Update(key.first);
        md5.End(out);
    }

    void Sign(const char* in, size_t txt_sz, char* out, const char* key, size_t key_sz) {
        if (out == nullptr) {
            return;
        }

        if (in == nullptr || txt_sz == 0 || key == nullptr || key_sz == 0) {
            out[0] = '\0';
            return;
        }

        TKey keyData = TKey(TString(key, key_sz), BF_KEY{{}, {}});
        BF_set_key(&keyData.second, (int)key_sz, (const unsigned char*)key);
        return DoSign(in, txt_sz, out, keyData);
    }

    void Sign(const char* in, size_t txt_sz, char* out, const TKey& key) {
        if (out == nullptr) {
            return;
        }

        if (in == nullptr || txt_sz == 0 || key.first.empty()) {
            out[0] = '\0';
            return;
        }

        return DoSign(in, txt_sz, out, key);
    }

    bool DoVerify(const char* txt_signature, const char* txt_unsigned, size_t txt_sz, const TKey& key) {
        Y_ASSERT(txt_signature);
        Y_ASSERT(txt_unsigned);
        Y_ASSERT(txt_sz > 0);
        Y_ASSERT(!key.first.empty());

        const size_t signature_len = strlen(txt_signature);
        if (signature_len == 0) {
            return false;
        }

        char signature[33];

        if (signature_len == (SU_SIGN_SZ - 1)) {
            Sign(txt_unsigned, txt_sz, signature, key);
            return strcmp(txt_signature, signature) == 0;
        }

        return false;
    }

    bool Verify(const char* txt_signature, const char* txt_unsigned, size_t txt_sz, const char* key, size_t key_sz) {
        if (!txt_signature || !txt_unsigned || !txt_sz || !key || !key_sz) {
            return false;
        }

        TKey keyData = TKey(TString(key, key_sz), BF_KEY{{}, {}});
        BF_set_key(&keyData.second, (int)key_sz, (const unsigned char*)key);

        return DoVerify(txt_signature, txt_unsigned, txt_sz, keyData);
    }

    bool Verify(const char* txt_signature, const char* txt_unsigned, size_t txt_sz, const TKey& key) {
        if (!txt_signature || !txt_unsigned || !txt_sz || key.first.empty()) {
            return false;
        }

        return DoVerify(txt_signature, txt_unsigned, txt_sz, key);
    }

    static const ui32 buf_data_len = 4096 * 4 * 2;
    TString Crypt(const TString& data, const TKey& key) {
        char mybuf[buf_data_len];
        size_t crypted_len = Crypt(data.data(), mybuf, key.second);
        size_t crypt_sz = CryptedSize(data.size());
        if (crypted_len != crypt_sz)
            ythrow yexception() << "Crypt: crypt_sz=" << crypt_sz << " != crypted_len=" << crypted_len << "' data sz=" << data.size() << " data='" << data.data() << "'";

        return Base64EncodeUrl(TStringBuf(mybuf, crypt_sz));
    }

    TString Sign(TStringBuf signBase, const TKey& key) {
        char mybuf[buf_data_len];
        Sign(signBase.data(), signBase.size(), mybuf, key);
        size_t sign_len = strlen(mybuf);
        if (!sign_len) {
            ythrow yexception() << "empty sign for sign_base='" << signBase << "'";
        }
        return TString(mybuf, sign_len);
    }

    //Takes (possibly unpadded) base64-encoded TString, pads if necessary & returns decoded TString
    TString Base64PadDecode(TString padded) {
        while (padded.size() % 4) {
            padded.append("=");
        }
        try {
            return Base64StrictDecode(padded);
        } catch (const yexception& e) {
            return TString();
        }
    }

    size_t Decrypt(const char* data, size_t data_sz, char* buf, size_t buf_sz, const TKey& key) {
        if (data_sz >= buf_sz) {
            ythrow yexception() << "ClickHandle::Decrypt data_sz=" << data_sz << "  exceeded buf_sz=" << buf_sz;
        }
        return Decrypt(data, data_sz, buf, key.second);
    }

    TString InplaceDecrypt(TString data, const TKey& key, TStringBuf keyno, B64E b64e) {
        if (b64e == FULL)
            data = Base64PadDecode(data);
        char bf_buf[buf_data_len];
        size_t dec_sz = Decrypt(data.data(), data.size(), bf_buf, buf_data_len, key);
        if (dec_sz == 0)
            ythrow yexception() << "Cannot decrypt data of size " << data.size() << "wrong dec_sz=" << dec_sz << "with keyno=" << keyno ;
        return TString(bf_buf, dec_sz);
    }

    TString Enpercent(TString cgi_val) {
        SubstGlobal(cgi_val, "&", "%26");  //
        SubstGlobal(cgi_val, "/", "%2F");  // compatibility with perl's uri_escape_utf8
        SubstGlobal(cgi_val, ";", "%3B");  //
        return TString(cgi_val);
    }

    TString SignUrlSafeClickSuggest(const TString& prefix, const TString& url, const TKey& key, const TString& keyno, TOtherParams& params) {
        //need data=$state.../sign=.../keyno=.../from=(not crypted)/etext=.../*
        TString data = Enpercent(Crypt(prefix, key));
        TString oursign = Sign(prefix, key);
        TString out = data + "/sign=" + oursign +
                        "/keyno=" + keyno +
                        "/from=" + Enpercent(params.From) +
                        "/etext=" + Enpercent(params.Etext) +
                        "/*" + url;
        return out;
    }

    TString SignUrl(const TString& prefix, const TString& url, const TKey& key, const TString& keyno, TOtherParams& params, time_t sign_time = time(nullptr), const TString& uid ="", const B64E& b64e = UNCIPHERED) {
        TString url_escaped = Enpercent(CGIEscapeRet(url));

        if (b64e != UNCIPHERED) {    //unciphered links will be /redir/, all others - jsredir/
            SubstGlobal(url_escaped, "%22", "%5C%22");      // " => \" because jsredir uses external " ", #SERP-25950
        };
        TString data = "url=" + Enpercent(url_escaped) + "&ts=" + Sprintf("%lu", sign_time) + "&uid=" + uid;
        TString new_data;

        if (b64e != UNCIPHERED) {
            new_data = Crypt(data, key);
        }

        params.UrlEscape();
        TString sign_base = prefix + "&text=" + (params.Etext.empty() ? Enpercent(params.Text) : "&etext=" + Enpercent(params.Etext)) + "&from=" + Enpercent(params.From) + new_data;

        TString oursign = Sign(sign_base, key);

        new_data = "data=" + Base64EncodeUrl(new_data) + "&sign=" + oursign + "&keyno=" + keyno;
        if (!params.Etext.empty()) {
            params.Text = "";
        }

        if (b64e != UNCIPHERED) {
            new_data = "from=" + Enpercent(params.From) + "&text=" + Enpercent(params.Text) + "&etext=" +
                       Enpercent(params.Etext) + "&uuid=" + params.Uid + "&state=" + Enpercent(Crypt(prefix, key)) + "&" +
                       (params.PrefixConst.empty() ? "" : "&cst=" + Enpercent(Crypt(params.PrefixConst, key)) + "&") +
                       new_data + "&b64e=" + ToString<ui32>(b64e) + (params.Ref.empty() ? "" : "&ref=" + Enpercent(Crypt(params.Ref, key)));
        }

        return new_data;
    }

    TUriAtPair DefineUrlType (const TString& query_string) {
        static THandlerMap handlers = {
            {"/jsredir?", AT_JSREDIR},
            {"/jsredir2?", AT_JSREDIR},
            {"/httpredir?", AT_HTTPREDIR},
            {"/redir/",   AT_REDIRECT},
            {"/safeclick/", AT_SAFECLICK},
            {"/click/", AT_IMAGE},
            {"/counter/", AT_IMAGE},
            {"/jclck/", AT_JAVASCRIPT},
        };

        TUriAtPair res;
        res.second = UNDEFINED;

        for (size_t pos = 0; pos < query_string.size(); pos++) {
            if (query_string[pos] != '/') {
                continue;
            }
            for (const auto& handler : handlers) {
                if (query_string.substr(pos, handler.first.size()) == handler.first) {
                    TString clck_uri = query_string;
                    clck_uri.replace(0, pos + handler.first.size(), "");
                    res.first = clck_uri;
                    res.second = handler.second;
                    return res;
                }
            }
        }

        return res;
    }

    TString GetInstallationInfo(const TString& clck_uri) {
        constexpr TStringBuf pathStringBuf = "installation_info=";
        for (const auto& it : StringSplitter(clck_uri).Split('/')) {
            const TStringBuf kv = it.Token();
            if (kv.StartsWith(pathStringBuf)) {
                return TString{kv.SubStr(pathStringBuf.size())};
            }
        }
        return "";
    }

///jsredir?url=http%3A%2F%2Fwww.google.ru%2Fsearch%3Fie%3DUTF-8%26hl%3Dru%26q%3Dc%252B%252B&s=s%3D12345+++&from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&etext=etext&sign=ecf20fd26cb0514b3ae058fb75bac02e
//&cst=AiuY0DBWFJ5fN_r-AEszkzOqYvuz11a_jFhQvH51613bv_JiUG1MlIbI_6uiuyXclxECqgEgRHC99tBGIWw8QW8RFuK40iA5EmKMZra68Jvz5IMjYqjMkgXUxsguzDmB-rh9afLvFC1NeOkUqmgzfKhiE0OopHzsYos1hcG4O_1O2cldENWeg-BjtiksJu
///7aUKoa3sqgV5Nn53mPptNRmXbLYyWB2a5hh2o2_6FWeXAttsucVRvaHcE88vhR6XZAq5pFopFxc2abbp7g2YQaUJF-cAgOwWATiIt-AAIfwp3cZJzq_J_zwGpXcEdfJ0IzxvCmLIF1a5dj-4LFNalM7VueaFOfr08e8HgNAuJyVQM1e72M_14sGLU3pIXUNX4lr
//:wAiOikD6P0cKi6xdqwyMtw,,&keyno=0&b64e=3&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kphf8eGbhlTpTiSiuIot-Fkw,,

    TUnsignedUrl UnsignUrl(const TString& query_string, ISignKeys* SignKeys, TUriAtPair* at_ptr) {
        bool is_safeclick = false;
        TUnsignedUrl res;
        TCgiParameters mycgi;

        TUriAtPair uri_at = at_ptr ? *at_ptr : DefineUrlType(query_string);
        TString clck_uri = uri_at.first;
        if (uri_at.second == AT_SAFECLICK) {
            is_safeclick = true;
            SubstGlobal(clck_uri, '/', '&');
        }
        mycgi.Scan(clck_uri);
        TString keyno = mycgi.Get("keyno");
        res.Key_n = "0";
        if (!keyno.empty()) {
            res.Key_n = keyno;
            res.Key_n_orig = res.Key_n;
        }

        if (res.Key_n == ROBOT_KEYNO) {
            res.Key_n = "0";
            res.Rbt   = true;
        }

        TString installationInfoBase64 = mycgi.Get("installation_info");
        if (!installationInfoBase64) {
            installationInfoBase64 = GetInstallationInfo(clck_uri);
        }
        if (installationInfoBase64) {
            try {
                const TString installationInfo = Base64StrictDecode(installationInfoBase64);
                TStringStream installationInfoStream(installationInfo);
                NJson::TJsonValue installationInfoJson = NJson::ReadJsonTree(&installationInfoStream, true);
                res.Geo = installationInfoJson["geo"].GetString();
                res.Contour = installationInfoJson["ctype"].GetString();  //  be careful here: everywhere else this is called contour
            } catch (const yexception&) {
                res.Geo = "";
                res.Contour = "";
            }
        }

        res.ExtParams = mycgi.Get("ext_params");
        if (res.ExtParams) {
            UrlUnescape(res.ExtParams);
        }

        NSignUrl::TKey emptyKey;
        const NSignUrl::TKey* key{&emptyKey};
        key = &SignKeys->GetKeyData(res.Key_n);
        TString data = mycgi.Get("data");
        res.Sign = mycgi.Get("sign");
        if (res.Sign.empty()) {
            res.Clck_uri = clck_uri;
            return res;
        }

        try {
            TString b64e = mycgi.Get("b64e");
            ui32 val = FromString<ui32>(b64e);
            if (val < UNCIPHERED || val > BAOBAB) {
                val = UNCIPHERED;
            }
            res.B64e = static_cast<B64E>(val);
        } catch (...) {
            res.B64e = UNCIPHERED;
        }

        res.OtherParams.PreferRef = mycgi.Has("rp", "1");

        TString ref = mycgi.Get("ref");
        if (ref) {
            try {
                res.OtherParams.RefEnc = ref;
                if  (res.B64e != UNCIPHERED || is_safeclick) {
                    ref = InplaceDecrypt(ref, *key);
                }
                res.OtherParams.Ref = ref;
            } catch (const yexception&) {
                res.OtherParams.Ref = "_WRONG_REF_NOT_HTTP_PREFIXED_";
                res.OtherParams.RefEnc = "";
            }
        }

        if (uri_at.second == AT_JSREDIR || uri_at.second == AT_REDIRECT || uri_at.second == AT_HTTPREDIR) {
            res.Prefix = mycgi.Get("state");
            res.OtherParams.Text = mycgi.Get("text");
            res.OtherParams.Etext = mycgi.Get("etext");
            res.OtherParams.From = mycgi.Get("from");

            TString cst = mycgi.Get("cst");
            TString url = mycgi.Get("url");
            TCgiParameters unesc;
            unesc.ScanAddUnescaped(clck_uri.data());
            TString text = unesc.Get("text");
            TString etext = unesc.Get("etext");
            TString from = unesc.Get("from");
            res.Urlesc = unesc.Get("url");
            res.OtherParams.Uid = unesc.Get("uid");
            if (res.B64e == BAOBAB) {
                try {
                    res.Ts = FromString<time_t>(unesc.Get("ts"));
                } catch (...) {}
            }
            res.S = unesc.Get("s");
            res.OtherParams.FromEsc = from;
            if (uri_at.second == AT_JSREDIR || uri_at.second == AT_HTTPREDIR) {
                SubstGlobal(clck_uri, "&data&", "&data=&"); //SERP-36632
            }

            size_t end1 = clck_uri.find("?data=");
            if (end1 == TString::npos) {
                end1 = clck_uri.find("*data=");
                if (end1 != TString::npos)
                    end1++;
                else {
                    end1 = clck_uri.find("&data=");
                }
                if (end1 == TString::npos) {
                    end1 = clck_uri.find("data=");
                }
            }
            if (uri_at.second == AT_JSREDIR || uri_at.second == AT_HTTPREDIR) {
                res.Jsredir = true;
                if  (res.B64e != UNCIPHERED && res.B64e != BAOBAB) {
                    res.PrefixEnc = res.Prefix;
                    res.Prefix = InplaceDecrypt(res.Prefix, *key);
                }
                if (end1 != TString::npos) {
                    clck_uri.replace(0, end1, res.Prefix);
                }
            } else if (uri_at.second == AT_REDIRECT && end1 != TString::npos ) {
                res.Prefix = TString(clck_uri.data(), 0, end1);
                if (clck_uri[end1] == '?' || clck_uri[end1] == '*') {
                    end1++;
                }
                clck_uri.replace (0, end1, "");
                mycgi.Scan(clck_uri);
                data = mycgi.Get("data");
                if (res.B64e != UNCIPHERED) {
                    res.PrefixEnc = res.Prefix;
                    res.Prefix = InplaceDecrypt(res.Prefix, *key);
                }
            }
            if (cst) {
                if (res.B64e != UNCIPHERED) {
                    try {
                        cst = InplaceDecrypt(cst, *key);
                    } catch (...) {
                        cst.clear();
                        res.ParsingSemiFailed = true;
                    }
                }
                res.OtherParams.PrefixConst = cst;
            }

            res.Clck_uri = clck_uri;
            if (res.B64e == SEMI || res.B64e == FULL )
                 data = Base64PadDecode(data);

            TString sign_base = ((res.B64e != BAOBAB) ?
                                 res.Prefix + patch_sign_base(uri_at.second, text, etext, from) + ((res.B64e != SHORTENED) ? data : "url=" + res.Urlesc)
                               : "url=" + res.Urlesc + "&s=" + res.S + "&from=" + from +
                                 (text.empty() ? "" : "&text=" + text) + "&etext=" +
                                 etext + (res.OtherParams.Uid.empty() ? "" :"&uid=" + res.OtherParams.Uid) +
                                 "&ts=" + Sprintf("%lu", res.Ts));

            if (!Verify(res.Sign.data(), sign_base.data(), sign_base.size(), *key)) {
                return res;
            } else {
                res.Verified = true;
                size_t end2 = res.Prefix.find("/PROXY=1/");
                if (end2 != TString::npos)
                    res.Proxy = true;
                if (!data.empty()) {
                    if (res.B64e != UNCIPHERED) {
                        data = InplaceDecrypt(data, *key, res.Key_n, res.B64e);
                    }

                    mycgi.Scan(data);
                    res.OtherParams.Uid = mycgi.Get("uid");
                    res.Url = mycgi.Get("url");
                    try {
                        res.Ts = FromString<time_t>(TString(mycgi.Get("ts")));
                    } catch (const yexception&) {
                        res.Ts = 0;
                    }
                } else if (res.B64e == SHORTENED) {
                    res.Url = url;
                    TVector<TString> slashed;
                    StringSplitter(res.OtherParams.PrefixConst).Split('/').SkipEmpty().Collect(&slashed);
                    for (TVector<TString>::iterator fld = slashed.begin(); fld < slashed.end(); fld++) {
                        if (fld->StartsWith(TStringBuf("ts=")))
                            res.Ts = FromString<time_t>(TString(fld->c_str() + 3));
                        if (fld->StartsWith(TStringBuf("u=")))
                            res.OtherParams.Uid =  TString(fld->c_str() + 2);
                    }
                }
            }
        } else if (uri_at.second == AT_SAFECLICK) {
            data = InplaceDecrypt(data, *key);
            if (Verify(res.Sign.data(), data.data(), data.size(), *key)) {
                res.Verified = true;
                res.Prefix = data;
            }
        }

        UrlEscape(res.Url);
        return res;
    }
}
