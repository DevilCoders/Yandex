#pragma once

#include "metatrie_conf.h"

#include <util/generic/yexception.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

namespace NMetatrie {
    class TMetatrieException: public yexception {};

    class TKey;
    class TVal;

    struct IFace : TAtomicRefCount<IFace>, TNonCopyable {
        virtual ~IFace() {
        }
    };

    struct IIterator : IFace {
        virtual bool Next() = 0;
        virtual bool CurrentKey(TKey&) const = 0;
        virtual bool CurrentVal(TVal&) const = 0;
    };

    struct ITrieBase : IFace {
        virtual ESubtrieType GetType() const = 0;
    };

    struct ITrie : ITrieBase {
        virtual TString Report() const = 0;
        virtual bool Get(TStringBuf, TVal&) const = 0;
        virtual IIterator* Iterator() const = 0;
    };

    struct IIndexBase : IFace {
        virtual EIndexType GetType() const = 0;
    };

    struct IIndex : IIndexBase {
        virtual ui64 GetIndex(TStringBuf) const = 0;
        virtual TString Report() const = 0;
    };

    struct TSubtrieBuilderBase;
    struct TIteratorImpl;
    struct TMetatrieImpl;

    struct TKeyImpl;
    struct TValImpl;

    struct TSubtrieBuilderBase : ITrieBase {
        size_t RawKeysSize;
        size_t RawValsSize;

        TString FirstKey;
        TString LastKey;
        TBlob Result;

        TSubtrieBuilderBase()
            : RawKeysSize()
            , RawValsSize()
            , State()
        {
        }

        size_t RawSize() const {
            return RawKeysSize + RawValsSize;
        }

        void Add(TStringBuf key, TStringBuf val) {
            RawKeysSize += key.size();
            RawValsSize += val.size();

            if (StateVirgin == State) {
                FirstKey = key;
                State = StateDating;
            }

            LastKey = key;
            DoAdd(key, val);
        }

        void Commit() {
            if (StateCommited == State)
                return;

            State = StateCommited;
            Result = DoCommit();

            return;
        }

        bool Empty() const {
            return StateVirgin == State;
        }

    protected:
        virtual void DoAdd(TStringBuf, TStringBuf) = 0;
        virtual TBlob DoCommit() = 0;

    private:
        enum EState {
            StateVirgin = 0,
            StateDating = 1,
            StateCommited = 2
        };

        EState State;
    };

    struct IIndexBuilder : IIndexBase {
        virtual TString GenerateMetaKey(TStringBuf key) const = 0;

        virtual void CheckRange(TStringBuf /*first*/, TStringBuf /*last*/) {
        }
        virtual void Add(TStringBuf first, TStringBuf last, size_t count) = 0;

        virtual TBlob Commit() = 0;
    };

}
