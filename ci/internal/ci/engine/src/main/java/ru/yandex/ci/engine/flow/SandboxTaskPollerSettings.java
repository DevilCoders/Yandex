package ru.yandex.ci.engine.flow;

import java.time.Clock;
import java.time.Duration;

import com.google.common.base.Preconditions;
import lombok.Value;

@Value
public class SandboxTaskPollerSettings {
    Clock clock;
    Duration minPollingInterval;
    Duration maxPollingInterval;
    Duration maxWaitOnQuotaExceeded;
    double waitScalePow;

    /**
     * Ручка sandbox'а task/audit работает в несколько раз быстрее (и соответственно потребляет меньше квоты)
     * Бенчмарки - https://st.yandex-team.ru/CI-1190#60ec62a37fbb3061ae4e6ead
     * Однако не дает 100% гарантий и в редкий случаях обновления туда могут не попадать.
     * Поэтому раз в N запросов мы дергаем обычную ручку получения статуса.
     * Так же может получиться, что из-за настройки updateTaskOutputEachNRequests
     * это условие никогда не будет выполняться.
     * Это нормально, т.к. TaskOutput содержит валидный и актуальный статус
     */
    int notUseAuditEachNRequests;

    int updateTaskOutputEachNRequests;

    int maxUnexpectedErrorsInRow;

    public SandboxTaskPollerSettings(Clock clock,
                                     Duration minPollingInterval,
                                     Duration maxPollingInterval,
                                     Duration maxWaitOnQuotaExceeded,
                                     int notUseAuditEachNRequests,
                                     int updateTaskOutputEachNRequests,
                                     int maxUnexpectedErrorsInRow) {
        this.clock = clock;
        this.minPollingInterval = minPollingInterval;
        this.maxPollingInterval = maxPollingInterval;
        this.maxWaitOnQuotaExceeded = maxWaitOnQuotaExceeded;
        this.notUseAuditEachNRequests = notUseAuditEachNRequests;
        this.updateTaskOutputEachNRequests = updateTaskOutputEachNRequests;
        this.maxUnexpectedErrorsInRow = maxUnexpectedErrorsInRow;
        Preconditions.checkArgument(minPollingInterval.toMillis() >= 0);
        Preconditions.checkArgument(
                minPollingInterval.toMillis() <= maxPollingInterval.toMillis(),
                "minPoolingInterval (%s) should be <= maxPoolingInterval (%s)",
                minPollingInterval, maxPollingInterval
        );
        waitScalePow = Math.log((double) maxPollingInterval.toMillis()) /
                Math.log((double) minPollingInterval.toMillis());
    }
}
