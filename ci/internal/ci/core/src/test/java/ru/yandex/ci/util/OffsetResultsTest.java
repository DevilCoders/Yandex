package ru.yandex.ci.util;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class OffsetResultsTest {

    @Test
    void emptyResult() {
        OffsetResults<String> results = OffsetResults.builder()
                .withItems(5, limit -> List.<String>of())
                .withTotal(() -> 0L)
                .fetch();
        assertThat(results.getTotal()).isEqualTo(0);
        assertThat(results.hasMore()).isFalse();
        assertThat(results.items()).isEmpty();
    }

    @Test
    void limitShouldBeOneMore() {
        AtomicLong limitHolder = new AtomicLong();
        OffsetResults.builder()
                .withItems(13, limit -> {
                    limitHolder.set(limit);
                    return List.<String>of();
                })
                .withTotal(() -> 0L)
                .fetch();

        assertThat(limitHolder.get()).isEqualTo(14);
    }

    @Test
    void shouldReturnLimitedValues() {
        OffsetResults<String> results = OffsetResults.builder()
                .withItems(3, limit -> List.of("red", "green", "blue", "white", "orange"))
                .withTotal(() -> 99L)
                .fetch();

        assertThat(results.items()).containsExactly("red", "green", "blue");
        assertThat(results.getTotal()).isEqualTo(99);
        assertThat(results.hasMore()).isTrue();
    }

    @Test
    void hasNoMoreIfReturnExactItems() {
        OffsetResults<String> results = OffsetResults.builder()
                .withItems(3, limit -> List.of("red", "green", "blue"))
                .withTotal(() -> 99L)
                .fetch();

        assertThat(results.items()).containsExactly("red", "green", "blue");
        assertThat(results.getTotal()).isEqualTo(3); // Извлекли настоящее количество элементов
        assertThat(results.hasMore()).isFalse();
    }

    @Test
    void noTotal() {
        OffsetResults<String> results = OffsetResults.builder()
                .withItems(3, limit -> List.of("red", "green", "blue", "white", "orange"))
                .fetch();

        assertThat(results.items()).containsExactly("red", "green", "blue");
        assertThat(results.getTotal()).isNull();
        assertThat(results.hasMore()).isTrue();
    }

    @Test
    void zeroLimit() {
        OffsetResults<String> results = OffsetResults.builder()
                .withItems(0, limit -> List.of("red", "green", "blue", "white", "orange"))
                .withTotal(() -> 99L)
                .fetch();

        assertThat(results.items()).containsExactly("red", "green", "blue", "white", "orange");
        assertThat(results.getTotal()).isEqualTo(99);
        assertThat(results.hasMore()).isFalse();
    }

    @Test
    void moreThanYdbLimit() {
        assertThatThrownBy(() ->
                OffsetResults.builder()
                        .withItems(1001, limit -> List.of(""))
                        .withTotal(() -> 99L)
                        .fetch()
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessage("limit cannot be greater than 1000, got 1001");
    }

    @Test
    void exactlyYdbLimit() {
        var requestedLimit = new AtomicInteger();
        var results = OffsetResults.builder()
                .withItems(1000, limit -> {
                    requestedLimit.set(limit);
                    return Collections.nCopies(limit, "some string");
                })
                .fetch();

        assertThat(results.hasMore()).isTrue();
        assertThat(results.items()).hasSize(1000);
        assertThat(requestedLimit.get()).isEqualTo(1000);
    }
}
