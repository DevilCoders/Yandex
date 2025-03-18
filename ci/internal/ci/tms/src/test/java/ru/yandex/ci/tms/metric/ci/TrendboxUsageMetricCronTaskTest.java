package ru.yandex.ci.tms.metric.ci;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import ru.yandex.ci.client.trendbox.TrendboxClient;
import ru.yandex.ci.client.trendbox.model.TrendboxScpType;
import ru.yandex.ci.client.trendbox.model.TrendboxWorkflow;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;

import static org.assertj.core.api.Assertions.assertThat;

class TrendboxUsageMetricCronTaskTest extends YdbCiTestBase {

    @Test
    void createMetrics() {

        TrendboxClient trendboxClient = Mockito.mock(TrendboxClient.class);

        List<TrendboxWorkflow> flows = List.of(
                new TrendboxWorkflow(
                        TrendboxScpType.ARCADIA,
                        "arcadia:/frontend",
                        "build",
                        Instant.parse("2021-04-01T12:53:49.071Z"),
                        Instant.parse("2021-04-15T12:50:14.503Z"),
                        Instant.parse("2021-04-15T12:45:35.096Z")
                ),
                new TrendboxWorkflow(
                        TrendboxScpType.GITHUB,
                        "git@github.yandex-team.ru:IMS/vh-selfservice.git",
                        "tkit",
                        Instant.parse("2021-04-01T12:54:00.088Z"),
                        Instant.parse("2021-04-12T19:42:50.546Z"),
                        Instant.parse("2021-04-12T19:57:42.387Z")
                ),
                new TrendboxWorkflow(
                        TrendboxScpType.BITBUCKET,
                        "git@github.yandex-team.ru:IMS/vh-selfservice.git",
                        "tkit",
                        Instant.parse("2021-04-01T12:54:00.088Z"),
                        Instant.parse("2021-04-29T19:42:50.546Z"),
                        null
                )
        );

        Mockito.when(trendboxClient.getWorkflows()).thenReturn(flows);

        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer(Instant.parse("2021-04-15T19:57:42.387Z"));

        TrendboxUsageMetricCronTask cronTask = new TrendboxUsageMetricCronTask(db, null, trendboxClient);
        cronTask.computeMetric(metricsConsumer);

        Map<String, Double> expected = new HashMap<>();

        expected.putAll(Map.of(
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=total:windowDays=1", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=total:windowDays=1", 2.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=total:windowDays=7", 2.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=total:windowDays=7", 3.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=total:windowDays=14", 2.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=total:windowDays=14", 3.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=total:windowDays=30", 2.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=total:windowDays=30", 3.0
        ));


        expected.putAll(Map.of(
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=arcadia:windowDays=1", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=arcadia:windowDays=1", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=arcadia:windowDays=7", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=arcadia:windowDays=7", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=arcadia:windowDays=14", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=arcadia:windowDays=14", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=arcadia:windowDays=30", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=arcadia:windowDays=30", 1.0
        ));

        expected.putAll(Map.of(
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=bitbucket:windowDays=1", 0.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=bitbucket:windowDays=1", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=bitbucket:windowDays=7", 0.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=bitbucket:windowDays=7", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=bitbucket:windowDays=14", 0.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=bitbucket:windowDays=14", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=bitbucket:windowDays=30", 0.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=bitbucket:windowDays=30", 1.0
        ));

        expected.putAll(Map.of(
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=github:windowDays=1", 0.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=github:windowDays=1", 0.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=github:windowDays=7", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=github:windowDays=7", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=github:windowDays=14", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=github:windowDays=14", 1.0,
                "ci_system_usage_active:status=success:system=Trendbox:type=flows:vcs=github:windowDays=30", 1.0,
                "ci_system_usage_active:status=all:system=Trendbox:type=flows:vcs=github:windowDays=30", 1.0
        ));
        assertThat(metricsConsumer.getMetrics()).containsExactlyInAnyOrderEntriesOf(expected);

    }

    @Test
    void metricIds() {
        Assertions.assertThat(TrendboxUsageMetricCronTask.getMetricIds()).hasSize(32);
    }
}
