#pragma once

#include <library/cpp/numerator/numerate.h>

#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>

namespace NIndexerCore {

class TDirectTextCreator;

class TDirectTextHandler : public INumeratorHandler, private TNonCopyable {
public:
    explicit TDirectTextHandler(TDirectTextCreator& creator);

    void OnTokenStart(const TWideToken&, const TNumerStat&) override;
    void OnSpaces(TBreakType, const wchar16*, unsigned, const TNumerStat&) override;
    void OnMoveInput(const THtmlChunk&, const TZoneEntry*, const TNumerStat&) override;

private:
    TDirectTextCreator& Creator;
    RelevLevel CurRelevLevel;
    int RelevCnt;
    THashSet<TString> Zones;
};

} // namespace NIndexerCore
