package ru.yandex.ci.common.temporal.heartbeat;

import java.time.Duration;
import java.util.List;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.logging.LoggingMeterRegistry;
import io.temporal.activity.Activity;
import org.assertj.core.api.Assertions;
import org.awaitility.Awaitility;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.TemporalTestBase;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleActivity;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow;


class TemporalWorkerHeartbeatServiceTest extends TemporalTestBase {

    private static final String HOST = "test-host";


    @Nullable
    private static volatile String heartbeat;

    @BeforeEach
    @Override
    public void setUp() {
        super.setUp();
        heartbeat = null;
    }

    @Override
    protected TemporalWorkerHeartbeatService heartbeatService() {
        return new TemporalWorkerHeartbeatService(Duration.ofMillis(100), 5, new LoggingMeterRegistry(), HOST);
    }

    @Override
    protected List<Class<?>> workflowImplementationTypes() {
        return List.of(SimpleTestWorkflowImpl.class);
    }

    @Override
    protected List<Object> activityImplementations() {
        SimpleActivityImpl simpleActivity = new SimpleActivityImpl();
        return List.of(simpleActivity);
    }


    @Test
    void heartbeat() {
        temporalService.startDeduplicated(
                SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.of("21")
        );
        Awaitility.await().atMost(Duration.ofMinutes(1))
                .until(() -> heartbeat != null);

        Assertions.assertThat(heartbeat).isEqualTo(HOST);
    }


    public static class SimpleTestWorkflowImpl implements SimpleTestWorkflow {
        SimpleActivity activity = TemporalService.createActivity(
                SimpleActivity.class,
                Duration.ofMinutes(2),
                Duration.ofSeconds(3)
        );

        @Override
        public void run(SimpleTestId input) {
            activity.runActivity();
        }
    }

    private static class SimpleActivityImpl implements SimpleActivity {
        @Override
        public void runActivity() {
            Awaitility.await()
                    .pollInSameThread()
                    .atMost(Duration.ofMinutes(1))
                    .until(() -> Activity.getExecutionContext().getHeartbeatDetails(String.class).isPresent());
            heartbeat = Activity.getExecutionContext().getHeartbeatDetails(String.class).orElseThrow();
        }
    }


}
