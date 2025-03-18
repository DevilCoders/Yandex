#pragma once

#include "query_marker.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/deprecated/mapped_file/mapped_file.h>

#include <util/charset/wide.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/system/filemap.h>

#include <utility>

template <class TStringType>
class TCompactDict: public TCompactTrie<typename TStringType::char_type, ui32> {
};

template <class TStringType>
class TTrieBuilder: public TCompactTrieBuilder<typename TStringType::char_type, ui32> {
public:
    explicit TTrieBuilder(TCompactTrieBuilderFlags flags)
        : TCompactTrieBuilder<typename TStringType::char_type, ui32>(flags)
    {
    }
};

template <class TStringType>
class TQueryMarkerTrie: public IQueryMarker {
protected:
    typedef TCompactDict<TStringType> TQDict;
    typedef TTrieBuilder<TStringType> TQTrieBuilder;

    TBufferOutput compact;
    THolder<TQDict> Dict;
    THolder<TQTrieBuilder> TrieBuilder;
    TMappedFile MappedFile;
    int MarkerTypesCount;

public:
    TQueryMarkerTrie() {
        TrieBuilder.Reset(new TQTrieBuilder(CTBF_NONE));
        MarkerTypesCount = 1;
    }

    bool LoadMarker(const TString& s, int wordType) override {
        TStringType str = StringNormalize<TStringType>(s);
        if (str.size() > 1) {
            Y_ASSERT(wordType > 0 && wordType <= 32);
            ui32 newMask = 1 << (wordType - 1);
            ui32 oldMask = 0;
            if (TrieBuilder->Find(str.data(), str.size(), &oldMask))
                newMask |= oldMask;
            TrieBuilder->Add(str.data(), str.size(), newMask);
            MarkerTypesCount = std::max(MarkerTypesCount, wordType + 1);
            return true;
        }
        return false;
    }

    ui32 LoadMarkers(const TString& path, int wordType) override {
        Y_ASSERT(wordType > 0 && wordType <= 32);
        ui32 markersLoaded = 0;
        TFileInput file(path);
        TString line;
        while (file.ReadLine(line)) {
            markersLoaded += LoadMarker(line, wordType);
        }
        return markersLoaded;
    }

    void LoadMarkers(const TString& path, const TMap<TString, int>& topicTypes, TWordTypeDistr& markersCount) override {
        TFileInput file(path);
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
            if (currentType != 0) {
                markersCount[currentType] += LoadMarker(line, currentType);
                MarkerTypesCount = std::max(MarkerTypesCount, currentType + 1);
            }
        }
    }

    void FinishInit() override {
        TBufferOutput raw;
        TrieBuilder->Save(raw);
        CompactTrieMinimize<typename TQTrieBuilder::TPacker>(compact, raw.Buffer().Data(), raw.Buffer().Size(), false);
        Dict.Reset(new TQDict());
        Dict->Init(compact.Buffer().Data(), compact.Buffer().Size());
    }

    ui32 QueryClassify(const TString& query, TWordTypeDistr& typeCount, TVector<TMarkerEntry>* foundMarkers) const {
        TNormalizedStroka<TStringType> normalized(query);
        TStringType& str = normalized.GetNormalized();
        TVector<int> queryMask(normalized.Size());
        ui32 querySize = queryMask.size();
        size_t start = 0;
        size_t end = str.size();
        // Cerr << "Normalized string: " << str << Endl;
        while (start < end - 1) {
            size_t prefixLen;
            ui32 mask;
            ui32 b = normalized.GetToken(start);
            if (Dict->FindLongestPrefix(str.data() + start, end - start, &prefixLen, &mask)) {
                ui32 e = normalized.GetToken(start + prefixLen - 1);
                bool first = true;
                int firstType = 0;
                for (int value = 1; value < MarkerTypesCount; ++value) {
                    if (mask & (1 << (value - 1))) {
                        if (firstType == 0)
                            firstType = value;
                        for (size_t i = b; i < e; ++i) {
                            if (first)
                                queryMask[i] = value;
                            ++typeCount[value];
                        }
                        first = false;
                    }
                }
                if (foundMarkers) {
                    foundMarkers->push_back(
                        TMarkerEntry(b, e - 1, mask, WideToUTF8(str.substr(start, prefixLen - 1))));
                }
                //Cerr << "Start: " << start << " PrefixLen: " << prefixLen << Endl;
                //Cerr << "Start token: " << b << "End token: " << e << Endl;
                // Cerr << str.substr(start, prefixLen) << Endl;
                start = normalized.GetTokenNPosition(e);
            } else {
                start = normalized.GetTokenNPosition(b + 1);
            }
        }
        for (auto it : queryMask) {
            if (it == 0) {
                ++typeCount[0];
            }
        }
        return querySize;
    }

    ui32 QueryClassify(const TString& query, TWordTypeDistr& typeCount) const override {
        return QueryClassify(query, typeCount, nullptr);
    }

    TString DeleteMarkers(const TString& query, int wordType, TVector<TMarkerEntry>* foundMarkers = nullptr) const {
        TVector<TMarkerEntry> entries;
        TWordTypeDistr typeCount;
        QueryClassify(query, typeCount, &entries);
        //if wordType == 0, delete all markers
        TNormalizedStroka<TUtf16String> normalized(query);
        if (normalized.Size() == 0) {
            return "";
        }
        TUtf16String result;
        TVector<TMarkerEntry>::const_iterator markerEntry = entries.begin();
        for (size_t token = 0; token < normalized.Size(); token++) {
            if (markerEntry != entries.end() && token > markerEntry->EndToken) {
                ++markerEntry;
            }
            if (markerEntry == entries.end() || token < markerEntry->StartToken ||
                (wordType != 0 && !((1 << (wordType - 1)) & markerEntry->Type)))
            {
                result += normalized.GetTokenText(token);
            }
        }

        //Cerr << "Size: " << +result << " " << result << Endl;
        if (foundMarkers != nullptr)
            *foundMarkers = entries;

        return WideToUTF8(result.substr(1));
    }

    void Load(const TString& fileName) override {
        MarkerTypesCount = 33;
        compact.Buffer().Clear();
        MappedFile.init(fileName.data());
        Dict.Reset(new TQDict());
        Dict->Init(TBlob::NoCopy(MappedFile.getData(), MappedFile.getSize()));
    }

    void Save(const TString& fileName) override {
        TOFStream out(fileName);
        out.Write(compact.Buffer().Data(), compact.Buffer().Size());
    }
};
