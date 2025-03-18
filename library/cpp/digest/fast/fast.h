#pragma once

#include <library/cpp/digest/crc32c/crc32c.h>
#include <util/digest/city.h>
#include <util/digest/numeric.h>
#include <util/random/fast.h>

/*
 * Очень быстрый хеш для блоков среднего размера и больше
 * Результат для одних и тех же данных может отличаться для разных процессоров, платформ и даже операционных систем
 * Поэтому не может быть использовал для постоянного хранения и передачи между машинами
 */

static inline ui32 FastHash32(const void* data, size_t len) noexcept {
    if (HaveFastCrc32c()) {
        return Crc32c(data, len);
    }

    return TPCGMixer::Mix(CityHash64((const char*)data, len));
}

/*
 * Может содержать меньше 64 значимых бит
 */
static inline ui64 FastHash64(const void* data, size_t len) noexcept {
    if (HaveFastCrc32c()) {
        return CombineHashes<ui64>(Crc32c(data, len), len);
    }

    return CityHash64((const char*)data, len);
}
