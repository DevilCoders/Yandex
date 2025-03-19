#pragma once

#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/doccodes.h>
#include <util/memory/blob.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

#include "common.h"

namespace NLexicalDecomposition {
    class TSpecifiedVocabularyBuilder : TNonCopyable {
    public:
        bool InsertWord(const TUtf16String& word, const TWordAdditionalInfo& value);
        void Save(IOutputStream& output);
        ui32 Size() const {
            return Id2InfoBuilder.Size();
        }

    private:
        typedef TAhoCorasickBuilder<TUtf16String, ui32, TSingleContainerBuilder<ui32>> TAhoBuilder;

        TCompactTrieBuilder<wchar16, ui32> Word2IdBuilder;
        TYVectorWriter<TWordAdditionalInfo> Id2InfoBuilder;
        TAhoBuilder AhoBuilder;
    };

    class TMultiVocabularyBuilder : TNonCopyable {
    public:
        TMultiVocabularyBuilder(bool verbose = false);

        /*
         * Vocabulary file format: <word> [<frequency> [<word`s type>]],
         * type: 0 - orfo, 1 - not orfo but frequent, 2 - stop word, 3 - unknown type
         */
        void LoadVocabulary(ELanguage language, const TString& filename, ECharset charset = CODES_WIN);
        void LoadVocabulary(ELanguage language, IInputStream& input, ECharset charset = CODES_WIN);

        void Save(IOutputStream& output) const;
        const TBlob Save() const;
        void Update(const TBlob& input, IOutputStream& output) const;

    private:
        typedef TSimpleSharedPtr<TSpecifiedVocabularyBuilder> TVocPtr;

        TSpecifiedVocabularyBuilder& Voc(ELanguage language);

        TMap<ELanguage, TVocPtr> Lang2Voc;
        bool Verbose;
    };

}
