#include "qd_ut_utils.h"

#include <kernel/querydata/cgi/qd_cgi_strings.h>
#include <kernel/querydata/cgi/qd_cgi_utils.h>
#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/yexception.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/printf.h>
#include <util/system/defaults.h>

namespace NQueryData {
    namespace NTests {

        TQueryData& BuildQDSimple(TQueryData& qd, const NSc::TValue& data) {
            const NSc::TDict& src = data.GetDict();
            for (NSc::TDict::const_iterator srcit = src.begin(); srcit != src.end(); ++srcit) {
                const NSc::TDict& key = srcit->second.GetDict();
                for (NSc::TDict::const_iterator keyit = key.begin(); keyit != key.end(); ++keyit) {
                    const NSc::TDict& fac = keyit->second.GetDict();
                    TSourceFactors* sf = qd.AddSourceFactors();
                    sf->SetSourceName(TString{srcit->first});
                    sf->SetSourceKey(TString{keyit->first});
                    sf->SetSourceKeyType(KT_QUERY_EXACT);

                    for (NSc::TDict::const_iterator facit = fac.begin(); facit != fac.end(); ++facit) {
                        if (facit->first == "JSON") {
                            sf->SetJson(facit->second.ToJson(true));
                        } else if (facit->first == "VER") {
                            sf->SetVersion(facit->second.GetIntNumber());
                        } else if (facit->first == "RT") {
                            sf->SetRealTime(facit->second.GetIntNumber());
                        } else {
                            TFactor* f = sf->AddFactors();
                            f->SetName(TString{facit->first});
                            if (facit->second.IsNumber()) {
                                f->SetFloatValue(facit->second.GetNumber());
                            } else if (facit->second.IsString()) {
                                f->SetStringValue(TString{facit->second.GetString()});
                            }
                        }
                    }
                }
            }
            return qd;
        }

        static EKeyType CheckKeyType(TStringBuf sb) {
            if (!sb)
                ythrow yexception() << "invalid key type: '" << sb << "'";
            EKeyType kt = (EKeyType)GetNormalizationTypeFromName(sb);
            if (kt < 0 || kt >= KT_COUNT) {
                if (!EKeyType_Parse(TString{sb}, &kt))
                    ythrow yexception() << "invalid key type: '" << sb << "'";
            }
            return kt;
        }

        TFactor& BuildF(TFactor& f, const NSc::TValue& data) {
            f.SetName(TString{data["Name"].GetString()});
            if (data.Has("StringValue"))
                f.SetStringValue(data["StringValue"].ForceString());
            if (data.Has("IntValue"))
                f.SetIntValue(data["IntValue"].ForceIntNumber());
            if (data.Has("FloatValue"))
                f.SetFloatValue(data["FloatValue"].ForceNumber());
            if (data.Has("BinaryValue"))
                f.SetBinaryValue(data["BinaryValue"].ForceString());
            return f;
        }

        TSourceFactors& BuildSF(TSourceFactors& sf, const NSc::TValue& data) {
            sf.SetSourceName(TString{data["SourceName"].GetString()});

            if (data.Has("TrieName"))
                sf.SetTrieName(TString{data["TrieName"].GetString()});

            if (data.Has("Common"))
                sf.SetCommon(data["Common"].GetNumber());

            if (!sf.GetCommon()) {
                if (data.Has("SourceKey")) {
                    sf.SetSourceKey(TString{data["SourceKey"].GetString()});
                }

                if (data.Has("SourceKeyType")) {
                    sf.SetSourceKeyType(CheckKeyType(data["SourceKeyType"].GetString()));
                }

                if (data.Has("SourceKeyTraits")) {
                    sf.MutableSourceKeyTraits()->SetIsPrioritized(data["SourceKeyTraits"]["IsPrioritized"].GetNumber());
                    sf.MutableSourceKeyTraits()->SetMustBeInScheme(data["SourceKeyTraits"]["MustBeInScheme"].GetNumber());
                }
            }

            sf.SetVersion(data.Get("Version").GetNumber());

            {
                const NSc::TArray& arr = data["SourceSubkeys"].GetArray();
                for (const auto& ssk : arr) {
                    TSourceSubkey* sk = sf.AddSourceSubkeys();
                    sk->SetKey(TString{ssk.Get("Key").GetString()});
                    sk->SetType(CheckKeyType(ssk.Get("Type")));
                    if (ssk.Has("Traits")) {
                        sk->MutableTraits()->SetIsPrioritized(ssk.Get("Traits")["IsPrioritized"].GetNumber());
                        sk->MutableTraits()->SetMustBeInScheme(ssk.Get("Traits")["MustBeInScheme"].GetNumber());
                    }
                }
            }

            if (data["MergeTraits"].Has("Priority")) {
                sf.MutableMergeTraits()->SetPriority(data["MergeTraits"]["Priority"].GetNumber());
            }

            if (data.Has("Json")) {
                sf.SetJson(data["Json"].ToJson(true));
            }

            {
                const NSc::TArray& arr = data["Factors"].GetArray();
                for (const auto& fac : arr) {
                    TFactor* f = sf.AddFactors();
                    BuildF(*f, fac);
                }
            }

            return sf;
        }

        TQueryData& BuildQD(TQueryData& qd, const NSc::TValue& data) {
            const NSc::TArray& arr = data["SourceFactors"].GetArray();
            for (NSc::TArray::const_iterator it = arr.begin(); it != arr.end(); ++it) {
                BuildSF(*qd.AddSourceFactors(), *it);
            }
            return qd;
        }

        static void SetOrAppend(TString& buf, TStringBuf prefix, TStringBuf suffix, bool first) {
            if (first) {
                buf = suffix;
            } else {
                buf.assign(prefix).append('\t').append(suffix);
            }
        }

        template <typename TFactor>
        static void SetFactor(TFactor* f, const NSc::TValue& value) {
            if (value.IsString()) {
                f->SetStringValue(TString{value.GetString()});
            } else if (value.IsNumber()) {
                double n = value.GetNumber();
                if (n == (ui64)n)
                    f->SetIntValue((ui64)n);
                else
                    f->SetFloatValue(n);
            }
        }

        struct TFillValuesCtx {
            NSc::TValue FMap;

            TMockTrie::TPtr Trie;

            TFillValuesCtx()
                : Trie(new TMockTrie)
            {}

            void AddKeys(const NSc::TValue& sub, TStringBuf prefix, ui32 cnt, ui32 firstCnt) {
                TMockTrie* trie = (TMockTrie*)Trie.Get();
                if (!!prefix && !trie->Prefixes.contains(prefix))
                    trie->Prefixes.insert(TString{prefix});

                if (sub.IsString()) {
                    Y_ENSURE(!cnt, "string unexpected");
                    trie->Data[prefix].SetKeyRef(TString{sub.GetString()});
                    return;
                }

                Y_ENSURE(sub.IsDict(), "dict expected");

                TString buf;
                for (const auto& item : sub.GetDict()) {
                    if (cnt) {
                        SetOrAppend(buf, prefix, item.first, cnt == firstCnt);
                        AddKeys(item.second, buf, cnt - 1, firstCnt);
                    } else {
                        if ("Json" == item.first) {
                            trie->Data[prefix].SetJson(item.second.ToJson(true));
                            continue;
                        }

                        if ("Version" == item.first) {
                            trie->Data[prefix].SetVersion(item.second.GetIntNumber(-1));
                            continue;
                        }

                        int id = FMap.Get(item.first).GetIntNumber(-1);

                        Y_ENSURE(id >= 0, "invalid factor: '" << item.first << "' (" << prefix << ")");

                        TRawFactor* rf = trie->Data[prefix].AddFactors();
                        rf->SetId(id);
                        SetFactor(rf, item.second);
                    }
                }
            }
        };

        TSource::TPtr MakeFakeSource(const NSc::TValue& json) {
            TSource::TPtr res = new TSource;
            TFileDescription fd;
            fd.SetSourceName(TString{json.Get("SourceName").GetString()});
            fd.SetVersion(json.Get("Version").GetIntNumber());
            fd.SetIndexingTimestamp(json.Get("IndexingTimestamp").GetIntNumber());

            if (json.Has("Shards")) {
                fd.SetShards(json.Get("Shards").GetIntNumber(-1));
            }

            if (json.Has("ShardNumber")) {
                fd.SetShardNumber(json.Get("ShardNumber").GetIntNumber(-1));
            }

            if (json.Has("HasKeyRef")) {
                fd.SetHasKeyRef(json.Get("HasKeyRef").IsTrue());
            }

            const NSc::TArray& keys = json.Get("SourceKeys").GetArray();

            Y_ENSURE(!keys.empty(), "no SourceKeys array was found");

            fd.SetKeyType(CheckKeyType(keys[0].GetString()));

            for (ui32 i = 1, sz = keys.size(); i < sz; ++i) {
                fd.AddSubkeyTypes(CheckKeyType(keys[i].GetString()));
            }

            TFillValuesCtx ctx;

            {
                TStringBufs fnames;

                const NSc::TDict& factors = json.Get("FactorNames").GetDict();

                for (const auto& fact : factors) {
                    fnames.push_back(fact.first);
                    ctx.FMap.GetOrAdd(fnames.back()).SetNumber(fnames.size() - 1);
                }

                for (const auto& fname : fnames) {
                    fd.AddFactorsMeta()->SetName(TString{fname});
                }
            }

            const NSc::TDict& cfacts = json.Get("CommonFactors").GetDict();

            for (const auto& cfact : cfacts) {
                TFactor* f = fd.AddCommonFactors();
                f->SetName(TString{cfact.first});
                SetFactor(f, cfact.second);
            }

            if (json.Has("CommonJson")) {
                fd.SetCommonJson(json.Get("CommonJson").ToJson(true));
            }

            if (json.Has("HasJson")) {
                fd.SetHasJson(json.Get("HasJson").GetNumber());
            }

            ctx.AddKeys(json.Get("SourceData"), TStringBuf(), keys.size(), keys.size());

            res->SetFile(TStringBuilder() << fd.GetSourceName() << ".trie." << fd.GetVersion(), json.Get("FileTimestamp").GetIntNumber());
            res->SetDescr(fd);
            res->SetTrie(ctx.Trie);

            return res;
        }

        void MockQueryDataRequest(TRequestRec& rec, TStringBuf req) {
            rec.UserQuery = rec.UserId = rec.YandexTLD = req;

            TStringBuf t;
            while (NextToken(t, req, ',')) {
                rec.DocItems.AddDocId(t);
                rec.DocItems.AddSnipDocId(t);
                rec.DocItems.AddCateg(t);
                rec.UserRegions.push_back(TString{t});
            }

            rec.SerpType = SERP_TYPE_MOBILE;
        }

    }
}
