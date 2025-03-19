#pragma once

#include "qd_trie.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <library/cpp/infected_masks/masks_comptrie.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NQueryData {

    // TODO: перенести код реализаций методов в .cpp

    class TQDCategMaskCompTrie : public TQDTrie {
        using TResult = TCategMaskComptrie::TResult;

        struct TIteratorImpl : TIterator {
            bool Next() override {
                if (It.IsEmpty()) {
                    It = Begin;

                    return It != End;
                }

                if (It == End)
                    return false;

                ++It;

                return It != End;
            }

            bool Current(TString& rawkey, TString& val) const override {
                if (It.IsEmpty() || It == End)
                    return false;
                rawkey = It.GetKey();
                auto pos = rawkey.find_last_of('\t');
                if (TString::npos != pos) {
                    rawkey.replace(pos, 1, 1, ' ');
                }
                val = It.GetValue();
                return true;
            }

            TCompactTrie<char, TStringBuf>::TConstIterator Begin;
            TCompactTrie<char, TStringBuf>::TConstIterator End;
            TCompactTrie<char, TStringBuf>::TConstIterator It;
        };

        void DoInit() override {
            Trie.Init(Blob);
        }

    public:
        TString Report() const override {
            return "categ_mask_comptrie: ";
        }

        TAutoPtr<TIterator> Iterator() const override {
            THolder<TIteratorImpl> it = MakeHolder<TIteratorImpl>();
            it->Begin = Trie.Begin();
            it->End = Trie.End();
            return it.Release();
        }

        static void UpdateBansData(const TStringBuf rawkey, NJson::TJsonValue& bans) {
            if (bans.Has("sub_url_key")) {
                TString subUrlKey = bans["sub_url_key"].GetString();
                if (NJson::TJsonValue* subUrlValue = bans.GetValueByPath(subUrlKey, '.')) {
                    TString value = subUrlValue->GetString();
                    TStringBuf prefix, url;
                    rawkey.RSplit(' ', prefix, url);
                    TString urlStr(url);
                    urlStr = AddSchemePrefix(urlStr);
                    Quote(urlStr, "");
                    SubstGlobal(value, "${url}", urlStr);
                    (*subUrlValue) = value;
                }
                bans.EraseValue("sub_url_key");
            }
        }

        static TString GetInfectedMaskFromInternalKey(TString key) {
            SubstGlobal(key, ' ', '\t');
            return TCategMaskComptrie::GetInfectedMaskFromInternalKey(key);
        }

        void ProcessSingleRecord(const TResult::value_type& r, TRawQueryData& resultQD, NJson::TJsonValue& jsonBuf) const {
            TRawQueryData qd;
            Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(r.second.data(), r.second.size());
            if (!resultQD.HasVersion()) {
                resultQD = qd;
            }
            if (qd.HasJson()) {
                TStringStream ss(qd.GetJson());
                ReadJsonTree(&ss, &jsonBuf);
            }
        }

        void MergeMasksData(const TStringBuf rawkey, TResult& result, TValue& value) const {
            SortUniqueBy(result, [](const TResult::value_type& a) { return a.second; });

            TRawQueryData resultQD;
            NJson::TJsonValue bans;

            for (const auto& r : result) {
               NJson::TJsonValue single;
               ProcessSingleRecord(r, resultQD, single);
               UpdateBansData(rawkey, single);
               if (!single.IsNull()) {
                   bans.AppendValue(single);
               }
            }

            if (bans.IsArray()) {
                resultQD.SetJson(WriteJson(bans, false, true));
            }

            value.Value = (value.StrVal = resultQD.SerializeAsString());
        }

        bool Find(const TStringBuf rawkey, TValue& v) const override {
            v.Value.Clear();
            TResult result;

            if (Trie.Find(rawkey, result)) {
                MergeMasksData(rawkey, result, v);
                return true;
            } else {
                return false;
            }
        }

        bool FindExact(TStringBuf rawkey, TValue& v) const override {
            v.Value.Clear();
            TStringBuf val;

            TString rawkeystr = TString{rawkey};
            auto pos = rawkeystr.find_last_of(' ');
            if (TString::npos != pos) {
                rawkeystr.replace(pos, 1, 1, '\t');
            }
            if (Trie.FindExact(rawkeystr, val)) {
                TResult result;
                result.emplace_back(TString{rawkey}, val);
                MergeMasksData(rawkey, result, v);
                return true;
            } else {
                return false;
            }
        }

        bool CanCheckPrefix() const override { return true; }

        bool HasExactKeys() const override { return false; }

        bool HasPrefix(const TStringBuf prefix) const override {
            return !Trie.FindTails(prefix).IsEmpty();
        }

        TCategMaskComptrie Trie;
    };

}
