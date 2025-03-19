#include "trie_url_mask_iterator.h"

#include <kernel/hosts/owner/owner.h>

#include <util/generic/yexception.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSaasTrie {
    inline TStringBuf GetOwnerFromHost(TStringBuf host) {
        return TOwnerCanonizer::WithSerpCategOwners().GetHostOwner(host);
    }

    bool TUrlMaskHostIterator::SetUrl(TStringBuf url) {
        Url = url;
        FullHost = GetHost(url);
        const size_t hostSize = FullHost.size();
        if (Y_UNLIKELY(hostSize == 0)) {
            Index = MaxHosts;
            return false;
        }
        ui32 i = MaxHosts - 1;
        size_t dotPos = FullHost.rfind('.');
        if (dotPos != TStringBuf::npos) {
            dotPos = FullHost.rfind('.', dotPos);
            for (; dotPos != TStringBuf::npos && i > 0; --i) {
                Heads[i] = dotPos + 1;
                dotPos = FullHost.rfind('.', dotPos);
            }
        }
        Heads[i] = 0;
        Index = i;
        OwnerLimit = 0;
        return true;
    }

    bool TUrlMaskHostIterator::CheckOwner() {
        if (OwnerLimit == 0) {
            const ui32 hostSize = FullHost.size();
            const ui32 ownerSize = GetOwnerFromHost(FullHost).size();
            OwnerLimit = Y_LIKELY(hostSize >= ownerSize) ? hostSize - ownerSize : 0;
        }
        if (Heads[Index] <= OwnerLimit) {
            return true;
        }
        Index = MaxHosts;
        return false;
    }

    const TStringBuf PathDelim{"/"};
    const TStringBuf RootPath = PathDelim;
    const TStringBuf EmptyPath{""};

    void TUrlMaskPathIterator::SetUrl(TStringBuf url) {
        TStringBuf pathAndQuery = GetPathAndQuery(url, true);
        if (Y_UNLIKELY(!pathAndQuery)) {
            pathAndQuery = RootPath;
        }
        const TStringBuf path = pathAndQuery.Before('?');
        const size_t pathSize = path.size();
        SlashIndex = MaxPaths;
        Index = 0;
        End = 1;
        Paths[0] = pathAndQuery;
        if (pathSize < pathAndQuery.size()) {
            Paths[End++] = path;
        } else if (path.back() != '/') {
            SlashIndex = End;
            Paths[End++] = path;
        }
        size_t slashPos = 0;
        for (; slashPos < pathSize - 1 && End < MaxPaths; ++End) {
            Paths[End] = path.SubStr(0, slashPos + 1);
            slashPos = path.find('/', slashPos + 1);
        }
    }

    TStringBuf Slice(TStringBuf s, size_t from, size_t to) {
        return TStringBuf{s.data() + from, to - from};
    }

    struct TUrlMaskPathPathSplitter {
        // magic number from https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/infected_masks/infected_masks.cpp?rev=3176868#L258
        static const ui32 MaxPaths = 6;

        void SetUrl(TStringBuf url) {
            FullPath = GetPathAndQuery(url, true);
            if (Y_UNLIKELY(!FullPath)) {
                FullPath = RootPath;
            }
            size_t qpos = FullPath.find('?');
            const TStringBuf path = qpos == TStringBuf::npos ? FullPath : Slice(FullPath, 0, qpos);
            const size_t pathSize = path.size();
            SlashIndex = MaxPaths;
            Offset = (qpos != TStringBuf::npos || path.back() != '/') ? 2 : 1;
            End = Offset;
            size_t slashPos = 0;
            for (; slashPos < pathSize - 1 && End < MaxPaths; ++End) {
                SubPaths[End] = slashPos + 1;
                slashPos = path.find('/', slashPos + 1);
            }
            if (Offset == 1) {
                SubPaths[0] = pathSize;
            } else {
                if (qpos == TStringBuf::npos) {
                    SubPaths[0] = SubPaths[1] = pathSize;
                    SlashIndex = 1;
                } else {
                    SubPaths[0] = FullPath.size();
                    SubPaths[1] = pathSize;
                }
            }
        }

        bool Reset(ITrieStorageReader* trie) {
            ITrieStorageReader* parent = trie;
            ui32 from = 0;
            for (ui32 i = Offset; i < End; ++i) {
                SubTries[i] = parent->GetSubTree(Slice(FullPath, from, SubPaths[i]));
                if (SubTries[i]->IsEmpty()) {
                    RealEnd = i;
                    Index = Offset;
                    return FoundSubPath();
                }
                parent = SubTries[i].Get();
                from = SubPaths[i];
            }
            Index = 0;
            RealEnd = End;
            if (Offset == 1) {
                SubTries[0] = parent->GetSubTree(Slice(FullPath, from, SubPaths[0]));
            } else {
                if (SubPaths[0] > SubPaths[1]) {
                    SubTries[1] = parent->GetSubTree(Slice(FullPath, from, SubPaths[1]));
                    if (SubTries[1]->IsEmpty()) {
                        SubTries[0].Reset();
                    } else {
                        from = SubPaths[1];
                        SubTries[0] = SubTries[1]->GetSubTree(Slice(FullPath, from, SubPaths[0]));
                    }
                } else {
                    SubTries[0] = parent->GetSubTree(Slice(FullPath, from, SubPaths[0]));
                    if (SubTries[0]->IsEmpty()) {
                        SubTries[1].Reset();
                    } else {
                        SubTries[1] = SubTries[0]->GetSubTree(RootPath);
                    }
                }
            }
            return FoundSubPath();
        }
        TString CurrentPath() const {
            TStringBuf key{Slice(FullPath, 0, SubPaths[Index])};
            if (Index == SlashIndex) {
                return TString::Join(key, '/');
            }
            return TString{key};
        }
        std::pair<TStringBuf, TStringBuf> CurrentPathPieces() const {
            auto path = Slice(FullPath, 0, SubPaths[Index]);
            if (Index == SlashIndex) {
                return {path, PathDelim};
            }
            return {path, EmptyPath};
        }
        ui64 GetValue() const {
            return Value;
        }
        bool Next() {
            ++Index;
            return FoundSubPath();
        }

    private:
        bool FoundSubPath() {
            for (; Index < RealEnd; ++Index) {
                if (SubTries[Index] && SubTries[Index]->Get({}, Value)) {
                    return true;
                }
            }
            return false;
        }

        std::array<THolder<ITrieStorageReader>, MaxPaths> SubTries;
        TStringBuf FullPath;
        ui64 Value = 0;
        std::array<ui32, MaxPaths> SubPaths;
        ui32 SlashIndex = MaxPaths;
        ui32 Offset = 0;
        ui32 End = 0;
        ui32 Index = 0;
        ui32 RealEnd = 0;
    };

    struct TUrlMaskSplitter {
        void SetTrie(const ITrieStorageReader* trie) {
            Trie = trie;
        }
        bool SetUrl(TStringBuf url) {
            Y_ENSURE(!url.empty());
            if (url[0] == '\t') {
                url = Slice(url, 1, url.size());
            }
            if (!HostIterator.SetUrl(url)) {
                return false;
            }
            PathIterator.SetUrl(url);
            return FindNotEmptyHost();
        }
        TString GetKey() const {
            return HostIterator.CurrentHost() + PathIterator.CurrentPath();
        }
        std::pair<TStringBuf, std::pair<TStringBuf, TStringBuf>> GetKeyPieces() const {
            return {HostIterator.CurrentHost(), PathIterator.CurrentPathPieces()};
        }
        ui64 GetValue() const {
            return PathIterator.GetValue();
        }
        bool Next() {
            if (PathIterator.Next()) {
                return true;
            }
            return HostIterator.Next() && FindNotEmptyHost();
        }
    private:
        bool FindNotEmptyHost() {
            do {
                HostTrie = Trie->GetSubTree(HostIterator.CurrentHost());
                if (!HostTrie->IsEmpty() && HostIterator.CheckOwner() && PathIterator.Reset(HostTrie.Get())) {
                    return true;
                }
            } while (HostIterator.Next());
            return false;
        }

        const ITrieStorageReader* Trie = nullptr;
        TUrlMaskHostIterator HostIterator;
        TUrlMaskPathPathSplitter PathIterator;
        THolder<ITrieStorageReader> HostTrie;
    };

    struct TTrieUrlMaskIterator : ITrieStorageIterator {
        TTrieUrlMaskIterator(const ITrieStorageReader& trie, const TComplexKeyPreprocessed& key)
            : ComplexKey(key)
        {
            auto nLevels = ComplexKey.SortedRealms.size();
            Y_ENSURE(nLevels > 0);
            MainSubTrie = trie.GetSubTree(ComplexKey.UrlMaskPrefix + '\t');
            if (!MainSubTrie->IsEmpty()) {
                End = ComplexKey.SortedRealms[0].size();
                Splitter.SetTrie(MainSubTrie.Get());
                FindNotEmptyUrl();
            }
        }

        bool AtEnd() const override {
            return Current >= End;
        }
        TString GetKey() const override {
            auto keyPieces = Splitter.GetKeyPieces();
            return TString::Join(ComplexKey.UrlMaskPrefix, '\t', keyPieces.first, keyPieces.second.first, keyPieces.second.second);
        }
        ui64 GetValue() const override {
            return Splitter.GetValue();
        }
        bool Next() override {
            if (Splitter.Next()) {
                return true;
            }
            ++Current;
            return FindNotEmptyUrl();
        }

    private:
        bool FindNotEmptyUrl() {
            for (; Current < End; ++Current) {
                if (Splitter.SetUrl(ComplexKey.SortedRealms[0][Current])) {
                    return true;
                }
            }
            return false;
        }

        const TComplexKeyPreprocessed& ComplexKey;
        THolder<ITrieStorageReader> MainSubTrie;
        TUrlMaskSplitter Splitter;
        size_t Current = 0;
        size_t End = 0;
    };

    THolder<ITrieStorageIterator> CreateTrieUrlMaskIterator(const ITrieStorageReader& trie,
                                                            const TComplexKeyPreprocessed& key) {
        return MakeHolder<TTrieUrlMaskIterator>(trie, key);
    }
}
