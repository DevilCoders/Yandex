package ru.yandex.ci.common.temporal;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import io.micrometer.core.instrument.MeterRegistry;
import io.temporal.activity.ActivityExecutionContext;
import io.temporal.api.common.v1.WorkflowExecution;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptor;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptorBase;
import io.temporal.common.interceptors.WorkerInterceptor;
import io.temporal.common.interceptors.WorkflowInboundCallsInterceptor;
import io.temporal.common.interceptors.WorkflowInboundCallsInterceptorBase;
import io.temporal.common.interceptors.WorkflowOutboundCallsInterceptor;
import io.temporal.workflow.Workflow;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.config.TemporalWorkerBuilder;
import ru.yandex.ci.common.temporal.config.TemporalWorkerFactoryBuilder;
import ru.yandex.ci.common.temporal.config.TemporalWorkerFactoryWrapper;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.MetricTemporalMonitoringService;
import ru.yandex.ci.common.temporal.spring.TemporalTestConfig;
import ru.yandex.ci.common.temporal.spring.TemporalYdbTestConfig;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;

import static org.awaitility.Awaitility.await;

@Timeout(value = 5, unit = TimeUnit.MINUTES)
@ExtendWith(SpringExtension.class)
@ActiveProfiles(profiles = CiProfile.UNIT_TEST_PROFILE)
@ContextConfiguration(
        classes = {
                TemporalYdbTestConfig.class,
                TemporalTestConfig.class
        }
)
public abstract class TemporalTestBase {

    @Nullable
    private TemporalWorkerHeartbeatService heartbeatService;

    @Autowired
    protected TemporalService temporalService;

    @Autowired
    protected TemporalDb temporalDb;

    @Autowired
    protected MetricTemporalMonitoringService monitoringService;

    @Autowired
    protected MeterRegistry meterRegistry;

    @Nullable
    private TemporalWorkerFactoryWrapper workerFactory;

    private final List<String> executedWorkflowId = Collections.synchronizedList(new ArrayList<>());
    private final List<String> executedRunId = Collections.synchronizedList(new ArrayList<>());
    private final Map<String, Integer> workflowAttemptNumbers = new ConcurrentHashMap<>();
    private final Map<String, Integer> workflowActivityAttemptNumbers = new ConcurrentHashMap<>();

    @BeforeEach
    public void setUp() {

        var workerFactoryBuilder = TemporalConfigurationUtil.createWorkerFactoryBuilder(temporalService)
                .monitoringService(monitoringService)
                .customInterceptor(new TestInterceptor());

        heartbeatService = heartbeatService();
        if (heartbeatService != null) {
            workerFactoryBuilder.heartbeatService(heartbeatService);
            heartbeatService.startAsync();
        }

        createWorkerIfNecessary(
                workerFactoryBuilder, TemporalConfigurationUtil.createDefaultWorker(),
                workflowImplementationTypes(), activityImplementations()
        );

        createWorkerIfNecessary(
                workerFactoryBuilder, TemporalConfigurationUtil.createCronWorker(),
                cronWorkflowImplementationTypes(), cronActivityImplementations()
        );

        workerFactory = workerFactoryBuilder.build();
        workerFactory.start();
    }

    private void createWorkerIfNecessary(TemporalWorkerFactoryBuilder workerFactoryBuilder,
                                         TemporalWorkerBuilder workerBuilder,
                                         List<Class<?>> workflowImplementationTypes,
                                         List<Object> activityImplementations) {
        if (workflowImplementationTypes.isEmpty() && activityImplementations.isEmpty()) {
            return;
        }
        workerBuilder.workflowImplementationTypes(workflowImplementationTypes);
        workerBuilder.activitiesImplementations(activityImplementations);

        workerFactoryBuilder.worker(workerBuilder);
    }

    @AfterEach
    public void tearDown() {
        Preconditions.checkState(workerFactory != null);
        workerFactory.shutdown();
        if (heartbeatService != null) {
            heartbeatService.stopAsync();
            heartbeatService = null;
        }
        executedWorkflowId.clear();
        executedRunId.clear();
        workflowAttemptNumbers.clear();
        monitoringService.flush();
        meterRegistry.clear();
        clearDb();
    }

    private void clearDb() {
        temporalDb.tx(() -> {
            temporalDb.temporalFailingWorkflow().deleteAll();
            temporalDb.temporalLaunchQueue().deleteAll();
        });
    }

    protected void awaitCompletion(WorkflowExecution execution, Duration timeout) {
        await().atMost(timeout).until(
                () -> executedWorkflowId.contains(execution.getWorkflowId())
                        && executedRunId.contains(execution.getRunId())
        );
    }

    protected void awaitWorkflowAttempt(WorkflowExecution execution, int attemptNumber, Duration timeout) {
        await().atMost(timeout).until(
                () -> workflowAttemptNumbers.getOrDefault(execution.getWorkflowId(), 0) >= attemptNumber
        );
    }

    protected void awaitWorkflowActivityAttempt(WorkflowExecution execution, int attemptNumber, Duration timeout) {
        await().atMost(timeout).until(
                () -> workflowActivityAttemptNumbers.getOrDefault(execution.getWorkflowId(), 0) >= attemptNumber
        );
    }

    @Nullable
    protected TemporalWorkerHeartbeatService heartbeatService() {
        return null;
    }

    protected abstract List<Class<?>> workflowImplementationTypes();

    protected abstract List<Object> activityImplementations();


    protected List<Class<?>> cronWorkflowImplementationTypes() {
        return List.of();
    }

    protected List<Object> cronActivityImplementations() {
        return List.of();
    }

    public class TestInterceptor implements WorkerInterceptor {
        @Override
        public WorkflowInboundCallsInterceptor interceptWorkflow(WorkflowInboundCallsInterceptor next) {
            return new TestWorkflowInterceptor(next);
        }

        @Override
        public ActivityInboundCallsInterceptor interceptActivity(ActivityInboundCallsInterceptor next) {
            return new TestActivityInterceptor(next);
        }
    }

    public class TestWorkflowInterceptor extends WorkflowInboundCallsInterceptorBase {

        public TestWorkflowInterceptor(WorkflowInboundCallsInterceptor next) {
            super(next);
        }


        @Override
        public void init(WorkflowOutboundCallsInterceptor outboundCalls) {
            var info = Workflow.getInfo();
            workflowAttemptNumbers.put(info.getWorkflowId(), info.getAttempt());
            super.init(outboundCalls);
        }

        @Override
        public WorkflowOutput execute(WorkflowInput input) {
            var info = Workflow.getInfo();
            try {
                return super.execute(input);
            } finally {
                executedWorkflowId.add(info.getWorkflowId());
                executedRunId.add(info.getRunId());
            }
        }
    }

    public class TestActivityInterceptor extends ActivityInboundCallsInterceptorBase {
        public TestActivityInterceptor(ActivityInboundCallsInterceptor next) {
            super(next);
        }

        @Override
        public void init(ActivityExecutionContext context) {
            var info = context.getInfo();
            workflowActivityAttemptNumbers.put(info.getWorkflowId(), info.getAttempt());
            super.init(context);
        }
    }

}
