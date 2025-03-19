#include "ruoblivion.h"
#include "tb_names.h"

#include <kernel/qtree/richrequest/lemmas.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash_set.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/subst.h>

namespace NTotalBan {
    static wchar16 ASTERISK[] = {'*', 0};

    namespace NRuOblivion {
        static TUtf16String NormalizeToken(TUtf16String token) {
            token.to_lower();
            SubstGlobal(token, (wchar16)L'\u0451', (wchar16)L'\u0435'); // ё -> e
            return token;
        }

        class TQueryForms : public TFormsProcessor, public THashSet<TUtf16String> {
        public:
            void OnForm(const TUtf16String& form, bool&) override {
                insert(NormalizeToken(form));
            }
        };

        using TNameTokens = TVector<TUtf16String>;

        static void PrepareNameTokens(TNameTokens& nameTokens, TStringBuf name) {
            TUtf16String wname = UTF8ToWide(name);
            ui32 last = 0;
            for (ui32 i = 0, sz = wname.size(); i <= sz; ++i) {
                if (i == sz || !IsAlpha(wname[i])) {
                    if (i < sz && wname[i] == '*') {
                        nameTokens.push_back(ASTERISK);
                    }

                    if (last < i) {
                        nameTokens.push_back(NormalizeToken(wname.substr(last, i - last)));
                    }

                    last = i + 1;
                }
            }
        }

        static bool NameMatchesQuery(const TQueryForms& forms, const TNameTokens& nameTokens) {
            for (const auto& nameToken : nameTokens) {
                if (nameToken == ASTERISK) {
                    return true;
                }

                if (!forms.contains(nameToken)) {
                    return false;
                }
            }

            return true;
        }

        static TString WriteMatch(const TNameTokens& names) {
            TString match;
            for (const auto& name : names) {
                if (match) {
                    match.append('+');
                }
                match.append(WideToUTF8(name));
            }
            return match;
        }

        static void ParseItems(TVector<TString>& items, TStringBuf value) {
            while (value) {
                if (TStringBuf aName = value.NextTok(',')) {
                    items.push_back(TString{aName});
                }
            }
        }

        static bool IsFlagRTBF(TStringBuf flag) {
            return flag.NextTok('=') == FLAG_RUOBLIVION_FLAG_RTBF;
        }

        static TString WriteItems(const TVector<TString>& items) {
            return items.size() < 100 ? JoinRange(",", items.begin(), items.end()) : "*";
        }

        struct TParseResult {
            TVector<TString> Names;
            TVector<TString> Flags;
            bool AllRTBF = true;

            void Add(TStringBuf params) {
                bool hasRTBF = false;

                while (params) {
                    TStringBuf tok = params.NextTok(';');
                    if (!tok) {
                        continue;
                    }

                    TStringBuf name, value;
                    if (!tok.TrySplit('=', name, value)) {
                        continue;
                    }

                    if (FLAG_RUOBLIVION_NAMES == name) {
                        ParseItems(Names, value);
                    } else if (FLAG_RUOBLIVION_FLAGS == name) {
                        TVector<TString> flags;
                        ParseItems(flags, value);
                        hasRTBF |= (FindIf(flags.begin(), flags.end(), IsFlagRTBF) != flags.end());
                        Flags.insert(Flags.end(), flags.begin(), flags.end());
                    }
                }

                AllRTBF &= hasRTBF;
            }

            void Finalize() {
                SortUnique(Names);
                SortUnique(Flags);

                if (!AllRTBF) {
                    Flags.resize(std::remove_if(Flags.begin(), Flags.end(), IsFlagRTBF) - Flags.begin());
                }
            }

            TString Save() const {
                TStringStream str;

                if (Names) {
                    str << FLAG_RUOBLIVION_NAMES << '=' << WriteItems(Names) << ';';
                }

                if (Flags) {
                    str << FLAG_RUOBLIVION_FLAGS << '=' << WriteItems(Flags);
                }

                return str.Str();
            }

            bool HasRTBFFlag() const {
                return FindIf(Flags.begin(), Flags.end(), IsFlagRTBF) != Flags.end();
            }
        };

    }

    using namespace NRuOblivion;

    TString MergeRuOblivionParameters(const TVector<TStringBuf>& params) {
        TParseResult res;

        for (auto param : params) {
            res.Add(param);
        }

        res.Finalize();

        return res.Save();
    }

    TRuOblivionResult FindRuOblivionMatch(TStringBuf params, TRichTreeConstPtr richTree) {
        if (!richTree || !richTree->Root) {
            return TRuOblivionResult();
        }

        TParseResult parseResult;
        parseResult.Add(params);

        TQueryForms queryForms;
        queryForms.CollectForms(richTree->Root.Get());

        for (auto name : parseResult.Names) {
            TNameTokens nameTokens;
            PrepareNameTokens(nameTokens, name);
            if (NameMatchesQuery(queryForms, nameTokens)) {
                return TRuOblivionResult(WriteMatch(nameTokens), parseResult.HasRTBFFlag());
            }
        }

        return TRuOblivionResult();
    }

}
