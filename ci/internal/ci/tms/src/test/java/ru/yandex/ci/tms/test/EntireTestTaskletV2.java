package ru.yandex.ci.tms.test;

import java.util.List;

import WoodflowCi.furniture_factory.FurnitureFactory;
import com.google.gson.JsonParser;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.job.TaskletContext;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.test.woodflow.FurnitureFactoryStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

public class EntireTestTaskletV2 extends AbstractEntireTest {

    @Test
    void simplestTaskletV2() throws YavDelegationException, InterruptedException {
        var taskletSpy = spy(new FurnitureFactoryStub());
        taskletV2MetadataHelper.registerExecutor(TaskletV2Resources.FURNITURE,
                FurnitureFactory.Input.class,
                FurnitureFactory.Output.class,
                taskletSpy::execute
        );

        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_TASKLETV2_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы', " +
                                "произведенного [ИП Иванов, ООО Пилорама, ОАО Липа не липа]"
                ));

        var inputCapture = ArgumentCaptor.forClass(FurnitureFactory.Input.class);
        verify(taskletSpy, atLeastOnce()).execute(inputCapture.capture());

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var input = inputCapture.getValue();
        var expectedContext = TestUtils.parseProtoTextFromString("""
                job_instance_id {
                  job_id: "furniture-factory"
                  number: 1
                }
                target_revision {
                  hash: "r2"
                  number: 2
                  pull_request_id: 92

                }
                secret_uid: "sec-01dy7t26dyht1bj4w3yn94fsa"
                release_vsc_info {
                }
                config_info {
                  path: "release/sawmill/a.yaml"
                  dir: "release/sawmill"
                  id: "simplest-tasklet-v2-release"
                }
                launch_number: 1
                flow_triggered_by: "andreevdm"
                ci_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-tasklet-v2-release&version=1"
                ci_job_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-tasklet-v2-release&version=1&selectedJob=furniture-factory\
                &launchNumber=1"
                version: "1"
                version_info {
                    full: "1"
                    major: "1"
                }
                target_commit {
                  revision {
                    hash: "r2"
                    number: 2
                    pull_request_id: 92

                  }
                  date {
                    seconds: 1594676509
                    nanos: 42000000
                  }
                  message: "Message"
                  author: "sid-hugo"
                }
                branch: "trunk"
                flow_type: DEFAULT
                """, TaskletContext.class);

        expectedContext = updateContext(expectedContext, finalLaunch, flowLaunch);

        assertThat(input.getContext()).isEqualTo(expectedContext);
    }

    @Test
    void normalExecution() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_TASKLETV2_SIMPLE_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);
        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));
        var jobLaunch = flowLaunch.getJobState("simple").getLastLaunch();

        assertThat(jobLaunch).isNotNull();
        assertThat(jobLaunch.getLastStatusChangeType())
                .isEqualTo(StatusChangeType.SUCCESSFUL);
        assertThat(jobLaunch.getExecutionExceptionStacktrace()).isNull();
    }

    @Test
    void userErrorExecution() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_TASKLETV2_SIMPLE_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        var flowVars = JsonParser.parseString("""
                {
                    "input": "%s"
                }
                """.formatted(SimpleAction.SIMULATE_USER_ERROR)).getAsJsonObject();
        Launch launch = launch(processId, revision, flowVars);
        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.FAILURE);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));
        var jobLaunch = flowLaunch.getJobState("simple").getLastLaunch();

        assertThat(jobLaunch).isNotNull();
        assertThat(jobLaunch.getLastStatusChangeType())
                .isEqualTo(StatusChangeType.FAILED);
        assertThat(jobLaunch.getExecutionExceptionStacktrace())
                .contains("Tasklet v2 failed")
                .contains("User error");
    }

    @Test
    void serverErrorExecution() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_TASKLETV2_SIMPLE_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        var flowVars = JsonParser.parseString("""
                {
                    "input": "%s"
                }
                """.formatted(SimpleAction.SIMULATE_SERVER_ERROR)).getAsJsonObject();
        Launch launch = launch(processId, revision, flowVars);
        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.FAILURE);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));
        var jobLaunch = flowLaunch.getJobState("simple").getLastLaunch();

        assertThat(jobLaunch).isNotNull();
        assertThat(jobLaunch.getLastStatusChangeType())
                .isEqualTo(StatusChangeType.FAILED);

        assertThat(jobLaunch.getExecutionExceptionStacktrace())
                .contains("Tasklet v2 failed")
                .contains("Runtime exception");
    }

    @Test
    void invalidExecution() throws YavDelegationException, InterruptedException {

        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_TASKLETV2_SIMPLE_INVALID_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);
        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.FAILURE);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));
        var jobLaunch = flowLaunch.getJobState("simple").getLastLaunch();

        assertThat(jobLaunch).isNotNull();
        assertThat(jobLaunch.getLastStatusChangeType())
                .isEqualTo(StatusChangeType.FAILED);
        assertThat(jobLaunch.getExecutionExceptionStacktrace())
                .contains("Tasklet v2 failed")
                .contains("Tasklet internal error");
    }

}
