#pragma once

#include "qd_inferiority.h"

#include <kernel/querydata/common/qd_key_token.h>
#include <kernel/querydata/common/querydata_traits.h>

#include <util/digest/numeric.h>
#include <util/digest/murmur.h>
#include <util/generic/buffer.h>
#include <util/generic/deque.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/pool.h>

namespace NQueryData {

    class TRequestRec;

    class TBaseTuple {
    public:
        virtual ~TBaseTuple() {
        }

        TStringBuf AsStrBuf() const {
            return GetStringBuf();
        }

        void GetSubkeys(TStringBufs& subkeys) const {
            SplitKeyIntoSubkeys(AsStrBuf(), subkeys);
        }

        void AddSubkey(TStringBuf subkey) {
            Inferiority.AddSubkey(subkey);
            DoAddSubkey(subkey);
        }

        void AddPrioritizedSubkey(TStringBuf subkey, ui8 inferiority) {
            WithPriorities = true;
            Inferiority.AddSubinferiority(inferiority);
            DoAddSubkey(subkey);
        }

        bool operator<(const TBaseTuple& t) const {
            return Inferiority < t.Inferiority;
        }

        bool InferiorTo(const TBaseTuple& t) const {
            return Inferiority.InferiorTo(t.Inferiority);
        }

        ui64 GetInferiority() const {
            return Inferiority.Inferiority;
        }

        ui64 GetPriority() const {
            return Inferiority.GetPriority32();
        }

        bool HasPriorities() const {
            return WithPriorities;
        }

    private:
        void DoAddSubkey(TStringBuf subkey) {
            if (!Empty) {
                Append(TStringBuf("\t"));
            }
            Append(subkey);
            Empty = false;
        }
        virtual void Append(TStringBuf buf) = 0;
        virtual TStringBuf GetStringBuf() const = 0;

    private:
        TInferiorityClass Inferiority;
        bool Empty = true;
        bool WithPriorities = false;
    };


    class TRawMemoryTuple: public TBaseTuple {
    public:
        TRawMemoryTuple() = default;

        TRawMemoryTuple(char* begin, size_t size)
            : Begin(begin)
            , End(begin)
            , MaxLen(size)
        {
        }

        TRawMemoryTuple(TStringBuf readyquery)
            : Begin((char*) readyquery.begin())
            , End((char*) readyquery.end())
            , MaxLen(readyquery.size())
        {
        }

        size_t Size() const {
            return End - Begin;
        }

    private:
        void Append(TStringBuf subkey) override;

        TStringBuf GetStringBuf() const override {
            return TStringBuf(Begin, End);
        }

    private:
        char* Begin = nullptr;
        char* End = nullptr;
        size_t MaxLen = 0;
    };


    class TBufferTuple: public TBaseTuple {
    public:
        explicit TBufferTuple(size_t len = 0)
                : Buffer(len)
        {
        }

        size_t Size() const {
            return Buffer.Size();
        }

    private:
        void Append(TStringBuf subkey) override {
            Buffer.Append(subkey.data(), subkey.size());
        }

        TStringBuf GetStringBuf() const override {
            return TStringBuf(Buffer.Data(), Buffer.Pos());
        }

    private:
        TBuffer Buffer;
    };

    using TRawTuples = TVector<TRawMemoryTuple>;

    class TSourceFactors;

    void FillSourceFactorsKeys(TSourceFactors& facts, const TRawMemoryTuple& tuple, const TStringBufs&, const TKeyTypes& keyTypes, bool patchKey);

    struct TRawTuplesCache {
        TMemoryPool TuplePool { 1024, TMemoryPool::TLinearGrow::Instance() };
        TStringBufs GoodPrefixes;
        TRawTuples Tuples;

        void Clear() {
            GoodPrefixes.clear();
            Tuples.clear();
            TuplePool.ClearKeepFirstChunk();
        }
    };


    class TSubkeysCache {
        TVector<TStringBufs> SubkeysCache;
        TDeque<TString> CacheStringPool;
        TMemoryPool CacheMemoryPool { 1024 };

    public:
        TSubkeysCache();

        void ResetCache();

        static int FixType(int type);

        bool FillSubkeys(TKeyTypes&, const TRequestRec& req, TStringBuf nSpace);

        TDeque<TString>& StringPool() {
            return CacheStringPool;
        }

        TMemoryPool& MemoryPool() {
            return CacheMemoryPool;
        }

        TStringBufs& GetSubkeysMutable(int type) {
            return SubkeysCache[FixType(type)];
        }

        const TStringBufs& GetSubkeys(int type) const {
            return SubkeysCache[FixType(type)];
        }

        bool KeysNeedPrefixPruning(const TKeyTypes& keyTypes) const;

        const TStringBufs& GetAllPrefixes(const TKeyTypes& keyTypes) const;

        void FillTuplesCache(TRawTuplesCache& cache, const TKeyTypes& keyTypes, bool goodPrefixes = false) const;

        void MakeFakeTuple(TRawTuples& tuples, TStringBuf rawQuery) const;

        void ClearFakeSubkeys();
    };


    class TRawKeys : public TSubkeysCache, public TRawTuplesCache {
    public:
        TKeyTypes KeyTypes;

        bool FillAllSubkeys(const TRequestRec& req, TStringBuf nspace) {
            return FillSubkeys(KeyTypes, req, nspace);
        }

        void GenerateTuples(bool goodPrefixes = false) {
            FillTuplesCache(*this, KeyTypes, goodPrefixes);
        }

        void GenerateFakeTuple(TStringBuf rawQuery) {
            MakeFakeTuple(Tuples, rawQuery);
        }

        void AddGoodPrefix(TStringBuf prefix) {
            GoodPrefixes.push_back(prefix);
        }

        const TStringBufs& Prefixes() const {
            return GetAllPrefixes(KeyTypes);
        }

        bool NeedsPrefixPruning() const {
            return KeysNeedPrefixPruning(KeyTypes);
        }

        void ClearTuples() {
            ClearFakeSubkeys();
            Clear();
        }
    };


    struct TSearchBuffer {
        TRawTuplesCache Cache;
        TKeyTypes KeyTypes;
        TStringBufs Subkeys;
        TString Buffer;
    };
}
