#include "index_proc.h"

#include "iterator_yndex_impl.h"
#include "iterator_offroad_impl.h"

#include <kernel/keyinv/invkeypos/keyconv.h>

#include <ysite/yandex/posfilter/leaf.h>

#include <util/string/builder.h>

using namespace NReqBundleIterator;

namespace {
    bool MakeAttrKey(TMemoryPool& pool, TWtringBuf lemma, bool utf8IndexKeys, TStringBuf& result) {
        // TODO(sankear): remove temp buffers, don't copy strings

        static const TUtf16String Separator = u"=";
        size_t sep = lemma.find(Separator);
        if (sep == TWtringBuf::npos) {
            return false;
        }

        const size_t valueStartPos = sep + Separator.size();
        const size_t valueSize = lemma.size() - valueStartPos;
        if (valueSize > MAXKEY_LEN) {
            return false;
        }

        TString attrName = PrepareAttrName(lemma.substr(0, sep));
        TString attrValue;

        wchar16 attrOrigValue[MAXKEY_BUF];
        memcpy(attrOrigValue, lemma.data() + valueStartPos, valueSize * sizeof(wchar16));
        attrOrigValue[valueSize] = 0;

        if (!PrepareAttrValue(TWtringBuf(attrOrigValue, valueSize), utf8IndexKeys, attrValue)) {
            return false;
        }

        TString attrKey;
        SetAttrKey(attrKey, attrName, "=", attrValue);

        result = pool.AppendString<char>(attrKey);

        return true;
    }

    TStringBuf MakeLemmaKey(TMemoryPool& pool, bool utf8IndexKeys, TWtringBuf lemma) {
        size_t bufLen = 3 * lemma.size() + 2;
        char* buf = pool.AllocateArray<char>(bufLen);
        size_t keySize;
        if (utf8IndexKeys) {
            size_t nread = 0;
            RecodeFromUnicode(CODES_UTF8, lemma.data(), buf, lemma.size(), bufLen - 1, nread, keySize);
            buf[keySize] = 0;
            return TStringBuf(buf, keySize);
        } else {
            TFormToKeyConvertor(buf, bufLen).Convert(lemma.data(), lemma.size(), keySize);
            return TStringBuf(buf, keySize);
        }
    }
} // namespace

namespace NReqBundleIteratorImpl {
    void TLemmIndexData::OnFormAccepted(
        TWtringBuf yndexForm,
        ui8 yndexFormFlags,
        ELanguage yndexFormLang,
        TWtringBuf richTreeForm)
    {
        TFormInfo formInfo;
        formInfo.Form = FormsPool->AppendString(yndexForm);
        formInfo.Extra = ((ui32)yndexFormLang << 16)
            | (yndexFormFlags << 8);

        TIdsPair ids;
        ids.LowLevelId = Max<ui16>(); // will be assigned later
        auto it = RichTreeFormsCatalog->Form2Id.find(richTreeForm);
        if (it == RichTreeFormsCatalog->Form2Id.end()) {
            Y_ASSERT(richTreeForm.empty());
            ids.RichTreeId = 0;
        } else {
            ids.RichTreeId = it->second;
        }
        auto iterator = YndexFormsCatalog->insert(std::make_pair(formInfo, ids)).first;
        TYndexCatalogRef ref = &*iterator;
        Forms.push_back(ref);
    }

    void TLemmIndexData::MakeIterators(
        TIteratorPtrs& target,
        size_t blockId,
        TMemoryPool& iteratorsMemory)
    {
        Hits->MakeIterators(target, blockId, iteratorsMemory, *this);
    }

    void TWordIndexData::CollectLemmas(
        const TWordBundleData& commonData,
        TMemoryPool& pool,
        bool utf8IndexKeys,
        const TVector<ui64>& keyPrefixes,
        TIndexLookupMapping& allKeys)
    {
        Lemmas.reserve(commonData.Lemm2Data.size() * keyPrefixes.size());

        for (const auto& it : commonData.Lemm2Data) {
            TStringBuf key;
            if (it.second->IsAttribute) {
                if (!MakeAttrKey(pool, it.first, utf8IndexKeys, key)) {
                    continue;
                }
            } else {
                key = MakeLemmaKey(pool, utf8IndexKeys, it.first);
            }
            for (ui64 kps : keyPrefixes) {
                Lemmas.emplace_back(
                    pool,
                    &commonData,
                    &YndexFormsCatalog,
                    it.second.Get());

                ++NumLemmas;
                TIndexLookupKey k{ key, kps };
                allKeys[k].push_back(Lemmas.data() + NumLemmas - 1);
            }
        }
    }

    void TWordIndexData::MakeIterators(
        TIteratorPtrs& target,
        TVector<ui16>* richTreeFormIds,
        size_t blockId,
        TMemoryPool& iteratorsMemory)
    {
        Y_ENSURE(YndexFormsCatalog.size() <= TLowLevelFormId::MaxValue,
            "too many index forms when matching qbundle node");

        ui16 id = 0;
        bool hasAttributeLemma = AnyOf(Lemmas, [](const TLemmIndexData& lemma) { return lemma.BundleData->IsAttribute; });
        if (hasAttributeLemma) {
            ++id; // reserve LowLevelId == 0 for attributes
        }
        for (auto& it : YndexFormsCatalog) {
            it.second.LowLevelId = id++;
        }
        if (richTreeFormIds) {
            if (hasAttributeLemma) {
                richTreeFormIds->push_back(0);
            }
            for (auto& it : YndexFormsCatalog) {
                richTreeFormIds->push_back(it.second.RichTreeId);
            }
        }
        for (size_t i = 0; i < NumLemmas; i++) {
            Lemmas[i].MakeIterators(target, blockId, iteratorsMemory);
        }
    }

    void TBlockIndexData::CollectIndexKeys(
        const TBlockBundleData& commonData,
        TMemoryPool& pool,
        bool utf8IndexKeys,
        const TVector<ui64>& keyPrefixes,
        TIndexLookupMapping& allKeys)
    {
        Words.reserve(commonData.NumWords);

        for (size_t i = 0; i < commonData.NumWords; i++) {
            Words.emplace_back(pool);
            ++NumWords;
            Words[i].CollectLemmas(commonData.Words[i], pool, utf8IndexKeys, keyPrefixes, allKeys);
        }
    }
} // NReqBundleIteratorImpl
