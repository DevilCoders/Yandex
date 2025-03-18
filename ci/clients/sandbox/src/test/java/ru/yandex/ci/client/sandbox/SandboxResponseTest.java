package ru.yandex.ci.client.sandbox;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

class SandboxResponseTest {
    @Test
    void longValues() {
        Assertions.assertThat(consumptionPercent(72714898, 194400000)).isEqualTo(37);
    }

    @Test
    void consumptionMoreThanQuota() {
        Assertions.assertThat(consumptionPercent(194400000, 10)).isEqualTo(100);
    }

    @Test
    void unknown() {
        Assertions.assertThat(consumptionPercent(-1, 42)).isEqualTo(-1);
        Assertions.assertThat(consumptionPercent(42, -1)).isEqualTo(-1);
        Assertions.assertThat(consumptionPercent(-1, -1)).isEqualTo(-1);
    }

    @Test
    void zeroConsumptionWithUnknownQuota() {
        Assertions.assertThat(consumptionPercent(0, -1)).isEqualTo(0);
    }

    private int consumptionPercent(long apiQuotaConsumptionMillis, long apiQuotaMillis) {
        return SandboxResponse.of(null, apiQuotaConsumptionMillis, apiQuotaMillis).getApiQuotaConsumptionPercent();
    }
}
