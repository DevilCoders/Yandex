#pragma once

#include "common.h"

#include <library/cpp/containers/comptrie/comptrie_trie.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/containers/comptrie/protopacker.h>

namespace NWordFeatures {

    static const char* SURF_META_INFO =
"\n\
repeated data id 0\n\
    scalar value id  0 default const 0\n\
end\n";

    void BuildSurfIndex(const TString& srcFile, const TString& outputFile, bool optimizeTrie = false);

}; // NWordFeatures

inline void SurfMetaInfoNoWarnings()
{
    Y_UNUSED(NWordFeatures::SURF_META_INFO);
}
