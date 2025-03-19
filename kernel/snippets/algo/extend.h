#pragma once

namespace NSnippets {

class IRestr;
class TSentsMatchInfo;
class TSnip;
class TWordSpanLen;

namespace NSnippetExtend {

    TSnip GetSnippet(const TSnip& mainSnip, const IRestr& restr, const TSentsMatchInfo& sentsMatchInfo, const TWordSpanLen& wordSpanLen, int maxSize);

} }
