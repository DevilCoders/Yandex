package ru.yandex.ci.flow.engine.runtime;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.LaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.independent.IndependentFlow;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;

@Slf4j
public class FlowStateServiceTest extends FlowEngineTestBase {

    @Autowired
    private FlowStateService sut;

    @Autowired
    private FlowProvider flowProvider;

    @Test
    public void saveAndRead() throws AYamlValidationException {

        FlowLaunchId launchId = sut.activateLaunch(
                LaunchParameters.builder()
                        .launchId(FlowTestUtils.LAUNCH_ID)
                        .flowInfo(FlowTestUtils.SIMPLE_FLOW_INFO)
                        .launchInfo(FlowTestUtils.SIMPLE_LAUNCH_INFO)
                        .vcsInfo(FlowTestUtils.VCS_INFO)
                        .projectId("prj")
                        .flow(flowProvider.get(SimpleFlow.FLOW_ID))
                        .triggeredBy(TestData.USER42)
                        .build()
        ).getFlowLaunchId();

        sut.recalc(launchId, new JobRunningEvent(SimpleFlow.JOB_ID, 1, DummyTmsTaskIdFactory.create()));
        sut.recalc(launchId, new JobExecutorSucceededEvent(SimpleFlow.JOB_ID, 1));
        sut.recalc(launchId, new SubscribersSucceededEvent(SimpleFlow.JOB_ID, 1));
        sut.recalc(launchId, new JobSucceededEvent(SimpleFlow.JOB_ID, 1));

        FlowLaunchEntity launch = flowLaunchGet(launchId);

        Assertions.assertEquals(
                5,
                launch.getJobs().get(SimpleFlow.JOB_ID).getFirstLaunch().getStatusHistory().size()
        );
    }

    @Test
    public void concurrentModification() throws AYamlValidationException, ExecutionException, InterruptedException {
        flowTester.register(IndependentFlow.flow(), IndependentFlow.FLOW_ID);

        FlowLaunchId launchId = sut.activateLaunch(
                LaunchParameters.builder()
                        .launchId(FlowTestUtils.LAUNCH_ID)
                        .flowInfo(FlowTestUtils.SIMPLE_FLOW_INFO)
                        .launchInfo(FlowTestUtils.SIMPLE_LAUNCH_INFO)
                        .vcsInfo(FlowTestUtils.VCS_INFO)
                        .projectId("prj")
                        .flow(flowProvider.get(IndependentFlow.FLOW_ID))
                        .triggeredBy(TestData.USER42)
                        .build()
        ).getFlowLaunchId();

        var pool = Executors.newFixedThreadPool(4); // It works very poor due to nonstop YDB lock concurrency
        try {
            var tasks = IntStream.range(0, IndependentFlow.JOB_COUNT)
                    .mapToObj(i -> pool.submit(() -> {
                        String jobId = IndependentFlow.JOB_PREFIX + i;

                        log.info("job running: {}", jobId);
                        sut.recalc(launchId, new JobRunningEvent(jobId, 1, DummyTmsTaskIdFactory.create()));
                        sut.recalc(launchId, new JobExecutorSucceededEvent(jobId, 1));
                        sut.recalc(launchId, new SubscribersSucceededEvent(jobId, 1));
                        log.info("job succeeded: {}", jobId);
                        sut.recalc(launchId, new JobSucceededEvent(jobId, 1));
                        return jobId;
                    }))
                    .collect(Collectors.toList());
            for (var task : tasks) {
                log.info("Complete: {}", task.get());
            }
        } finally {
            pool.shutdown();
        }

        FlowLaunchEntity launch = flowLaunchGet(launchId);

        Assertions.assertEquals(201, launch.getStateVersion());
        Assertions.assertTrue(launch.getJobs().values().stream().allMatch(this::isSuccessful));
    }

    private boolean isSuccessful(JobState state) {
        return state.getLastLaunch()
                .getLastStatusChange()
                .getType()
                .equals(StatusChangeType.SUCCESSFUL);
    }
}
