#include "article.h"
#include "translatedict.h"

#include <util/generic/hash.h>
#include <util/stream/input.h>

//#define TRANSLATION_DICT_SAVE_LOAD_DEBUG

static bool Check(const TGramBitSet& a) {
    TGramClass mask = 0;
    for (EGrammar gr : a) {
        TGramClass cl = TGrammarIndex::GetClass(gr);
        if (mask & cl)
            return false;
        mask |= cl;
    }
    return true;
}

static int CompatLevel(const TGramBitSet& a, const TGramBitSet& b) {
    if (a.none() || b.none())
        return 0;
    int res = int((a & b).count());
    if (!Check(a & ~b | b & ~a))
        return -1;
    return res;
}

using NTranslationDictInt::TArticleInt;

class TTranslationDict::TImpl {
private:
    template <class TChr>
    class TTStrPool {
    private:
        typedef THashMap<const TChr*, size_t, NTranslationDictInt::TStrHash<TChr>, NTranslationDictInt::TStrCmp<TChr>> THash;
        typedef TVector<const TChr*> TVectorType;

    public:
        TTStrPool()
            : Num(0)
        {
        }
        size_t operator()(const TChr* s) const {
            typename THash::const_iterator i = Hash.find(s);
            if (i != Hash.end())
                return i->second;
            return size_t(-1);
        }
        const TChr* operator()(size_t i) const {
            if (i < Num)
                return Vector[i];
            return nullptr;
        }
        size_t Size() const {
            return Num;
        }
        void Load(IInputStream& inputStream) {
            using NTranslationDictInt::Load;
            Load(inputStream, Num);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
            Cerr << "TTStrPool::Load: " << Num << " strings to load" << Endl;
#endif
            Vector.Reset(new const TChr*[Num]);
            for (size_t i = 0; i < Num; ++i) {
                size_t val = 0;
                Load(inputStream, val);
                Vector[i] = nullptr;
                Vector[i] += val;
            }

            size_t poolSize = 0;
            Load(inputStream, poolSize);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
            Cerr << "TTStrPool::Load: poolSize is " << poolSize << " characters" << Endl;
#endif
            Pool.Reset(new TChr[poolSize]);
            inputStream.LoadOrFail(Pool.Get(), poolSize * sizeof(TChr));

            for (size_t i = 0; i < Num; ++i) {
                Vector[i] = Pool.Get() + (Vector[i] - (const TChr*)nullptr);
                Hash[Vector[i]] = i;
            }
        }

    private:
        THolder<TChr, TDeleteArray> Pool;
        THolder<const TChr*, TDeleteArray> Vector;
        size_t Num;
        THash Hash;
    };

    class TGrammarPool {
    public:
        TGrammarPool()
            : Num(0)
        {
        }
        const TGramBitSet& operator()(size_t i) const {
            return Vector[i];
        }
        size_t Size() const {
            return Num;
        }
        void Load(IInputStream& inputStream) {
            TTStrPool<char> pool;
            pool.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
            Cerr << pool.Size() << " gramma strings loaded" << Endl;
#endif
            Vector.Reset(new TGramBitSet[pool.Size()]);
            Num = pool.Size();
            for (size_t i = 0; i < Num; ++i)
                Vector[i] = TGramBitSet::FromBytes(pool(i));
        }

    private:
        THolder<TGramBitSet, TDeleteArray> Vector;
        size_t Num;
    };

    typedef TTStrPool<wchar16> TStrPool;

    class TDictionary {
    private:
        struct TDistrib {
            size_t Offset;
            size_t Length;
        };

    public:
        TDictionary()
            : DistSize(0)
        {
        }
        const TArticleInt& operator()(size_t word, size_t i) const {
            Y_ASSERT(word < DistSize);
            Y_ASSERT(i < Distr[word].Length);
            return Dict[Distr[word].Offset + i];
        }
        size_t Size(size_t word) const {
            Y_ASSERT(word < DistSize);
            return Distr[word].Length;
        }
        size_t Size() const {
            return DistSize;
        }

        void Load(IInputStream& inputStream) {
            using NTranslationDictInt::Load;
            Load(inputStream, DistSize);
            Distr.Reset(new TDistrib[DistSize]);
            for (size_t i = 0; i < DistSize; ++i) {
                Load(inputStream, Distr[i].Offset);
                Load(inputStream, Distr[i].Length);
            }
            size_t dictSize = 0;
            Load(inputStream, dictSize);
            Dict.Reset(new TArticleInt[dictSize]);
            for (size_t j = 0; j < dictSize; ++j)
                Dict[j].Load(inputStream);
        }

    private:
        THolder<TArticleInt, TDeleteArray> Dict;
        THolder<TDistrib, TDeleteArray> Distr;
        size_t DistSize;
    };

    struct TMetaDict {
        const TStrPool& From;
        const TStrPool& To;
        const TDictionary& Dict;
        const TGrammarPool& GrammarPool;

        TMetaDict(const TStrPool& poolFrom, const TStrPool& poolTo, const TDictionary& dict, const TGrammarPool& grammarPool)
            : From(poolFrom)
            , To(poolTo)
            , Dict(dict)
            , GrammarPool(grammarPool)
        {
        }

        size_t Translate(const TTranslationDict::TArticle& art, TVector<TTranslationDict::TArticle>& result) const {
            result.clear();
            if (!art.Word)
                return 0;
            size_t wFrom = From(art.Word);
            if (wFrom > Dict.Size())
                return 0;
            int maxLev = -1;
            for (size_t i = 0; i < Dict.Size(wFrom); ++i) {
                const TArticleInt& t = Dict(wFrom, i);
                int lv = CompatLevel(art.Grammar, GrammarPool(t.InitialGr));
                if (lv > maxLev)
                    maxLev = lv;
            }
            if (maxLev < 0)
                return 0;
            for (size_t i = 0; i < Dict.Size(wFrom); ++i) {
                const TArticleInt& t = Dict(wFrom, i);
                int lv = CompatLevel(art.Grammar, GrammarPool(t.InitialGr));
                if (lv == maxLev)
                    result.push_back(TTranslationDict::TArticle(To(t.Word), GrammarPool(t.Gramm)));
            }
            return result.size();
        }
    };

public:
    TImpl()
        : E2R_(EngPool, RusPool, E2RDict, GrammarPool)
        , R2E_(RusPool, EngPool, R2EDict, GrammarPool)
    {
    }

    size_t E2R(const TArticle& art, TVector<TArticle>& result) const {
        return E2R_.Translate(art, result);
    }
    size_t R2E(const TArticle& art, TVector<TArticle>& result) const {
        return R2E_.Translate(art, result);
    }

    void Load(IInputStream& inputStream) {
        RusPool.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
        Cerr << "RusPool loaded" << Endl;
#endif
        EngPool.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
        Cerr << "EngPool loaded" << Endl;
#endif
        GrammarPool.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
        Cerr << "GrammarPool loaded" << Endl;
#endif
        E2RDict.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
        Cerr << "E2R loaded" << Endl;
#endif
        R2EDict.Load(inputStream);
#ifdef TRANSLATION_DICT_SAVE_LOAD_DEBUG
        Cerr << "R2E loaded" << Endl;
#endif
    }

private:
    TStrPool RusPool;
    TStrPool EngPool;
    TGrammarPool GrammarPool;
    TDictionary E2RDict;
    TDictionary R2EDict;
    TMetaDict E2R_;
    TMetaDict R2E_;
};

TTranslationDict::TTranslationDict()
    : Impl(new TTranslationDict::TImpl)
{
}

TTranslationDict::~TTranslationDict() {
}

size_t TTranslationDict::FromEnglish(const TArticle& art, TVector<TArticle>& result) const {
    return Impl->E2R(art, result);
}

size_t TTranslationDict::ToEnglish(const TArticle& art, TVector<TArticle>& result) const {
    return Impl->R2E(art, result);
}

void TTranslationDict::Load(IInputStream& inputStream) {
    Impl->Load(inputStream);
}
