package ru.yandex.ci.common.temporal.monitoring;

import java.time.Duration;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.test.annotation.DirtiesContext;

import ru.yandex.ci.common.temporal.TemporalTestBase;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestLongId;

import static org.assertj.core.api.Assertions.assertThat;

public class MetricTemporalMonitoringServiceTest extends TemporalTestBase {
    @Override
    protected List<Class<?>> workflowImplementationTypes() {
        return List.of(
                DeserializationExecptionWorkflow.Impl.class,
                RuntimeExceptionWorkflow.Impl.class,
                WorkflowWithFailingActivity.Impl.class
        );
    }

    @Override
    protected List<Object> activityImplementations() {
        return List.of(
                new FailingActivity.Impl()
        );
    }

    @Test
    void deserializationExceptionCreatesNewAttempt() {
        var execution = temporalService.startDeduplicated(
                DeserializationExecptionWorkflow.class,
                wf -> wf::run,
                new DeserializationExecptionWorkflow.BadId("id")
        );
        awaitWorkflowAttempt(execution, 3, Duration.ofMinutes(1));
    }

    @Test
    void exceptionExceptionCreatesNewAttempt() {
        var execution = temporalService.startDeduplicated(
                RuntimeExceptionWorkflow.class,
                wf -> wf::run,
                SimpleTestId.of("exceptionExceptionCreatesNewAttempt")
        );
        awaitWorkflowAttempt(execution, 3, Duration.ofMinutes(1));
    }

    @Test
    @DirtiesContext(methodMode = DirtiesContext.MethodMode.AFTER_METHOD)
    void failingWorkflowMetricsForActivities() {
        var execution1 = temporalService.startDeduplicated(
                WorkflowWithFailingActivity.class,
                wf -> wf::run,
                SimpleTestLongId.of(40)
        );

        var execution2 = temporalService.startDeduplicated(
                WorkflowWithFailingActivity.class,
                wf -> wf::run,
                SimpleTestLongId.of(41)
        );

        awaitWorkflowActivityAttempt(execution1, 10, Duration.ofMinutes(2));
        awaitWorkflowActivityAttempt(execution2, 10, Duration.ofMinutes(2));
        monitoringService.updateMetrics();

        assertThat(retryExceededMetricValue("warn", "WorkflowWithFailingActivity")).isEqualTo(2);
        assertThat(retryExceededMetricValue("warn", "TOTAL")).isEqualTo(2);
        assertThat(retryExceededMetricValue("crit", "TOTAL")).isEqualTo(0);
    }

    @Test
    void metricsAreGoodAfterActivityPassed() {
        var execution = temporalService.startDeduplicated(
                WorkflowWithFailingActivity.class,
                wf -> wf::run,
                SimpleTestLongId.of(12)
        );
        awaitWorkflowActivityAttempt(execution, 10, Duration.ofMinutes(2));
        monitoringService.updateMetrics();
        assertThat(retryExceededMetricValue("warn", "WorkflowWithFailingActivity")).isEqualTo(1);
        assertThat(retryExceededMetricValue("warn", "TOTAL")).isEqualTo(1);
        assertThat(retryExceededMetricValue("crit", "TOTAL")).isEqualTo(0);

        awaitCompletion(execution, Duration.ofMinutes(2));
        monitoringService.updateMetrics();
        assertThat(retryExceededMetricValue("warn", "WorkflowWithFailingActivity")).isNaN();
        assertThat(retryExceededMetricValue("warn", "TOTAL")).isEqualTo(0);
        assertThat(retryExceededMetricValue("crit", "TOTAL")).isEqualTo(0);
    }

    @Test
    void totalMetricsAlwaysExists() {
        monitoringService.updateMetrics();
        assertThat(retryExceededMetricValue("warn", "TOTAL")).isEqualTo(0);
        assertThat(retryExceededMetricValue("crit", "TOTAL")).isEqualTo(0);
    }

    private double retryExceededMetricValue(String level, String workflowType) {
        var gauges = meterRegistry.get(MetricTemporalMonitoringService.RETRY_EXCEEDED_METRIC_NAME)
                .tag("level", level)
                .tag("type", workflowType)
                .gauges();

        assertThat(gauges).hasSize(1);
        var gauge = gauges.stream().findFirst().orElseThrow();
        return gauge.value();
    }
}
