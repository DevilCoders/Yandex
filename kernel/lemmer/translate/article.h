#pragma once

#include <util/generic/hash.h>
#include <util/str_stl.h>
#include <util/ysaveload.h>

namespace NTranslationDictInt {
    template <class TChr>
    struct TStrHash {
        size_t operator()(const TChr* str) const {
            return ComputeHash(TBasicStringBuf<TChr>{str});
        }
    };

    template <class TChr>
    struct TStrCmp {
        bool operator()(const TChr* a, const TChr* b) const {
            return TBasicStringBuf<TChr>(a) == b;
        }
    };

    inline void Save(IOutputStream& outputStream, size_t i) {
        ::Save(&outputStream, (ui32)i);
    }
    inline void Load(IInputStream& inputStream, size_t& i) {
        ui32 j = 0;
        ::Load(&inputStream, j);
        i = j;
    }

    struct TArticleInt {
        size_t InitialGr;
        size_t Word;
        size_t Gramm;

        TArticleInt(const size_t& gFrom, size_t wTo, const size_t& gTo)
            : InitialGr(gFrom)
            , Word(wTo)
            , Gramm(gTo)
        {
        }
        TArticleInt()
            : InitialGr()
            , Word()
            , Gramm()
        {
        }

        void Save(IOutputStream& outputStream) const {
            using NTranslationDictInt::Save;
            Save(outputStream, InitialGr);
            Save(outputStream, Word);
            Save(outputStream, Gramm);
        }
        void Load(IInputStream& inputStream) {
            using NTranslationDictInt::Load;
            Load(inputStream, InitialGr);
            Load(inputStream, Word);
            Load(inputStream, Gramm);
        }
    };
}
