#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/snippets/urlmenu/common/common.h>
#include <util/generic/vector.h>

namespace NUrlMenu {

    using TUrlMenuTrie = TCompactTrie<char, TString>;
    using TUrlMenuTrieBuilder = TCompactTrieBuilder<char, TString>;

    class TSearcher {
    private:
         TUrlMenuTrie Index;

    public:
        TSearcher(const TString& indexLocation)
            : Index(TBlob::FromFile(indexLocation)) {}

        TSearcher(const TBlob& blob)
            : Index(blob) {}

        static bool Search(const TVector<TUrlMenuTrie>& indexes, const TString& url, TUrlMenuVector& result);
        static bool Search(const TUrlMenuTrie& index, const TString& url, TUrlMenuVector& result);

        bool Search(const TString& url, TUrlMenuVector& result) const {
            return Search(Index, url, result);
        }

        void Print(IOutputStream* out);
    };

    //Опции при создания поискового трая
    struct TSearcherIndexCreationOptions {
    public:
        bool Minimize; //минимизирует итоговый трай
        bool Verbose;
        bool Sorted; //все строки на входе должны быть отсортированы, значительно сокращает используемую память
        bool Normalize; //нормализовать урлы на входе

        TSearcherIndexCreationOptions()
            : Minimize(true)
            , Verbose(true)
            , Sorted(false)
            , Normalize(true)
        {
        }
    };

    //Создаёт трай для поиска по данным из файла sourceFile;
    //данные из sourceFile считываются построчно и рассматриваются как разделенная табом пара ключ(урл), значение(название)
    void CreateSearcherIndex(const TString& sourceFile, const TString& trieFile, const TSearcherIndexCreationOptions& options);

    //Старый интерфейс, для сохранения совместимости
    void CreateSearcherIndex(const TString& sourceFile, const TString& trieFile, bool minimized, bool verbose);
}
