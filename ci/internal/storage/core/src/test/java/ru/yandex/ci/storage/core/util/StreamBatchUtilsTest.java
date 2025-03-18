package ru.yandex.ci.storage.core.util;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.utils.StreamBatchUtils;
import ru.yandex.ci.util.ObjectStore;

import static org.assertj.core.api.Assertions.assertThat;

public class StreamBatchUtilsTest {

    @Test
    public void test() {
        var ints = List.of(1, 2, 3, 4, 5);
        var sum = new ObjectStore<>(0);
        StreamBatchUtils.process(ints.stream(), 2, batch -> batch.forEach(element -> sum.set(sum.get() + element)));

        assertThat(sum.get()).isEqualTo(15);
    }
}
