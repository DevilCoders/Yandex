package ru.yandex.ci.tms.task;

import java.nio.file.Path;
import java.time.Duration;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Activity;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchStatistics;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.client.SolomonMetric;
import ru.yandex.ci.tms.client.SolomonMetrics;
import ru.yandex.ci.tms.spring.CiTmsPropertiesTestConfig;
import ru.yandex.ci.tms.spring.tasks.CiTaskConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ContextConfiguration(classes = {
        CiTaskConfig.class,
        JugglerClientTestConfig.class,
        CiTmsPropertiesTestConfig.class
})
class LaunchMetricsExportCronTaskTest extends EngineTestBase {
    private static final long SEED = 2082556104L;

    @Autowired
    private LaunchMetricsExportCronTask task;

    @MockBean
    private SolomonClient solomonClient;

    @Captor
    private ArgumentCaptor<SolomonMetrics> metrics;

    EnhancedRandom random;

    int launchNumber;

    @BeforeEach
    public void setUp() {
        launchNumber = 0;
        when(solomonClient.push(any(), any()))
                .thenReturn(new SolomonClient.PushResponse(null, -1, SolomonClient.PushStatus.OK));

        random = EnhancedRandomBuilder.aNewEnhancedRandomBuilder()
                .seed(SEED)
                .build();
    }

    @Test
    void basic() {
        var launches = IntStream.range(1, 50)
                .mapToObj(number -> {
                    var path = Path.of(Paths.get(random), "a.yaml");
                    var id = Ids.get(random);
                    var processId = CiProcessId.ofFlow(path, id);
                    var activity = random.nextObject(Activity.class);
                    return launch(processId, Projects.get(random), random.nextInt(3), activity);
                })
                .toList();

        db.currentOrTx(() -> db.launches().save(launches));

        task.executeImpl(null);

        verify(solomonClient, atLeastOnce()).push(any(), metrics.capture());

        var expectedResults = TestUtils.parseJson("flow-metrics.json", SolomonMetrics[].class);
        var capturedValues = metrics.getAllValues();
        for (int i = 0; i < capturedValues.size(); i++) {
            var actualMetricsByLabels = capturedValues.get(i).getMetrics().stream().collect(Collectors.toMap(
                    SolomonMetric::getLabels,
                    Function.identity()
            ));
            var expectedMetricsByLabels = expectedResults[i].getMetrics().stream().collect(Collectors.toMap(
                    SolomonMetric::getLabels,
                    Function.identity()
            ));
            assertThat(actualMetricsByLabels)
                    .describedAs("i: %s, comparing actual %s with expected %s", i, capturedValues.get(i),
                            expectedResults[i])
                    .isEqualTo(expectedMetricsByLabels);
        }
    }

    private static String randomString(EnhancedRandom random, Class<? extends Enum<?>> source) {
        return random.nextObject(source).name().toLowerCase();
    }

    enum Projects {
        TESTENV,
        SANDBOX;

        public static String get(EnhancedRandom random) {
            return randomString(random, Projects.class);
        }
    }

    enum Paths {
        GREEN,
        WHITE,
        BLACK;

        public static String get(EnhancedRandom random) {
            return randomString(random, Paths.class);
        }
    }

    enum Ids {
        MONICA,
        NIKOLE;

        public static String get(EnhancedRandom random) {
            return randomString(random, Ids.class);
        }
    }

    private Launch launch(CiProcessId processId, String project, int retries, Activity active) {
        clock.plus(Duration.ofMinutes(1));
        return TestData.launchBuilder()
                .launchId(LaunchId.of(processId, ++launchNumber))
                .project(project)
                .activity(active)
                .activityChanged(clock.instant())
                .statistics(LaunchStatistics.builder().retries(retries).build())
                .build();
    }
}
