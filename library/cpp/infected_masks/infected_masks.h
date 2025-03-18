#pragma once

#include "sb_masks.h"

#include <library/cpp/binsaver/bin_saver.h>
#include <util/generic/hash_set.h>
#include <util/string/vector.h>

namespace NInfectedMasks {
    class TLiteMask {
    public:
        TStringBuf Host;
        TStringBuf PathAndQuery;
        // для совместимости со старым яндексовым кодом, для "/something" нам может понадобиться также сгенерить "/something/"
        // гугловая спека этого не требует
        TStringBuf Suffix;

    public:
        TLiteMask() = default;

        TLiteMask(TStringBuf host, TStringBuf pNq);

        TLiteMask(TStringBuf host, TStringBuf pNq, TStringBuf suf);

        void Render(TString& res) const;

        TString ToString() const;

        bool operator==(const TLiteMask&) const;
    };

    // Гугловый формат описан тут: https://developers.google.com/safe-browsing/v3/update-guide#SuffixPrefixLookup

    enum ECompatibility {
        C_GOOGLE = 0,
        // Гугловый формат предписывает генерировать маски в определённом порядке,
        // Этот порядок не совпадает с порядком в яндексовом формате.
        // Отсюда гибридная совместимость: GenerateMasksFast использует гугловый порядок.
        C_YANDEX_GOOGLE_ORDER = 1,
        // Если нужен яндексовый порядок, сгенерированные маски пересортировываются.
        // C_YANDEX = C_YANDEX_YANDEX_ORDER
        C_YANDEX = 3
    };

    void GenerateMasksFast(TVector<TLiteMask>& masks, TStringBuf normUrl, ECompatibility);

    // Максимальное число сгенерённых масок
    // Для формата GOOGLE - 30 = 5 (hosts) * 6 (paths)
    // Для формата YANDEX - 35 = 5 (hosts) * 7 (paths)
}

class TInfectedMasksGenerator {
public:
    explicit TInfectedMasksGenerator(
            const TString& url,
            NInfectedMasks::ECompatibility compatibility = NInfectedMasks::C_YANDEX
    );

    TInfectedMasksGenerator(
            const TString& canonHost,
            const TString& canonPath,
            NInfectedMasks::ECompatibility compatibility = NInfectedMasks::C_YANDEX
    );

    bool IsValid() const {
        return MaskIdx < (int)Masks.size();
    }

    bool HasNext() const {
        return MaskIdx < (int)Masks.size();
    }

    const TString& Current() const {
        return MaskBuffer;
    }

    void Next() {
        MaskIdx += 1;
        SetMask();
    }

private:
    void Init(TStringBuf canonHost, TStringBuf canonPath, NInfectedMasks::ECompatibility compatibility);

    void SetMask();

private:
    TString Url;
    TString MaskBuffer;

    TVector<NInfectedMasks::TLiteMask> Masks;
    i32 MaskIdx = -1;
};

using TMasks = THashMultiMap<TString, TString>;
using TMasksIterator = TMasks::const_iterator;
using TMasksRange = std::pair<TMasksIterator, TMasksIterator>;

//
// The file format is described here: https://jira.yandex-team.ru/browse/SRCHPROJECT-827
//
class TInfectedMasks {
public:
    TInfectedMasks() {
    }

    TInfectedMasks(const TString& filePath) {
        Init(filePath);
    }

    void Init(const TString& filePath);

    bool IsInfectedUrl(const TString& hostname, const TString& urlpath) const;
    bool IsInfectedUrl(const TString& url) const;

    TVector<TString>* GetData(const TString& url) const;

    void AddMask(TString mask, TString data);

    SAVELOAD(Domains, Masks);
    Y_SAVELOAD_DEFINE(Domains, Masks);

protected:
    TMasksRange GetRange(const TString& hostname, const TString& urlpath) const;
    TMasksRange GetRange(const TString& url) const;

private:
    THashSet<TString> Domains;
    TMasks Masks;
};
