package ru.yandex.ci.common.temporal;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

class TemporalCronUtilsTest {

    @Test
    void everyMinute() {
        Assertions.assertThat(TemporalCronUtils.everyMinute().asString()).isEqualTo("* * * * *");
    }
}
