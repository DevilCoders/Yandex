#pragma once

#include "keyholder.h"

#include <util/generic/string.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/printf.h>
#include <util/string/subst.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/system/types.h>
#include <util/datetime/base.h>
#include <util/generic/hash.h>

#define SU_SIGN_SZ 33

namespace NSignUrl {
    struct TOtherParams {
        TString Text = "";
        TString From = "";
        TString FromEsc = "";
        TString Uid = "";
        TString Etext = "";
        TString Ref = "";
        TString RefEnc = "";
        bool PreferRef = false;
        TString PrefixConst = "";
        void UrlEscape() {
            ::UrlEscape(this->From);
            ::UrlEscape(this->Etext);
            if (!this->Etext.empty()) {
                this->Text = "";
            } else {
                ::UrlEscape(this->Text);
            }
        };
        TOtherParams UrlEscapeCopy() const {
             TOtherParams copy(*this);
             copy.UrlEscape();
             return copy;
        };
        void Dump(IOutputStream& out = Cerr) const {
            out << "\t\tText\t=>\t" << this->Text << Endl;
            out << "\t\tFrom\t=>\t" << this->From << Endl;
            out << "\t\tFromEsc\t=>\t" << this->FromEsc << Endl;
            out << "\t\tUid\t=>\t" << this->Uid << Endl;
            out << "\t\tEtext\t=>\t" << this->Etext << Endl;
            out << "\t\tRef\t=>\t" << this->Ref << Endl;
            out << "\t\tRefEnc\t=>\t" << this->RefEnc << Endl;
            out << "\t\tPreferRef\t=>\t" << this->PreferRef << Endl;
            out << "\t\tPrefixConst\t=>\t" << this->PrefixConst << Endl;
        }
    };

    enum B64E {
        UNCIPHERED,
        SEMI,
        FULL,
        SHORTENED,
        BAOBAB
    };

    struct TUnsignedUrl {
        bool Verified = false;
        TString Prefix = "";
        TString PrefixEnc = "";
        TString Url = "";
        TString Urlesc = "";
        TString Clck_uri = "";
        TString S = "";
        TString Geo = "";
        TString Contour = "";
        TString ExtParams = "";  // add extra param to jsredir url. CLICKDAEMON-210
        time_t Ts = 0;
        bool Jsredir = false;
        B64E B64e = UNCIPHERED;
        TString Sign = "";
        TString Key_n = "0";
        TString Key_n_orig = "0";
        bool Rbt  = false;
        bool Proxy = false;
        TOtherParams OtherParams;
        bool ParsingSemiFailed = false; // Is true when it's possible to make a redirect for user, but some parts of link are broken

        void Dump(IOutputStream& out = Cerr) const {
            out << "\tVerified\t=> " << this->Verified << Endl;
            out << "\tPrefix\t=> " << this->Prefix << Endl;
            out << "\tPrefixEnc\t=> " << this->PrefixEnc << Endl;
            out << "\tUrl\t=> " << this->Url << Endl;
            out << "\tUrlesc\t=> " << this->Urlesc << Endl;
            out << "\tClck_uri\t=> " << this->Clck_uri << Endl;
            out << "\tTs\t=> " << this->Ts << Endl;
            out << "\tS\t=> " << this->S << Endl;
            out << "\tGeo\t=> " << this->Geo << Endl;
            out << "\tContour\t=> " << this->Contour << Endl;
            out << "\tExtParams\t=> " << this->ExtParams << Endl;
            out << "\tjsredir\t=> " << this->Jsredir << Endl;
            out << "\tb64e\t=> " << (ui32)this->B64e << Endl;
            out << "\tsign\t=> " << this->Sign << Endl;
            out << "\tkey_n\t=> " << this->Key_n << Endl;
            out << "\tRbt\t=> " << this->Rbt << Endl;
            out << "\tOtherParams\t=> { " << Endl;
            this->OtherParams.Dump(out);
            out << "\t]" << Endl;
        }
    };

    enum ANSWER_TYPE {
        AT_REDIRECT,
        AT_IMAGE,
        AT_JAVASCRIPT,
        AT_SAFECLICK,
        AT_JSREDIR,
        AT_HTTPREDIR,
        UNDEFINED
    };

    typedef TMap<TString, ANSWER_TYPE> THandlerMap;
    typedef std::pair<TString, ANSWER_TYPE> TUriAtPair;

    void Sign(const char* in, size_t in_sz, char* out, const char* key, size_t key_sz);
    void Sign(const char* in, size_t in_sz, char* out, const NSignUrl::TKey& key);
    TString Sign(TStringBuf signBase, const NSignUrl::TKey& key);
    TString SignUrlSafeClickSuggest(const TString& prefix, const TString& url, const TKey& key, const TString& keyno, TOtherParams& params);
    TString SignUrl(const TString& prefix, const TString& url, const NSignUrl::TKey& key, const TString& keyno, TOtherParams& params, time_t sign_time, const TString& uid, const B64E& b64e);

    TUnsignedUrl UnsignUrl(const TString& query_string, ISignKeys* SignKeys, TUriAtPair* at_ptr = nullptr);

    size_t Crypt(const char* in, char* out, const char* key, size_t key_sz);
    size_t Crypt(const char* in, char* out, const NSignUrl::TKey& key);
    size_t Crypt(const char* in, char* out, const BF_KEY& key);
    TString Crypt(const TString& data, const NSignUrl::TKey& key);

    size_t CryptBinary(const char* in, size_t in_sz, char* out, const char* key, size_t key_sz);
    size_t CryptBinary(const char* in, size_t in_sz, char* out, const NSignUrl::TKey& key);
    size_t CryptBinary(const char* in, size_t in_sz, char* out, const BF_KEY& key);

    size_t Decrypt(const char* in, size_t in_sz, char* out, const char* key, size_t key_sz);
    size_t Decrypt(const char* in, size_t in_sz, char* out, const NSignUrl::TKey& key);
    size_t Decrypt(const char* in, size_t in_sz, char* out, const BF_KEY& key);

    bool Verify(const char* sign, const char* txt_unsigned, size_t txt_sz, const char* key, size_t key_sz);
    bool Verify(const char* sign, const char* txt_unsigned, size_t txt_sz, const NSignUrl::TKey& key);

    TString Enpercent(TString cgi_val);
    TUriAtPair DefineUrlType (const TString& query_string);

    TString Base64PadDecode(TString padded);
    TString InplaceDecrypt(TString data, const TKey& key, TStringBuf keyno="-1", B64E b64e=FULL);
    static inline size_t SignSize() {
        return SU_SIGN_SZ;
    }

    inline TString patch_sign_base(ANSWER_TYPE at, const TString& text, const TString& etext, const TString& from) {
        return ((at == AT_JSREDIR || at == AT_HTTPREDIR) ? "&text=" + text + (!etext.empty() ? "&etext=" + etext : "") + "&from=" + from : "");
    };

    static inline size_t CryptedSize(size_t datasize) {
        return (datasize % 8) ? 8 * ((datasize / 8) + 1) : datasize;
    }
}
