package ru.yandex.ci.ayamler;

import java.nio.file.Path;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import lombok.Value;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class AYamlerServiceHelperTest extends AYamlerTestBase {

    private final AtomicInteger cacheLoadCount = new AtomicInteger();

    @BeforeEach
    void setUp() {
        cacheLoadCount.set(0);
    }

    @Test
    void loadFuturesFromCache_shouldNotReloadCompletedFutures() {
        Map<CacheKey, CompletableFuture<String>> valueProviderMap = Map.of(
                CacheKey.of("path"), CompletableFuture.completedFuture("path")
        );

        Cache<CacheKey, CompletableFuture<String>> cache = CacheBuilder.newBuilder().build();

        assertThat(loadFuturesFromCache("path", cache, valueProviderMap))
                .extracting(map -> map.get(CacheKey.of("path")).getNow(null))
                .isEqualTo("path");
        assertThat(cacheLoadCount)
                .describedAs("Should ask value from provider for uncached key")
                .hasValue(1);

        assertThat(loadFuturesFromCache("path", cache, valueProviderMap))
                .extracting(map -> map.get(CacheKey.of("path")).getNow(null))
                .isEqualTo("path");
        assertThat(cacheLoadCount)
                .describedAs("Should not ask value from provider for cached key")
                .hasValue(1);
    }

    @Test
    void loadFuturesFromCache_shouldReloadFailedFutures() {
        Map<CacheKey, CompletableFuture<String>> valueProviderMap = Map.of(
                CacheKey.of("exception"), CompletableFuture.failedFuture(new RuntimeException("exception"))
        );

        Cache<CacheKey, CompletableFuture<String>> cache = CacheBuilder.newBuilder().build();

        assertThat(loadFuturesFromCache("exception", cache, valueProviderMap))
                .extractingByKey(CacheKey.of("exception"))
                .matches(CompletableFuture::isCompletedExceptionally);
        assertThat(cacheLoadCount)
                .describedAs("Should ask value from provider for uncached key")
                .hasValue(1);

        assertThat(loadFuturesFromCache("exception", cache, valueProviderMap))
                .extractingByKey(CacheKey.of("exception"))
                .matches(CompletableFuture::isCompletedExceptionally);
        assertThat(cacheLoadCount)
                .describedAs("Should ask value from provider for cached future, that is finished exceptionally")
                .hasValue(2);
    }

    @Test
    void loadFuturesFromCache_shouldNotReloadFuturesInProgress() {
        Map<CacheKey, CompletableFuture<String>> valueProviderMap = Map.of(
                CacheKey.of("not-finished-future"), new CompletableFuture<>()
        );

        Cache<CacheKey, CompletableFuture<String>> cache = CacheBuilder.newBuilder().build();

        assertThat(loadFuturesFromCache("not-finished-future", cache, valueProviderMap))
                .extractingByKey(CacheKey.of("not-finished-future"))
                .matches(future -> !future.isDone());
        assertThat(cacheLoadCount)
                .describedAs("Should ask value from provider for uncached key")
                .hasValue(1);

        assertThat(loadFuturesFromCache("not-finished-future", cache, valueProviderMap))
                .extractingByKey(CacheKey.of("not-finished-future"))
                .matches(future -> !future.isDone());
        assertThat(cacheLoadCount)
                .describedAs("Should not ask value from provider for cached future, that is not finished yet")
                .hasValue(1);
    }

    private Map<CacheKey, CompletableFuture<String>> loadFuturesFromCache(
            String cacheKey,
            Cache<CacheKey, CompletableFuture<String>> cache,
            Map<CacheKey, CompletableFuture<String>> valueProviderMap
    ) {
        return AYamlerServiceHelper.loadFuturesFromCache(
                cache,
                keys -> {
                    cacheLoadCount.incrementAndGet();
                    return keys.stream().collect(Collectors.toMap(
                            Function.identity(), valueProviderMap::get
                    ));
                },
                Set.of(CacheKey.of(cacheKey))
        );
    }

    @Test
    void loadFromCache() {
        Map<CacheKey, String> valueProviderMap = Map.of(
                CacheKey.of("path"), "path",
                CacheKey.of("another-path"), "another-path"
        );

        Cache<CacheKey, String> cache = CacheBuilder.newBuilder().build();

        assertThat(loadFromCache("path", cache, valueProviderMap)).isEqualTo(Map.of(
                CacheKey.of("path"), "path"
        ));
        assertThat(cacheLoadCount).hasValue(1);
        assertThat(loadFromCache("path", cache, valueProviderMap)).isEqualTo(Map.of(
                CacheKey.of("path"), "path"
        ));
        assertThat(cacheLoadCount).hasValue(1);

        assertThat(loadFromCache("another-path", cache, valueProviderMap)).isEqualTo(Map.of(
                CacheKey.of("another-path"), "another-path"
        ));
        assertThat(cacheLoadCount).hasValue(2);
        assertThat(loadFromCache("another-path", cache, valueProviderMap)).isEqualTo(Map.of(
                CacheKey.of("another-path"), "another-path"
        ));
        assertThat(cacheLoadCount).hasValue(2);
    }

    private Map<CacheKey, String> loadFromCache(
            String cacheKey,
            Cache<CacheKey, String> cache,
            Map<CacheKey, String> valueProviderMap
    ) {
        return AYamlerServiceHelper.loadFromCache(
                cache,
                keys -> {
                    cacheLoadCount.incrementAndGet();
                    return keys.stream().collect(Collectors.toMap(
                            Function.identity(), valueProviderMap::get
                    ));
                },
                Set.of(CacheKey.of(cacheKey))
        );
    }

    @SuppressWarnings("MissingOverride")
    @Value(staticConstructor = "of")
    private static class CacheKey implements PathGetter {
        Path path;

        private static CacheKey of(String path) {
            return CacheKey.of(Path.of(path));
        }
    }

}
