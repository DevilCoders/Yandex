#pragma once

#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NMango {
    class TPortionTags {
    private:
        TVector<TString> DelayedTags;
        TString PortionTagDir;
        TString TagType;

        TFsPath GetTagDir() const;
        TFsPath GetTagFile(const TString &tag) const;

    public:
        TPortionTags(const TString& tagType);

        void SetTag(const TString& tag, bool delay = false);
        void SetDelayedTags();
        bool HasTag(const TString& tag) const;
        const THashSet<TString> GetTags() const;
    };
}
