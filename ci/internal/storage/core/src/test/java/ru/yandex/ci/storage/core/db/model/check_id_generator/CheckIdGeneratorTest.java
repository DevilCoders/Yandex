package ru.yandex.ci.storage.core.db.model.check_id_generator;

import java.util.HashSet;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

public class CheckIdGeneratorTest {
    @Test
    public void test() {
        var result = CheckIdGenerator.generate(0L, 10000);

        assertThat(result.get(0)).isEqualTo(100000000000L);
        assertThat(result.get(5)).isEqualTo(600000000000L);
        assertThat(result.get(998)).isEqualTo(99900000000000L);
        assertThat(result.get(999)).isEqualTo(100000000001L);
        assertThat(result.get(1000)).isEqualTo(200000000001L);

        assertThat(result).hasSize(new HashSet<>(result).size());
    }
}
