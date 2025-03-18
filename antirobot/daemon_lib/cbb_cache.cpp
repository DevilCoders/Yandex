#include "cbb_cache.h"

#include <antirobot/idl/cbb_cache.pb.h>
#include <antirobot/lib/ar_utils.h>

#include <google/protobuf/messagext.h>

#include <util/stream/file.h>
#include <util/system/rwlock.h>

namespace NAntiRobot {

TCbbCache::TCbbCache(const TFsPath& filename) {
    try {
        Load(filename);
    } catch (TFileError&) {
        /*
        * Если не удалось открыть файл с кешем ебб, мы продолжаем
        * работать. Например, при первом запуске на машине ещё нет файла с дампом,
        * это не повод ронять роботоловилку.
        *
        * Если же удалось открыть файл, но возникли ошибки при его чтении, мы
        * должны сообщить об этом наружу и аварийно завершиться. Именно поэтому здесь
        * перехватывается только TFileError.
        */
    }
}

TMaybe<TString> TCbbCache::Get(const TString& req) {
    TGuard<TMutex> guard(Mutex);
    if (const auto* result = Cache.FindPtr(req)) {
        return *result;
    }
    return Nothing();
}

void TCbbCache::Set(const TString& req, const TString& content) {
    TGuard<TMutex> guard(Mutex);
    Cache[req] = content;
}

TCbbCache::TCache TCbbCache::GetCopy() const {
    TGuard<TMutex> guard(Mutex);
    TCache result = Cache;
    return result;
}

void TCbbCache::Load(const TFsPath& filename) {
    TGuard<TMutex> guard(Mutex);

    TIFStream in(filename);
    TCbbCacheItem cacheItem;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&cacheItem, &adaptor)) {
        Cache[cacheItem.GetRequest()] = cacheItem.GetContent();
    }
}

void SaveCbbCache(const TCbbCache::TCache& cache, const TFsPath& filename) {
    static TMutex SaveMutex;
    FlushToFileIfNotLocked(cache, filename, SaveMutex);
}

} // namespace NAntiRobot

template <>
void Out<NAntiRobot::TCbbCache::TCache>(IOutputStream& out, const NAntiRobot::TCbbCache::TCache& cache) {
    NProtoBuf::io::TCopyingOutputStreamAdaptor streamAdaptor(&out);
    for (const auto& [req, content] : cache) {
        NAntiRobot::TCbbCacheItem cacheItem;
        cacheItem.SetRequest(req);
        cacheItem.SetContent(content);
        
        Y_ENSURE(
            NProtoBuf::io::SerializeToZeroCopyStreamSeq(&cacheItem, &streamAdaptor),
            "Failed to serialize TCbbCacheItem"
        );
    }
    streamAdaptor.Flush();
}
