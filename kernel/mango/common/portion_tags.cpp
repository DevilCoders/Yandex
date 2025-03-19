#include "portion_tags.h"

#include <util/stream/pipe.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <util/system/env.h>

namespace NMango {
    TFsPath TPortionTags::GetTagDir() const {
        return TFsPath(PortionTagDir) / TagType;
    }

    TFsPath TPortionTags::GetTagFile(const TString &tag) const {
        return GetTagDir() / tag;
    }

    TPortionTags::TPortionTags(const TString &tagType)
        : TagType(tagType)
    {
        TString portionTagDir = GetEnv("PORTION_TAG_DIR");
        if (!portionTagDir) {
            ythrow yexception() << "PORTION_TAG_DIR should be set";
        }
        PortionTagDir = portionTagDir;
    }

    void TPortionTags::SetTag(const TString& tag, bool delay) {
        if (delay) {
            DelayedTags.push_back(tag);
        } else {
            GetTagDir().MkDirs();
            GetTagFile(tag).Touch();
        }
    }

    void TPortionTags::SetDelayedTags() {
        for (TVector<TString>::const_iterator it = DelayedTags.begin(); it != DelayedTags.end(); ++it) {
            SetTag(*it, false);
        }
        DelayedTags.clear();
    }

    bool TPortionTags::HasTag(const TString& tag) const {
        return GetTagFile(tag).Exists();
    }

    const THashSet<TString> TPortionTags::GetTags() const {
        TVector<TString> result;
        GetTagDir().MkDirs();
        GetTagDir().ListNames(result);
        return THashSet<TString>(result.begin(), result.end());
    }
}

