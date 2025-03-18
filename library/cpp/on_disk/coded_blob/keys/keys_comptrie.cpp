#include "keys_comptrie.h"

namespace NCodedBlob {
    void TCompactTrieKeys::Init(TBlob b) {
        TMemoryInput min(b.AsCharPtr(), b.Size());
        ::Load(&min, EmptyKeyOffset);
        b = b.SubBlob(b.Size() - min.Avail(), b.Size());
        Keys.Init(b);
    }

    TCompactTrieKeys::TIterator::TIterator(const TCompactTrieKeys* keys)
        : Keys(keys)
        , NextStep(keys ? keys->Keys.Begin() : TPrimaryKeys::TConstIterator())
        , End(keys ? keys->Keys.End() : TPrimaryKeys::TConstIterator())
        , IteratorState(!keys ? IS_NONE : IS_NOT_READY)
    {
    }

    bool TCompactTrieKeys::TIterator::Next() {
        if (!HasNext()) {
            return false;
        }

        KeyHelper.Invalidate();

        switch (IteratorState) {
            case IS_NONE:
                Y_ENSURE_EX(false, TCodedBlobException() << "invalid iterator state");
                break;
            case IS_NOT_READY:
                if (Keys->HasEmptyKey()) {
                    IteratorState = IS_EMPTY_KEY;
                    return true;
                }
                [[fallthrough]]; /*no break*/
            case IS_EMPTY_KEY:
            case IS_TRIE:
                if (IS_TRIE != IteratorState) {
                    IteratorState = IS_TRIE;
                    Begin = Keys->Keys.Begin();
                } else {
                    ++Begin;
                }
                ++NextStep;
                break;
        }
        return true;
    }

}
