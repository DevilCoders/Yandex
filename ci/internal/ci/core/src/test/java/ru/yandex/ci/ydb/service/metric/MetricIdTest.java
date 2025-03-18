package ru.yandex.ci.ydb.service.metric;

import java.util.List;

import io.micrometer.core.instrument.Tag;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class MetricIdTest {

    @Test
    void tags() {
        String id = "name:key1=value1:key2=value2";
        MetricId metricId = MetricId.of("name", List.of(Tag.of("key1", "value1"), Tag.of("key2", "value2")));

        assertThat(metricId).isEqualTo(MetricId.ofString(id));
        assertThat(metricId.asString()).isEqualTo(id);
    }

    @Test
    void noTag() {
        String id = "name";
        MetricId metricId = MetricId.of("name", List.of());

        assertThat(metricId).isEqualTo(MetricId.ofString(id));
        assertThat(metricId.asString()).isEqualTo(id);
    }
}
