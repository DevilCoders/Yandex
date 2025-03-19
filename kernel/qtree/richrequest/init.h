#pragma once

#include <library/cpp/token/formtype.h>
#include <util/generic/fwd.h>
#include <util/system/defaults.h>

struct TCharSpan;
class TLanguageContext;
class TWordInstance;
struct TRequesterData;
class TLemmerCache;
class IInputStream;
class IThreadPool;

void InitInstance(const TUtf16String& Word, const TCharSpan& Span, const TLanguageContext& context, TFormType Type,
                  bool generateForms, bool useFixList, TWordInstance& instance);
void InitLemmerCache(IInputStream& keys, ui32 keyCount, const TRequesterData& data, IThreadPool* queue, TLemmerCache& cache);
