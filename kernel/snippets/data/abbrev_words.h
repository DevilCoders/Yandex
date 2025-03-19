#pragma once

#include <util/stream/input.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

class TTrieSet;

class TAbbrevContainer {
    THolder<TTrieSet> AbbrevWords;
public:
    TAbbrevContainer();
    ~TAbbrevContainer();
    void LoadFromStream(IInputStream& in);
    void LoadDefault();
    void LoadFromFile(const TString& file);
    bool Contains(const TString& s) const;
    bool Contains(const TUtf16String& w) const;
    static const TAbbrevContainer& GetDefault();
};
