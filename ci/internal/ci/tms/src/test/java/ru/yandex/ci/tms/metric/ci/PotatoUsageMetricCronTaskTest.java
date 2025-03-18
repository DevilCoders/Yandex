package ru.yandex.ci.tms.metric.ci;

import java.time.Duration;
import java.time.Instant;
import java.util.Map;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.ci.tms.task.potato.PotatoClient;
import ru.yandex.ci.tms.task.potato.client.Status;

import static java.util.Collections.emptyMap;
import static java.util.function.Function.identity;
import static java.util.stream.Collectors.toMap;
import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;

class PotatoUsageMetricCronTaskTest extends YdbCiTestBase {

    @MockBean
    private PotatoClient potatoClient;
    private OverridableClock clock;
    private PotatoUsageMetricCronTask task;

    @BeforeEach
    public void setUp() {
        clock = new OverridableClock();
        task = new PotatoUsageMetricCronTask(db, null, clock, potatoClient);
        clock.setTime(Instant.parse("2007-07-15T00:00:00.00Z"));
    }

    @Test
    void countMetrics() {
        when(potatoClient.healthCheck("rootLoader.parsing.json.*"))
                .thenReturn(healths("potato/config-metrics-json.txt"));
        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*"))
                .thenReturn(healths("potato/config-metrics-yaml.txt"));

        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer();
        task.computeMetric(metricsConsumer);

        assertThat(metricsConsumer.getMetrics())
                .containsExactlyInAnyOrderEntriesOf(Map.of(
                        "ci_system_usage:system=Potato:type=projects", 63.0,
                        "ci_system_usage:system=Potato:type=arcadiaProjects", 2.0,
                        "ci_system_usage:system=Potato:type=githubProjects", 32.0,
                        "ci_system_usage:system=Potato:type=bitbucketProjects", 29.0,
                        "ci_system_usage:system=Potato:type=bitbucketPublicProjects", 1.0,
                        "ci_system_usage:system=Potato:type=bitbucketEnterpriseProjects", 27.0,
                        "ci_system_usage:system=Potato:type=bitbucketBrowserProjects", 1.0,
                        "ci_system_usage:system=Potato:type=githubPublicProjects", 1.0,
                        "ci_system_usage:system=Potato:type=githubEnterpriseProjects", 31.0
                ));
    }

    @Test
    void window() {
        Map<String, Status> json = healths("potato/config-metrics-json.txt");
        Map<String, Status> yaml = healths("potato/config-metrics-yaml.txt");

        long longAgoTimestamp = clock.instant().minus(Duration.ofDays(70)).toEpochMilli();
        Status seenLongAgo = new Status(true, longAgoTimestamp);

        json.put("rootLoader.parsing.json.bitbucket-enterprise/AFISHA/domiki", seenLongAgo);
        yaml.put("rootLoader.parsing.yaml.arcanum/arcadia/music", seenLongAgo);

        when(potatoClient.healthCheck("rootLoader.parsing.json.*")).thenReturn(json);
        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*")).thenReturn(yaml);

        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer();
        task.computeMetric(metricsConsumer);

        assertThat(metricsConsumer.getMetrics())
                .containsExactlyInAnyOrderEntriesOf(Map.of(
                        "ci_system_usage:system=Potato:type=projects", 61.0,
                        "ci_system_usage:system=Potato:type=arcadiaProjects", 1.0,
                        "ci_system_usage:system=Potato:type=githubProjects", 32.0,
                        "ci_system_usage:system=Potato:type=bitbucketProjects", 28.0,
                        "ci_system_usage:system=Potato:type=bitbucketPublicProjects", 1.0,
                        "ci_system_usage:system=Potato:type=bitbucketEnterpriseProjects", 26.0,
                        "ci_system_usage:system=Potato:type=bitbucketBrowserProjects", 1.0,
                        "ci_system_usage:system=Potato:type=githubPublicProjects", 1.0,
                        "ci_system_usage:system=Potato:type=githubEnterpriseProjects", 31.0
                ));
    }

    @Test
    void useOnlyFreshData() {
        String config = "rootLoader.parsing.yaml.arcanum/arcadia/music";
        var now = clock.instant();
        var health = Map.of(config, new Status(true, now.toEpochMilli()));

        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*")).thenReturn(health);
        when(potatoClient.healthCheck("rootLoader.parsing.json.*")).thenReturn(emptyMap());

        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer();
        task.computeMetric(metricsConsumer);

        assertThat(metricsConsumer.getMetrics())
                .containsEntry("ci_system_usage:system=Potato:type=arcadiaProjects", 1.0);

        reset(potatoClient);
        health = Map.of(config, new Status(true, now.minus(Duration.ofDays(250)).toEpochMilli()));
        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*")).thenReturn(health);
        when(potatoClient.healthCheck("rootLoader.parsing.json.*")).thenReturn(emptyMap());

        metricsConsumer = new FakeMetricsConsumer();
        task.computeMetric(metricsConsumer);

        assertThat(metricsConsumer.getMetrics())
                .containsEntry("ci_system_usage:system=Potato:type=arcadiaProjects", 1.0);
    }

    @Test
    void dontRemoveIfPotatoForgot() {
        var health = Map.of("rootLoader.parsing.yaml.arcanum/arcadia/music", new Status(true,
                clock.instant().toEpochMilli()));

        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*")).thenReturn(health);
        when(potatoClient.healthCheck("rootLoader.parsing.json.*")).thenReturn(emptyMap());

        task.computeMetric(new FakeMetricsConsumer());

        reset(potatoClient);
        when(potatoClient.healthCheck("rootLoader.parsing.yaml.*")).thenReturn(emptyMap());
        when(potatoClient.healthCheck("rootLoader.parsing.json.*")).thenReturn(emptyMap());

        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer();
        task.computeMetric(metricsConsumer);

        assertThat(metricsConsumer.getMetrics())
                .containsEntry("ci_system_usage:system=Potato:type=arcadiaProjects", 1.0);
    }

    private Map<String, Status> healths(String sourcePath) {
        return TestUtils.textResource(sourcePath).lines()
                .collect(toMap(identity(), key -> new Status(true, clock.instant().toEpochMilli())));
    }
}
