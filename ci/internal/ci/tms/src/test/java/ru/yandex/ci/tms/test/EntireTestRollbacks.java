package ru.yandex.ci.tms.test;


import java.util.List;

import WoodflowCi.furniture_factory.FurnitureFactory;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchState.Status;
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

public class EntireTestRollbacks extends AbstractEntireTest {

    @Test
    void simplestWithCustomRollbackFlowNoJobData() throws Exception {
        var taskletSpy = spy(new FurnitureFactoryStub());
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, taskletSpy);

        var firstLaunch = firstSimplestRelease();
        assertThat(firstLaunch.getTitle()).isEqualTo("Simplest release process #1");

        var firstLaunchId = firstLaunch.getLaunchId();
        Mockito.reset(taskletSpy);

        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId);

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("2"));
        assertThat(finalLaunch.getTitle()).isEqualTo("Rollback: Simplest release process #2 (#1)");

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(rollbackLaunch.getLaunchId()),
                        "furniture-factory-rollback");

        var furnitures = getFurniture(resources);
        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы, rollback', " +
                                "произведенного [ОАО Липа не липа rollback 2, ООО Пилорама rollback 1, " +
                                "ИП Иванов ROLLBACK 3]"
                ));

        var inputCapture = ArgumentCaptor.forClass(FurnitureFactory.Input.class);
        verify(taskletSpy, atLeastOnce()).execute(inputCapture.capture());

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(rollbackLaunch.getLaunchId()));
        var input = inputCapture.getValue();
        var expectedContext = TestUtils.parseProtoTextFromString("""
                job_instance_id {
                  job_id: "furniture-factory-rollback"
                  number: 1
                }
                previous_revision {
                  hash: "r2"
                  number: 2
                  pull_request_id: 92

                }
                target_revision {
                  hash: "r2"
                  number: 2
                  pull_request_id: 92

                }
                secret_uid: "sec-01dy7t26dyht1bj4w3yn94fsa"
                release_vsc_info {
                    previous_release_revision {
                        hash: "r2"
                        number: 2
                        pull_request_id: 92

                      }
                }
                config_info {
                  path: "release/sawmill/a.yaml"
                  dir: "release/sawmill"
                  id: "simplest-release"
                }
                launch_number: 2
                flow_triggered_by: "andreevdm"
                ci_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-release&version=2"
                ci_job_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-release&version=2&selectedJob=furniture-factory-rollback&\
                launchNumber=1"
                version: "2"
                version_info {
                    full: "2"
                    major: "2"
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
                flow_type: ROLLBACK
                rollback_to_version {
                  full: "1"
                  major: "1"
                }
                """, TaskletContext.class);

        expectedContext = updateContext(expectedContext, finalLaunch, flowLaunch);

        assertThat(input.getContext()).isEqualTo(expectedContext);
    }

    @Test
    void simplestReleaseWithRollbackOnSameCommitNoJobData() throws YavDelegationException, InterruptedException {
        var firstLaunchId = firstSimplestRelease().getLaunchId();

        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;

        //

        Launch rollbackLaunch1 = launch(
                processId,
                TestData.TRUNK_R2,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId);

        engineTester.waitLaunch(rollbackLaunch1.getLaunchId(), WAIT, Status.SUCCESS);

        // Еще один, точно такой же rollback

        Launch rollbackLaunch2 = launch(
                processId,
                TestData.TRUNK_R2,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId);

        engineTester.waitLaunch(rollbackLaunch2.getLaunchId(), WAIT, Status.SUCCESS);
    }

    @Test
    void rollbackInBranch() throws YavDelegationException, InterruptedException {
        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        processCommits(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5,
                TestData.TRUNK_COMMIT_6);

        engineTester.delegateToken(processId.getPath());
        Launch launch = launch(processId, TestData.TRUNK_R6);

        var firstLaunchId = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(firstLaunchId.getVersion()).isEqualTo(Version.major("1"));

        db.tx(() -> branchService.createBranch(processId, TestData.TRUNK_R6, TestData.CI_USER));

        //

        Launch rollbackLaunch1 = launch(
                processId,
                TestData.TRUNK_R6,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId.getLaunchId());

        engineTester.waitLaunch(rollbackLaunch1.getLaunchId(), WAIT, Status.SUCCESS);

    }

    @Test
    void simplestReleaseWithRollbackOnPrevious() throws YavDelegationException, InterruptedException {
        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        processCommits(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5,
                TestData.TRUNK_COMMIT_6);

        engineTester.delegateToken(processId.getPath());
        Launch launch = launch(processId, TestData.TRUNK_R6);

        var firstLaunchId = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(firstLaunchId.getVersion()).isEqualTo(Version.major("1"));


        //

        Launch rollbackLaunch1 = launch(
                processId,
                TestData.TRUNK_R2,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId.getLaunchId());

        engineTester.waitLaunch(rollbackLaunch1.getLaunchId(), WAIT, Status.SUCCESS);

        // Еще один, точно такой же rollback

        Launch rollbackLaunch2 = launch(
                processId,
                TestData.TRUNK_R2,
                "simplest-rollback-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId.getLaunchId());

        engineTester.waitLaunch(rollbackLaunch2.getLaunchId(), WAIT, Status.SUCCESS);
    }

    @Test
    void simpleReleaseWithAutoRollbackNoJobData() throws YavDelegationException, InterruptedException {
        var firstLaunch = firstSimplestRelease();
        assertThat(firstLaunch.getTitle()).isEqualTo("Simplest release process #1");

        var firstLaunchId = firstLaunch.getLaunchId();

        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "rollback_simplest-release_simplest-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId);

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("2"));
        assertThat(finalLaunch.getTitle()).isEqualTo("Rollback: Simplest release process #2 (#1)");

        var executedJob = engineTester.waitJob(rollbackLaunch.getLaunchId(), WAIT, "woodcutter",
                StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(executedJob.isConditionalSkip()).isFalse();
        assertThat(executedJob.getTitle()).isEqualTo("Дровосек");

        var skippedJob = engineTester.waitJob(rollbackLaunch.getLaunchId(), WAIT, "furniture-factory",
                StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(skippedJob.isConditionalSkip()).isTrue();
        assertThat(skippedJob.getTitle()).isEqualTo("Skip by Registry: Фабрика");

        StoredResourceContainer woodcutterResources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(rollbackLaunch.getLaunchId()),
                        "woodcutter");
        assertThat(woodcutterResources.getResources()).isNotEmpty();


        StoredResourceContainer furnitureResources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(rollbackLaunch.getLaunchId()),
                        "furniture-factory");
        assertThat(furnitureResources.getResources()).isEmpty();
    }

    @Test
    void simpleReleaseWithAutoRollbackAndManual() throws YavDelegationException, InterruptedException {
        var processId = TestData.SIMPLEST_RELEASE_WITH_MANUAL_PROCESS_ID;
        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(processId.getPath());
        Launch launch = launch(processId, TestData.TRUNK_R2);

        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);
        disableManualSwitch(waitLaunch, "woodcutter");

        var firstLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(firstLaunch.getVersion()).isEqualTo(Version.major("1"));
        assertThat(firstLaunch.getTitle()).isEqualTo("simplest-release-with-manual #1");

        var firstLaunchId = firstLaunch.getLaunchId();

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "rollback_simplest-release-with-manual_simplest-flow-with-manual",
                Common.FlowType.FT_ROLLBACK,
                firstLaunchId);

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("2"));
        assertThat(finalLaunch.getTitle()).isEqualTo("Rollback: simplest-release-with-manual #2 (#1)");
    }

    @Test
    void woodcutterWithManualRollback() throws YavDelegationException, InterruptedException {
        var firstLaunch = firstWoodcutterRelease();
        assertThat(firstLaunch.getTitle()).isEqualTo("Demo samwill release #1");

        var processId = TestData.SAWMILL_RELEASE_PROCESS_ID;

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "release-sawmill-rollback",
                Common.FlowType.FT_ROLLBACK,
                firstLaunch.getLaunchId());

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getTitle()).isEqualTo("Rollback: Demo samwill release #2 (#1)");

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(finalLaunch.getLaunchId()), "furniture-factory");

        // Почему у нас title отличается?
        // Потому что в furniture-factory попали ресурсы, собранные в firstWoodcutterRelease
        var title = firstLaunch.getTitle();

        var user = TestData.CI_USER;
        var linden = "бревно из дерева Липа, которую заказал %s".formatted(user);
        var birch = "бревно из дерева Береза, которую срубили по просьбе %s на flow %s".formatted(user, title);

        var sawmill1 = "Лесопилка обычная, пилит 2 бревна";
        var sawmill2 = "Лесопилка \"%s\" производительная, начинает работу над \"%s\"".formatted(user, birch);

        var furniture = "Шкаф из 3 досок, полученных из материала";
        assertThat(getFurniture(resources)).isEqualTo(
                List.of(
                        "%s '%s', произведенного [%s, %s]".formatted(furniture, birch, sawmill2, sawmill1),
                        "%s '%s', произведенного [%s]".formatted(furniture, birch, sawmill2),
                        "%s '%s', произведенного [%s, %s]".formatted(furniture, linden, sawmill2, sawmill1),
                        "%s '%s', произведенного [%s]".formatted(furniture, linden, sawmill2)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEqualTo(
                List.of("%s[%s]".formatted(sawmill1, birch),
                        "%s[%s]".formatted(sawmill1, linden)
                ));
    }

    @Test
    void woodcutterWithManualRollbackButSameJobId() throws YavDelegationException, InterruptedException {
        var firstLaunch = firstWoodcutterRelease();
        assertThat(firstLaunch.getTitle()).isEqualTo("Demo samwill release #1");

        var processId = TestData.SAWMILL_RELEASE_PROCESS_ID;

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "release-sawmill-rollback-wrong-stage",
                Common.FlowType.FT_ROLLBACK,
                firstLaunch.getLaunchId());

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);
        assertThat(finalLaunch.getTitle()).isEqualTo("Rollback: Demo samwill release #2 (#1)");

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));

        // No delegated resources must be saved from original flow
        // Job is not skipped
        assertThat(flowLaunch.getJobState("woodcutter").getDelegatedOutputResources()).isNull();
    }

    @Test
    void woodcutterWithAutoRollback() throws YavDelegationException, InterruptedException {
        var firstLaunch = firstWoodcutterRelease();

        var processId = TestData.SAWMILL_RELEASE_PROCESS_ID;

        Launch rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "rollback_demo-sawmill-release_release-sawmill",
                Common.FlowType.FT_ROLLBACK,
                firstLaunch.getLaunchId());

        Launch finalLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunchId = FlowLaunchId.of(finalLaunch.getLaunchId());
        var resources = flowTestQueries.getProducedResources(flowLaunchId, "furniture-factory");

        var title = firstLaunch.getTitle();

        var user = TestData.CI_USER;
        var linden = "бревно из дерева Липа, которую заказал %s".formatted(user);
        var birch = "бревно из дерева Береза, которую срубили по просьбе %s на flow %s".formatted(user, title);

        var sawmill1 = "Лесопилка обычная, пилит 2 бревна";
        var sawmill2 = "Лесопилка \"%s\" производительная, начинает работу над \"%s\"".formatted(user, birch);

        var furniture = "Шкаф из 3 досок, полученных из материала";
        assertThat(getFurniture(resources)).isEqualTo(
                List.of(
                        "%s '%s', произведенного [%s, %s]".formatted(furniture, birch, sawmill2, sawmill1),
                        "%s '%s', произведенного [%s]".formatted(furniture, birch, sawmill2),
                        "%s '%s', произведенного [%s, %s]".formatted(furniture, linden, sawmill2, sawmill1),
                        "%s '%s', произведенного [%s]".formatted(furniture, linden, sawmill2)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEqualTo(
                List.of("%s[%s]".formatted(sawmill1, birch),
                        "%s[%s]".formatted(sawmill1, linden)
                ));


        var postProcessingResources = flowTestQueries.getProducedResources(flowLaunchId, "sawmill-post-process");
        var postBoards = getBoards(postProcessingResources);
        assertThat(postBoards).isEqualTo(
                List.of("%s[%s]".formatted(sawmill1, birch),
                        "%s[%s]".formatted(sawmill1, linden)
                ));
    }

    @Test
    void multiplyByWithRollback() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) ->
                // Artifacts are loaded from current task (not from all upstreams)
                assertThat(task.getCustomFields()).isEqualTo(
                        List.of(
                                new SandboxCustomField("boards_1_count", 3L),
                                new SandboxCustomField("boards_2_count", 3L)
                        ))
        );

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_MULTIPLY_VIRTUAL_RELEASE_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        var firstLaunch = launch(processId, TestData.TRUNK_R2);
        firstLaunch = engineTester.waitLaunch(firstLaunch.getLaunchId(), WAIT, Status.SUCCESS);


        var rollbackLaunch = launch(
                processId,
                TestData.TRUNK_R2,
                "rollback_simplest-multiply-virtual-release_simplest-multiply-virtual-flow",
                Common.FlowType.FT_ROLLBACK,
                firstLaunch.getLaunchId());

        rollbackLaunch = engineTester.waitLaunch(rollbackLaunch.getLaunchId(), WAIT, Status.SUCCESS);


        // Make sure we copied all resources
        StoredResourceContainer resources = flowTestQueries.getProducedResources(
                FlowLaunchId.of(rollbackLaunch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        // See simplestMultiplyVirtual#simplestMultiplyVirtual
        assertThat(furnitures).isEqualTo(
                List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'новое бревно из дерева Береза', " +
                                "произведенного [Пилит новое бревно из дерева Береза by %s]", user),
                        String.format("Шкаф из 3 досок, полученных из материала 'новое бревно из дерева Липа', " +
                                "произведенного [Пилит новое бревно из дерева Липа by %s]", user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

    }


    //

    private Launch firstSimplestRelease() throws YavDelegationException, InterruptedException {
        return launchAndWait(TestData.SIMPLEST_RELEASE_PROCESS_ID);
    }

    private Launch firstWoodcutterRelease() throws YavDelegationException, InterruptedException {
        return launchAndWait(TestData.SAWMILL_RELEASE_PROCESS_ID);
    }

    private Launch launchAndWait(CiProcessId processId) throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(processId.getPath());
        var launch = launch(processId, TestData.TRUNK_R2);

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(launch.getVersion()).isEqualTo(Version.major("1"));

        return launch;
    }


}
