package ru.yandex.ci.client.sandbox;

import lombok.Value;

@Value(staticConstructor = "of")
public class SandboxResponse<T> {
    T data;
    long apiQuotaConsumptionMillis;
    long apiQuotaMillis;

    /**
     * return -1 if unknown
     * Implementation details https://st.yandex-team.ru/CI-3134#61c4a8d0b780e2287566480f
     */
    public int getApiQuotaConsumptionPercent() {
        if (apiQuotaConsumptionMillis == 0) {
            return 0;
        }
        if (apiQuotaMillis < 0 || apiQuotaConsumptionMillis < 0) {
            return -1;
        }
        if (apiQuotaConsumptionMillis >= apiQuotaMillis) {
            return 100;
        }
        return (int) (apiQuotaConsumptionMillis * 100 / apiQuotaMillis);
    }
}
