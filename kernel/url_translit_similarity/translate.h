#pragma once
#include "hardcomp.h"
#include <util/string/split.h>

namespace NUrlTranslitSimilarity {
    class TVocabTransit {
    public:
        TPtRecMap PtRecMap;
    public:
        const TVector<TPtRec> *Get(const TString &key) const {
            TPtRecMap::const_iterator it = PtRecMap.find(key);
            if (it == PtRecMap.end()) {
                return nullptr;
            }
            const TVector<TPtRec>& value = it->second;
            return &(value);
        }

        int Load(IInputStream* inputStream) {
            int line = 0;
            TString entry;

            while (inputStream->ReadLine(entry)) {
                TVector<TString> parts;
                StringSplitter(entry).Split('\t').AddTo(&parts);
                Y_ENSURE(parts.size() == 3);
                if (parts[0] == parts[1]) {
                    continue;
                }
                double weight = FromString<double>(parts[2]);
                PtRecMap[parts[0]].push_back(TPtRec(parts[1], weight));
                PtRecMap[parts[1]].push_back(TPtRec(parts[0], weight));
                line++;
            }
            for (auto& tRec : PtRecMap) {
                TVector<TPtRec>& value = tRec.second;
                Sort(value.begin(), value.end());
            }
            return line;
        }

        bool SetIntersection(const TVector<TString>& set1,
                                const TVector<TPtRec>& set2,
                                TPtRec& res) const
        {
            if (set1.empty()) {
                return false;
            }
            if (set2.empty()) {
                return false;
            }
            TVector<TString>::const_iterator el1 = set1.begin();
            TVector<TPtRec>::const_iterator el2 = set2.begin();

            bool first = true;
            while (el1 != set1.end() && el2 != set2.end()) {
                if (*el1 < el2->Dst) {
                    ++el1;
                }
                else {
                    if (el2->Dst == *el1) {
                        if (first || (res.Weight > el2->Weight)) {
                            res = *el2;
                            first = false;
                        }
                        el1++;
                    }
                    el2++;
                }
            }
            return !first;
        }

        bool GetBestTranslation(const TString& str, const TVector<TString>& query, TPtRec& res) const {
            const TVector<TPtRec> *sr = Get(str);
            if (sr == nullptr) {
                return false;
            }
            return SetIntersection(query, *sr, res);
        }
    };
}
