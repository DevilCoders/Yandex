#include "cry.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>

#include <util/string/printf.h>

#include <util/generic/cast.h>

#include <library/cpp/uri/uri.h>

#include <re2/re2.h>

#include <cstdint>
#include <ostream>


namespace NAntiAdBlock {

    //empty result
    TDecryptResult result_none;
    // normal url should look like this
    RE2 url_re("(?:https?:)?//(?:[\\w.-])+.*?");
    // Регулярка для криптованных ссылок
    // Первая группа - длина зашифрованной ссылки, вторая - seed для рандомизации ключа шифрования,
    // третья - криптованная часть урла + не криптованная
    const TString pattern_part = Sprintf("([A-Za-z0-9]{%d})(/)([a-z0-9]{%d})(.+)", URL_LENGTH_PREFIX_LENGTH, SEED_LENGTH);
    const TString pattern_part_without_group = Sprintf("[A-Za-z0-9]{%d}/[a-z0-9]{%d}.+", URL_LENGTH_PREFIX_LENGTH, SEED_LENGTH);

    const TString GetKey(TStringBuf secret, TStringBuf data) {
        char result[EVP_MAX_MD_SIZE];
        ui32 resultSize = 0;

        if (!::HMAC(EVP_sha512(), secret.data(), secret.size(),
                    reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                    reinterpret_cast<unsigned char*>(result), &resultSize))
        {
            return TString();
        }

        return TString(result, resultSize);
    }

    const TString DecryptBase64(const TStringBuf data) {
        if (data.length() % 4 == 0) {
            return Base64Decode(data);
        }
        // padding to 4
        TString result = data.data();
        result += TString(4 - (data.length() % 4), '=');
        return Base64Decode(result);
    }

    const TString DecryptXor(const TStringBuf data, const TStringBuf key) {
        TString bdata = DecryptBase64(data);
        for (size_t i = 0; i < bdata.length(); i++) {
            bdata[i] = bdata[i] ^ key[i%key.length()];
        }
        return bdata;
    }

    int DecryptNumber(const TStringBuf seed, const TStringBuf url_length_prefix) {
        int k = seed[0];
        int b = seed[seed.length() - 1];
        int decrypted = 0;
        for (size_t i = 0; i < url_length_prefix.size(); i++) {
            if (isdigit(url_length_prefix[i])) {
                decrypted = 10 * decrypted + (url_length_prefix[i] - '0');
            }
        }
        decrypted = (decrypted - b) / k;
        return decrypted;
    }

    bool SymbolForRemove(char ch){
        switch (ch){
            case '/':
            case '?':
            case '.':
            case '=':
            case '&':
            case '!':
            case '$':
            case '*':
            case '~':
            case '[':
            case ']':
                return true;
            default:
                return false;
        }
    }

    const TPartsCryptUrl ResplitUsingLength(int length, const TStringBuf& cry_part, const TStringBuf& noncry_part) {
        TString all_part = TString::Uninitialized(cry_part.length() + noncry_part.length());
        all_part = "";
        all_part.append(cry_part.data(), cry_part.length());
        all_part.append(noncry_part.data(), noncry_part.length());

        TString cry = TString::Uninitialized(all_part.size());

        int length_cry = 0;
        int cnt_remove = 0;
        for (size_t i = 0; i < all_part.size(); i++) {
            if (length_cry == length) break;
            if (!SymbolForRemove(all_part[i])) {
                cry[length_cry] = all_part[i];
                length_cry++;
            } else {
                cnt_remove++;
            }
        }
        if (length_cry < length) {
            ythrow yexception() << "Failed resplit using length";
        }

        cry.resize(length_cry);
        TString noncry = TStringBuf(all_part.begin(), all_part.size()).SubStr(length + cnt_remove).data();
        return std::make_pair(cry, noncry);
    }

    bool IsCryptedUrl(const TStringBuf& crypted_url, const TStringBuf& crypt_prefix) {
        if (crypted_url.length() <= URL_LENGTH_PREFIX_LENGTH + SEED_LENGTH + 1) {
            return false;
        }
        TString pattern = Sprintf("(?:https?:)?//[\\w.-]+(?:%s)%s", crypt_prefix.data(), pattern_part_without_group.c_str());
        RE2 crypted_url_pattern(pattern);
        return RE2::FullMatch(crypted_url.data(), crypted_url_pattern);
    }

    const TDecryptResult DecryptUrlXorWithLength(const TStringBuf& crypted_url, const TStringBuf& secret_key,
                                                        const TStringBuf& crypt_prefix, int is_trailing_slash_enabled)
    {
        if (crypted_url.length() <= URL_LENGTH_PREFIX_LENGTH + SEED_LENGTH + 1) {
            return result_none;
        }

        // разбиваем урл на части по регулярке
        TString pattern = Sprintf("(?:%s)%s", crypt_prefix.data(), pattern_part.c_str());
        RE2 decrypt_url_xor_with_length(pattern);

        TString prefix = TString::Uninitialized(URL_LENGTH_PREFIX_LENGTH);
        TString seed = TString::Uninitialized(SEED_LENGTH);
        TString cry_part = TString::Uninitialized(crypted_url.length() - URL_LENGTH_PREFIX_LENGTH - SEED_LENGTH - 1);

        if (!RE2::FullMatch(crypted_url.data(), decrypt_url_xor_with_length, &prefix, (void*)NULL, &seed, &cry_part)) {
            return result_none;
        }

        int length_cry_part = DecryptNumber(seed, prefix);

        //get_key
        TString key = GetKey(secret_key, seed);
        if (key.size() == 0) {
            ythrow yexception() << "Failed get key";
        }

        const TPartsCryptUrl resplitted = ResplitUsingLength(length_cry_part, cry_part, "");

        TString decrypted = DecryptXor(resplitted.first, key);
        // проверяем, что раcшифрованная строка похожа на урл
        if (!RE2::FullMatch(decrypted, url_re)) {
            return result_none;
        }

        TString noncry_part = TString::Uninitialized(resplitted.second.size());
        if (resplitted.second.size() > 0) {
            if (is_trailing_slash_enabled && resplitted.second[0] == '/') {
                noncry_part = TStringBuf(resplitted.second.begin(), resplitted.second.size()).SubStr(1).data();
            } else {
                noncry_part = resplitted.second.data();
            }
        } else {
            noncry_part = "";
        }

        //отрезаем пришитый мусор
        RE2 remove_expander_re(" [a-zA-Z]*$");
        RE2::Replace(&decrypted, remove_expander_re, "");

        // Выпаршиваем и удаляем зашитый в ссылку origin
        RE2 origin_re("__AAB_ORIGIN(.*?)__");
        TString origin;
        RE2::PartialMatch(decrypted, origin_re, &origin);
        RE2::Replace(&decrypted, origin_re, "");

        if (noncry_part.size() > 0) {
            NUri::TUri url;
            TStringBuf path;
            if (url.Parse(decrypted, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown | NUri::TFeature::FeatureAllowEmptyPath) == NUri::TState::ParsedOK) {
                path = url.GetField(NUri::TField::FieldPath);
            } else {
                path = "";
            }
            if (!path.EndsWith("/") && !SymbolForRemove(decrypted[decrypted.size() - 1]) && !SymbolForRemove(noncry_part[0]) ) {
                decrypted += '/';
            }

            switch (noncry_part[0]) {
                case '&':
                    if (decrypted.find('?') == TString::npos) {
                        decrypted += '?';
                    }
                    break;
                case '?':
                    if (decrypted.find('?') != TString::npos) {
                        noncry_part[0] = '&';
                    }
                    break;
                case '/':
                    if (path.length() > 0 && path.EndsWith('/')) {
                        noncry_part = TStringBuf(noncry_part.begin(), noncry_part.size()).SubStr(1).data();
                    }
                    break;
                default:
                    break;
            }
            decrypted.append(noncry_part.begin(), noncry_part.size());
        }

        return {decrypted, seed, origin};
    }

    const TDecryptResult DecryptUrl(const TStringBuf& crypted_url, const TStringBuf& secret_key,
                                           const TStringBuf& crypt_prefix, int is_trailing_slash_enabled)
    {
        try {
            TString cry_uri_without_host = TString::Uninitialized(crypted_url.length());
            cry_uri_without_host = "";

            NUri::TUri cry_uri;
            // отрываем схему и домен у зашифрованного урла, если они есть
            if (cry_uri.Parse(crypted_url.data(), NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown | NUri::TFeature::FeatureAllowEmptyPath) == NUri::TState::ParsedOK) {
                TStringBuf path = cry_uri.GetField(NUri::TField::FieldPath);
                TStringBuf query = cry_uri.GetField(NUri::TField::FieldQuery);
                TStringBuf frag = cry_uri.GetField(NUri::TField::FieldFrag);
                cry_uri_without_host.append(path.data(), path.length());
                if (query.length() > 0) {
                    cry_uri_without_host += '?';
                    cry_uri_without_host.append(query.data(), query.length());
                }
                if (frag.length() > 0) {
                    cry_uri_without_host += '#';
                    cry_uri_without_host.append(frag.data(), frag.length());
                }
            } else {
                cry_uri_without_host.append(crypted_url.data(), crypted_url.length());
            }


            return DecryptUrlXorWithLength(cry_uri_without_host, secret_key, crypt_prefix, is_trailing_slash_enabled);
        } catch (...) {
            throw;
        }
    }
}
