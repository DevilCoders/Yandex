package ru.yandex.ci.engine.discovery.tier0;

import java.util.function.Function;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class LruCacheTest {

    @Test
    void lruCache() {
        var cache = new LruCache<Integer, Integer>(2, Function.identity());

        cache.get(1);
        assertThat(cache.getMap().keySet()).containsExactly(1);

        cache.get(2);
        assertThat(cache.getMap().keySet()).containsExactly(1, 2);

        cache.get(1);
        assertThat(cache.getMap().keySet()).containsExactly(2, 1);

        cache.get(3);
        assertThat(cache.getMap().keySet()).containsExactly(1, 3);

        cache.get(4);
        assertThat(cache.getMap().keySet()).containsExactly(3, 4);
    }

}
