package ru.yandex.ci.core.proto;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;

import static org.assertj.core.api.Assertions.assertThat;

class InfoPanelUtilsTest {

    @CsvSource(textBlock = """
            1, 1 second
            59, 59 seconds
            60, 1 minute
            61, 1.02 minutes
            3599, 59.98 minutes
            3600, 1 hour
            3601, 1 hour
            3630, 1.01 hours
            3123100, 867.53 hours
            """)
    @ParameterizedTest
    void formatSeconds(double value, String formatted) {
        assertThat(InfoPanelUtils.formatSeconds(value)).isEqualTo(formatted);
    }
}
