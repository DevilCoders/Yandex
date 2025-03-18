#pragma once

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <utility>

#include <library/cpp/on_disk/aho_corasick/reader.h>
#include <library/cpp/on_disk/aho_corasick/writer.h>

#include "query_marker.h"

using TAhoPair = std::pair<ui32, int>;

template <class TStringType>
class TAhoBuilder: public TAhoCorasickBuilder<TStringType, TAhoPair> {
};

template <class TStringType>
ui32 DirectLoadMarkers(TAhoBuilder<TStringType>& aho, const TString& path, int wordType) {
    ui32 markersLoaded = 0;
    TIFStream file(path);
    TString line;
    while (file.ReadLine(line)) {
        TStringType s = StringNormalize<TStringType>(line);
        if (s.size() > 1) {
            aho.AddString(s, TAhoPair(s.size(), wordType));
            ++markersLoaded;
        }
    }
    return markersLoaded;
}

template <class TStringType>
void DirectLoadMarkers(TAhoBuilder<TStringType>& aho, const TString& path, const TMap<TString, int>& topicTypes, TWordTypeDistr& marketsCount) {
    TIFStream file(path);
    TString line;
    int currentType = 0;
    while (file.ReadLine(line)) {
        if (line[0] == '[') {
            TMap<TString, int>::const_iterator it = topicTypes.find(line.substr(1, line.size() - 2));
            if (it != topicTypes.end())
                currentType = (*it).second;
            else
                currentType = 0;
            continue;
        }
        TStringType s = StringNormalize<TStringType>(line);
        if (currentType != 0 && s.size() > 1) {
            aho.AddString(s, TAhoPair(s.size(), currentType));
            ++marketsCount[currentType];
        }
    }
}

struct TYPairCmp {
    bool operator()(std::pair<ui32, TAhoPair> x, std::pair<ui32, TAhoPair> y) {
        return x.first == y.first ? (x.second.first == y.second.first ? x.second.second < y.second.second : x.second.first > y.second.first) : x.first < y.first;
    }
};

template <class TStringType>
ui32 DirectQueryClassify(const TMappedAhoCorasick<TStringType, TAhoPair>& aho, const TString& query, TWordTypeDistr& typeCount) {
    TNormalizedStroka<TStringType> normalized(query);
    typedef std::pair<ui32, TAhoPair> TTriple;
    typedef TDeque<TTriple> TListType;

    TListType result = aho.AhoSearch(normalized.GetNormalized());
    TVector<TTriple> sorted;
    for (TListType::const_iterator it = result.begin(); it != result.end(); ++it)
        sorted.push_back(TTriple(it->first - it->second.first + 1, it->second));

    static TYPairCmp ypairCmp;
    std::sort(sorted.begin(), sorted.end(), ypairCmp);
    ui32 current = 0;
    TVector<int> queryMask(normalized.Size());
    ui32 querySize = queryMask.size();
    for (TVector<TTriple>::const_iterator it = sorted.begin(); it != sorted.end();) {
        if ((*it).first >= current) {
            ui32 begin = (*it).first;
            ui32 end = (*it).second.first;
            bool first = true;
            do {
                ui32 b = normalized.GetToken((*it).first);
                ui32 e = normalized.GetToken((*it).first + (*it).second.first - 1);
                for (size_t i = b; i < e; ++i) {
                    if (first)
                        queryMask[i] = (*it).second.second;
                    ++typeCount[(*it).second.second];
                }
                first = false;
                ++it;
            } while (it != sorted.end() && begin == (*it).first && end == (*it).second.first);
            current = begin + end - 1;
        } else
            ++it;
    }
    for (auto it : queryMask) {
        if (it == 0) {
            ++typeCount[0];
        }
    }
    return querySize;
}

template <class TStringType>
class TQueryMarkerAho: public IQueryMarker {
private:
    THolder<TAhoBuilder<TStringType>> AhoBuilder;
    TBlob AhoCorasickData;
    //TODO: think about lazy init
public:
    TQueryMarkerAho()
        : AhoBuilder(new TAhoBuilder<TStringType>)
    {
    }

    bool LoadMarker(const TString& s, int wordType) override {
        TStringType str = StringNormalize<TStringType>(s);
        if (str.size() > 1) {
            if (!AhoBuilder.Get())
                throw yexception() << "aho builder it not initialized" << Endl;
            AhoBuilder->AddString(str, TAhoPair(str.size(), wordType));
            return true;
        }
        return false;
    }

    ui32 LoadMarkers(const TString& path, int wordType) override {
        if (!AhoBuilder.Get())
            throw yexception() << "aho builder it not initialized" << Endl;
        return DirectLoadMarkers(*AhoBuilder, path, wordType);
    }

    void LoadMarkers(const TString& path, const TMap<TString, int>& topicTypes, TWordTypeDistr& marketsCount) override {
        if (!AhoBuilder.Get())
            throw yexception() << "aho builder it not initialized" << Endl;
        DirectLoadMarkers(*AhoBuilder, path, topicTypes, marketsCount);
    }

    void FinishInit() override {
        if (!AhoBuilder.Get())
            throw yexception() << "aho builder it not initialized" << Endl;
        AhoCorasickData = AhoBuilder->Save();
        AhoBuilder.Destroy();
    }

    ui32 QueryClassify(const TString& query, TWordTypeDistr& typeCount) const override {
        TMappedAhoCorasick<TStringType, TAhoPair> ahoSearcher(AhoCorasickData);
        return DirectQueryClassify(ahoSearcher, query, typeCount);
    }
};
