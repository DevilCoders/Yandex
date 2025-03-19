#pragma once

#include <kernel/querydata/cgi/qd_cgi.h>
#include <kernel/querydata/cgi/qd_request.h>
#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/server/qd_source.h>
#include <kernel/querydata/tries/qd_trie.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/json/json_prettifier.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NQueryData {
    namespace NTests {

        TQueryData& BuildQDSimple(TQueryData& qd, const NSc::TValue& data);

        inline TQueryData& BuildQDSimpleJson(TQueryData& qd, TStringBuf data) {
            return BuildQDSimple(qd, NSc::TValue::FromJson(data));
        }

        TQueryData& BuildQD(TQueryData& qd, const NSc::TValue& data);

        inline TQueryData BuildQD(const NSc::TValue& data) {
            TQueryData res;
            return BuildQD(res, data);
        }

        inline TQueryData& BuildQDJson(TQueryData& qd, TStringBuf data) {
            NSc::TValue v = NSc::TValue::FromJsonThrow(data);
            return BuildQD(qd, v);
        }

        inline TQueryData BuildQDJson(TStringBuf data) {
            return BuildQD(NSc::TValue::FromJsonThrow(data));
        }

        TSourceFactors& BuildSF(TSourceFactors& qd, const NSc::TValue& data);

        inline TSourceFactors BuildSF(const NSc::TValue& data) {
            TSourceFactors sf;
            return BuildSF(sf, data);
        }

        inline TSourceFactors BuildSFJson(TStringBuf data) {
            return BuildSF(NSc::TValue::FromJsonThrow(data));
        }

        TFactor& BuildF(TFactor& qd, const NSc::TValue& data);

        inline TFactor& BuildFJson(TFactor& qd, TStringBuf data) {
            return BuildF(qd, NSc::TValue::FromJson(data));
        }

        struct TMockTrie : TQDTrie {
            typedef THashMap<TString, TRawQueryData> TDataMock;
            typedef THashSet<TString> TPrefixMock;
            TDataMock Data;
            TPrefixMock Prefixes;

            struct TMockIterator : TIterator {
                TStringBufs Keys;
                const TDataMock* Data;
                int Pos;

                TMockIterator(const TDataMock* data)
                    : Data(data)
                    , Pos(-1)
                {
                    for (TDataMock::const_iterator it = data->begin(); it != data->end(); ++it) {
                        Keys.push_back(it->first);
                    }
                }

                bool HasCurrent() const {
                    return Pos < (int)Keys.size();
                }

                bool Next() override {
                    if (!HasCurrent())
                        return false;
                    ++Pos;
                    return HasCurrent();
                }

                bool Current(TString& rawkey, TString& val) const override {
                    if (!HasCurrent())
                        return false;
                    TDataMock::const_iterator it = Data->find(Keys[Pos]);
                    rawkey = it->first;
                    val = it->second.SerializeAsString();
                    return true;
                }
            };

            void DoInit() override {}
            TString Report() const override { return "MOCK"; }

            TAutoPtr<TIterator> Iterator() const override {
                return new TMockIterator(&Data);
            }

            bool Find(TStringBuf rawkey, TValue& res) const override {
                TDataMock::const_iterator it = Data.find(rawkey);
                if (it == Data.end())
                    return false;
                TString qd = it->second.SerializeAsString();
                res.Buffer1.Assign(qd.begin(), qd.end());
                res.Value = TStringBuf(res.Buffer1.Data(), res.Buffer1.Size());
                return true;
            }

            bool CanCheckPrefix() const override { return !Prefixes.empty(); }
            bool HasPrefix(TStringBuf p) const override { return Prefixes.contains(p); }

            ui64 Size() const override {
                size_t sz = 0;
                for (TDataMock::const_iterator it = Data.begin(); it != Data.end(); ++it) {
                    sz += it->first.size() + it->second.SerializeAsString().size();
                }
                return sz;
            }

        };

        TSource::TPtr MakeFakeSource(const NSc::TValue& json);

        inline TSource::TPtr MakeFakeSourceJson(TStringBuf json) {
            return MakeFakeSource(NSc::TValue::FromJson(json));
        }

        void MockQueryDataRequest(TRequestRec& rec, TStringBuf req);

    }
}
