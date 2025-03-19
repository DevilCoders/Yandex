#include "init.h"
#include "lemmer_cache.h"
#include <ysite/yandex/reqdata/reqdata.h>
#include <library/cpp/threading/mtp_tasks/tasks.h>
#include <util/thread/pool.h>

namespace {

class TCacheInitTask {
public:
    TCacheInitTask(const TRequesterData& externals, TVector<TString>::const_iterator start, TVector<TString>::const_iterator end, TLemmerCache& cache);

    void ProcessTask();

private:
    const TRequesterData& Externals;
    const TVector<TString>::const_iterator Start;
    const TVector<TString>::const_iterator End;

    TLemmerCache& Cache;
};

TCacheInitTask::TCacheInitTask(const TRequesterData& externals,
        TVector<TString>::const_iterator start, TVector<TString>::const_iterator end, TLemmerCache& cache)
    : Externals(externals)
    , Start(start)
    , End(end)
    , Cache(cache)
{}

void TCacheInitTask::ProcessTask() {
    for (TVector<TString>::const_iterator repr = Start; repr != End; ++repr) {
        NLemmerCache::TKey key;
        key.FromJson(*repr);
        TLanguageContext context(key.LangMask, nullptr, Externals.WordFilter, Externals.Decimator);
        context.FilterForms = key.FilterForms;
        TWordInstance instance;
        InitInstance(key.Word, key.Span, context, key.Type, true, true, instance);
        Cache.Set(key, instance);
    }
}

} // Anonymous namespace.

void InitLemmerCache(IInputStream& keys, ui32 keyCount, const TRequesterData& externals, IThreadPool* queue, TLemmerCache& cache) {
    cache.Clear();
    cache.SetExternals(externals);
    TVector<TString> keyList;
    TString keyRepr;
    for (ui32 keyNumber = 0; keyNumber < keyCount && keys.ReadLine(keyRepr); ++keyNumber) {
        keyList.push_back(keyRepr);
    }
    static const size_t PARALLEL_THREAD_COUNT = 8;
    const size_t step = (keyList.size() + PARALLEL_THREAD_COUNT - 1) / PARALLEL_THREAD_COUNT;
    TFakeThreadPool defaultQueue;
    TSimpleMtpTask<TCacheInitTask> tasks(queue ? queue : &defaultQueue);
    for (size_t start = 0; start < keyList.size(); start += step) {
        const size_t end = Min(start + step, keyList.size());
        tasks.AddAndOwn(new TCacheInitTask(externals, keyList.begin() + start, keyList.begin() + end, cache));
    }
    tasks.Process();
}

void InitInstance(const TUtf16String& word, const TCharSpan& span, const TLanguageContext& context, TFormType type,
                  bool generateForms, bool useFixList, TWordInstance& instance)
{
    instance.Init(word, span, context, type, generateForms, useFixList);
    if (context.FilterForms) {
        TWordInstanceUpdate(instance).ShrinkIntrusiveBastards(useFixList);
        TWordInstanceUpdate(instance).RemoveBadRequest();
    }
    if (instance.NumLemmas() == 1) {
        instance.CleanBestFlag();
    }
}
