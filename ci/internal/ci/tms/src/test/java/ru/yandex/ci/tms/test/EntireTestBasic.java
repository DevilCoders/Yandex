package ru.yandex.ci.tms.test;

import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.stream.Collectors;

import WoodflowCi.furniture_factory.FurnitureFactory;
import WoodflowCi.woodcutter.Woodcutter;
import ci.tasklet.registry.demo.picklock.Schema;
import com.google.gson.JsonParser;
import com.google.protobuf.InvalidProtocolBufferException;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.EnumSource;
import org.mockito.ArgumentCaptor;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority.PriorityClass;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority.PrioritySubclass;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.TaskSemaphoreAcquire;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcServiceStub.ChangeInfo;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.event.EventType;
import ru.yandex.ci.engine.event.LaunchEvent;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.OnCommitLaunchService.StartFlowParameters;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.job.TaskletContext;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub.ManualStatusTaskExecutor;
import ru.yandex.ci.tms.test.internal.InternalJob;
import ru.yandex.ci.tms.test.woodflow.FurnitureFactoryStub;
import ru.yandex.ci.tms.test.woodflow.SawmillStub;
import ru.yandex.ci.tms.test.woodflow.WoodcutterStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.assertj.core.api.Assertions.fail;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static ru.yandex.ci.flow.engine.runtime.JobResourcesValidatorTest.resources;

public class EntireTestBasic extends AbstractEntireTest {

    @Autowired
    private InternalJob.Parameters internalJobParameters;

    @Test
    void calculateDownstreams() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.SAWMILL_RELEASE_PROCESS_ID;

        engineTester.delegateToken(releaseProcessId.getPath());
        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        FrontendFlowApi.LaunchState launchState = ProtoMappers.toProtoLaunchState(flowLaunch, launch,
                Common.StagesState.getDefaultInstance());

        Map<String, FrontendFlowApi.JobState> map = launchState.getJobsList().stream()
                .collect(Collectors.toMap(FrontendFlowApi.JobState::getId, Function.identity()));

        Function<FrontendFlowApi.JobState, List<String>> getDownstreams = jobState ->
                jobState.getDownstreamsList().stream()
                        .map(FrontendFlowApi.JobDownstream::getJobId)
                        .toList();

        assertThat(getDownstreams.apply(map.get("woodcutter")))
                .hasSameElementsAs(List.of("sawmill-1", "sawmill-2"));
        assertThat(getDownstreams.apply(map.get("sawmill-1")))
                .hasSameElementsAs(List.of("start-furniture"));
        assertThat(getDownstreams.apply(map.get("sawmill-2")))
                .hasSameElementsAs(List.of("start-furniture"));
        assertThat(getDownstreams.apply(map.get("start-furniture")))
                .hasSameElementsAs(List.of("furniture-factory"));
        assertThat(getDownstreams.apply(map.get("furniture-factory")))
                .hasSameElementsAs(List.of("sawmill-post-process"));
        assertThat(getDownstreams.apply(map.get("sawmill-post-process")))
                .isEmpty();
    }

    @Test
    void woodcutter() throws YavDelegationException, InterruptedException, InvalidProtocolBufferException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.SAWMILL_RELEASE_PROCESS_ID;

        engineTester.delegateToken(releaseProcessId.getPath());
        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var jobWoodcutter = flowLaunch.getJobState("woodcutter");
        assertThat(jobWoodcutter.getTitle()).isEqualTo("Дровосек");
        assertThat(jobWoodcutter.getDescription()).isEqualTo("Рубит деревья на бревна");

        var jobSawmill1 = flowLaunch.getJobState("sawmill-1");
        assertThat(jobSawmill1.getTitle()).isEqualTo("Лесопилка производительная");

        var jobSawmillLaunch = jobSawmill1.getLastLaunch();
        assertThat(jobSawmillLaunch).isNotNull();
        assertThat(jobSawmillLaunch.getTaskStates())
                .hasSize(1);

        var expectUrl = jobSawmillLaunch.getTaskStates().get(0).getUrl();
        assertThat(jobSawmillLaunch.getTaskStates())
                .isEqualTo(List.of(
                        TaskBadge.of("ci:sandbox", "SANDBOX", expectUrl,
                                TaskBadge.TaskStatus.SUCCESSFUL, null, null, true)
                ));


        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        // 2 timbers
        // 2x4 - sawmill-1 = 8
        // 2x3 - sawmill-2 = 6

        // Результат отсортирован, получится 2 набора
        // 3 из sawmill-1, 1 из sawmill-1 + 2 из sawmill-2

        var title = launch.getTitle();

        var user = TestData.CI_USER;
        var linden = "бревно из дерева Липа, которую заказал %s".formatted(user);
        var birch = "бревно из дерева Береза, которую срубили по просьбе %s на flow %s".formatted(user, title);

        var sawmill1 = "Лесопилка обычная, пилит 2 бревна";
        var sawmill2 = "Лесопилка \"%s\" производительная, начинает работу над \"%s\"".formatted(user, birch);

        var furniture = "Шкаф из 3 досок, полученных из материала";
        assertThat(furnitures).isEqualTo(
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

        expectEvents(launch, base -> List.of(
                base.toBuilder().setEventType(EventType.ET_CREATED).build(),
                base.toBuilder().setEventType(EventType.ET_RUNNING)
                        .setFlowLaunchId(flowLaunch.getIdString()).build(),
                base.toBuilder().setEventType(EventType.ET_FINISHED)
                        .setFlowLaunchId(flowLaunch.getIdString()).setSuccess(true).build()
        ));
    }


    @Test
    void skipDuplicateEvents() throws InvalidProtocolBufferException, InterruptedException, ExecutionException {
        var launchBuilder = TestData.launchBuilder().launchId(LaunchId.of(TestData.SIMPLEST_RELEASE_PROCESS_ID, 0));
        Function<Status, byte[]> makeEvent = (status) -> {
            var launch = launchBuilder.status(status).build();
            return new LaunchEvent(launch, clock).getData();
        };
        testQueue.write(makeEvent.apply(Status.STARTING)).get();
        testQueue.write(makeEvent.apply(Status.RUNNING)).get();
        testQueue.write(makeEvent.apply(Status.RUNNING)).get();
        testQueue.write(makeEvent.apply(Status.RUNNING)).get();
        testQueue.write(makeEvent.apply(Status.SUCCESS)).get();

        expectEvents(launchBuilder.build(), base -> List.of(
                base.toBuilder().setEventType(EventType.ET_CREATED).build(),
                base.toBuilder().setEventType(EventType.ET_RUNNING).build(),
                base.toBuilder().setEventType(EventType.ET_FINISHED).setSuccess(true).build()
        ));
    }

    @Test
    void simplest() throws YavDelegationException, InterruptedException {
        var taskletSpy = spy(new FurnitureFactoryStub());
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, taskletSpy);

        processCommits(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
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
        verify(taskletSpy).execute(inputCapture.capture());

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
                  id: "simplest-release"
                }
                launch_number: 1
                flow_triggered_by: "andreevdm"
                ci_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-release&version=1"
                ci_job_url: "https://arcanum-test-url/projects/ci/ci/releases/flow\
                ?dir=release%2Fsawmill&id=simplest-release&version=1&selectedJob=furniture-factory&launchNumber=1"
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
    void retry() throws YavDelegationException, InterruptedException {
        var tasklet = new FurnitureFactoryStub() {
            boolean executedOnce = false;

            @Override
            public FurnitureFactory.Output execute(FurnitureFactory.Input input) {
                if (!executedOnce) {
                    executedOnce = true;
                    throw new RuntimeException("expected test failure");
                }
                return super.execute(input);
            }
        };
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, tasklet);

        upsertCommit(TestData.TRUNK_COMMIT_2);

        var revision = TestData.TRUNK_R2;
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-with-retry");
        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), revision.toRevision(), false);

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flow = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));

        System.out.println(flow);
        var launches = flow.getJobs().get("furniture-factory").getLaunches();
        assertThat(launches).hasSize(2);
        assertThat(launches.get(0).getLastStatusChange().getType()).isEqualTo(StatusChangeType.FAILED);
        assertThat(launches.get(1).getLastStatusChange().getType()).isEqualTo(StatusChangeType.SUCCESSFUL);
    }

    @Test
    void simplestWithCustomFlow() throws YavDelegationException, InterruptedException {
        var taskletSpy = spy(new FurnitureFactoryStub());
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, taskletSpy);

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(
                TestData.SIMPLEST_RELEASE_PROCESS_ID,
                TestData.TRUNK_R2,
                "simplest-hotfix-flow",
                Common.FlowType.FT_HOTFIX,
                null);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);
        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы, hotfix', " +
                                "произведенного [ИП Иванов HOTFIX 3, ОАО Липа не липа hotfix 2, ООО Пилорама hotfix 1]"
                ));
    }


    @Test
    void onCommit() throws YavDelegationException, InterruptedException, InvalidProtocolBufferException {
        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_FLOW_ID.getPath());

        Launch startLaunch = launch(TestData.SIMPLEST_FLOW_ID, TestData.TRUNK_R2);
        var launch = engineTester.waitLaunch(startLaunch.getLaunchId(), WAIT);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));

        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);
        assertThat(flowLaunch.getVcsInfo().getPreviousRevision()).isEqualTo(TestData.TRUNK_R1);

        // Сначала запускаемся с launchNumber на 1 меньше (и в режиме delayed?)
        var prevLaunch = launch.getId().getLaunchNumber() - 1;
        expectEvents(launch, base -> List.of(
                base.toBuilder().setLaunchNumber(prevLaunch).setEventType(EventType.ET_CREATED)
                        .setDelayed(true).build(),
                base.toBuilder().setEventType(EventType.ET_CREATED).build(),
                base.toBuilder().setEventType(EventType.ET_RUNNING)
                        .setFlowLaunchId(flowLaunch.getIdString()).build(),
                base.toBuilder().setEventType(EventType.ET_FINISHED)
                        .setFlowLaunchId(flowLaunch.getIdString()).setSuccess(true).build()
        ));
    }

    @Test
    void onCommitAttempts() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, new FurnitureFactoryStub() {
            @Override
            public FurnitureFactory.Output execute(FurnitureFactory.Input input) {
                var context = input.getContext();
                if (context.getJobInstanceId().getNumber() == 1) {
                    throw new RuntimeException("test execution failure");
                }
                return super.execute(input);
            }
        });

        var flowId = TestData.SIMPLEST_FLOW_ID;
        engineTester.delegateToken(flowId.getPath());

        Launch startLaunch = launch(flowId, TestData.TRUNK_R2);
        var launch = engineTester.waitLaunch(startLaunch.getLaunchId(), WAIT);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var launches = flowLaunch.getJobState("furniture-factory").getLaunches();
        assertThat(launch.getStatus()).isEqualTo(Status.SUCCESS);
        assertThat(launches).hasSize(2);
        assertThat(launches.get(1).getLastStatusChange().getType())
                .isEqualTo(StatusChangeType.SUCCESSFUL);
    }


    @Test
    void onCommitAttemptsConditionalWithoutRestart() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, new FurnitureFactoryStub() {
            @Override
            public FurnitureFactory.Output execute(FurnitureFactory.Input input) {
                var context = input.getContext();
                if (context.getJobInstanceId().getNumber() == 1) {
                    throw new RuntimeException("test execution failure");
                }
                return super.execute(input);
            }
        });

        var flowId = TestData.SIMPLEST_FLOW_ID;
        engineTester.delegateToken(flowId.getPath());

        var flowVars = JsonParser.parseString("""
                {
                    "lipa": "Липа",
                    "restart": false
                }
                """).getAsJsonObject();
        Launch startLaunch = launch(flowId, TestData.TRUNK_R2, flowVars);
        var launch = engineTester.waitLaunch(startLaunch.getLaunchId(), WAIT);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var launches = flowLaunch.getJobState("furniture-factory").getLaunches();
        assertThat(launch.getStatus()).isEqualTo(Status.FAILURE);
        assertThat(launches).hasSize(1);
        assertThat(launches.get(0).getExecutionExceptionStacktrace())
                .contains("test execution failure");
        assertThat(launches.get(0).getLastStatusChange().getType())
                .isEqualTo(StatusChangeType.FAILED);
    }


    @Test
    void simplestOverrideMultipleResources() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна")); // +1
            resourceCollector.addResource("LOG", Map.of("title", "Сосна")); // +1
            resourceCollector.addResource("LOG", Map.of("title", "Сосна")); // +1
            resourceCollector.addResource("LOG", Map.of("title", "Сосна")); // +1
            resourceCollector.addResource("LOG", Map.of("title", "Дуб")); // Будет проигнорирован
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_RELEASE_OVERRIDE_MULTIPLE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_RELEASE_OVERRIDE_MULTIPLE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'Сосна', произведенного [Тест]"
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

    }

    @Test
    void simplestOverrideSingleResource() throws YavDelegationException, InterruptedException {
        AtomicReference<Map<String, Object>> contextHolder = new AtomicReference<>();
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна"));
            resourceCollector.addResource("LOG", Map.of("title", "Дуб"));

            contextHolder.set(task.getContext());
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_RELEASE_OVERRIDE_SINGLE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_RELEASE_OVERRIDE_SINGLE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var output = Set.copyOf(getResources(resources, JobResourceType.ofDescriptor(Schema.VaultValue.getDescriptor()),
                o -> o.getAsJsonObject("data").get("key").getAsString()));

        assertThat(output)
                .isEqualTo(Set.of("Дуб", "Сосна"));

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var expectedContext = TestUtils.parseProtoTextFromString("""
                job_instance_id {
                  job_id: "generate"
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
                  id: "simplest-release-override-single-resource"
                }
                launch_number: 1
                flow_triggered_by: "andreevdm"
                ci_url: "https://arcanum-test-url/projects/ci/ci/releases/flow?dir=release%2Fsawmill\
                &id=simplest-release-override-single-resource&version=1"
                ci_job_url: "https://arcanum-test-url/projects/ci/ci/releases/flow?dir=release%2Fsawmill\
                &id=simplest-release-override-single-resource&version=1&selectedJob=generate&launchNumber=1"
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

        assertThat(contextHolder.get().get("__CI_CONTEXT"))
                .isEqualTo(ProtobufSerialization.serializeToGsonMap(expectedContext));
    }


    @Test
    void simplestSandbox() throws YavDelegationException, InterruptedException {
        var killTimeout = new AtomicLong();
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            assertThat(task.getPriority())
                    .isEqualTo(new SandboxTaskPriority(PriorityClass.SERVICE, PrioritySubclass.NORMAL));

            killTimeout.set(Objects.requireNonNullElse(task.getKillTimeout(), 0L));
            assertThat(task.getContext()).containsOnlyKeys("__CI_CONTEXT");

            assertThat(task.getRequirements().getSemaphores().getAcquires())
                    .isEqualTo(List.of(new TaskSemaphoreAcquire().setName("acquire-andreevdm")));

            resourceCollector.addResource("PRODUCER", Map.of("title", "ООО Пилорама не пилорама"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна"));
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));

            resourceCollector.setOutput(Map.of("name", "ИП Иванов не Иванов"));
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_SANDBOX_RELEASE_PROCESS_ID;
        var revision = TestData.TRUNK_R2;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы', " +
                                "произведенного [ИП Иванов не Иванов, ООО Пилорама не пилорама, ОАО Липа]"
                ));

        assertThat(killTimeout.get())
                .isEqualTo(Duration.ofMinutes(8).toSeconds()); // The one from registry runtime config
    }

    @Test
    void simplestSandboxTemplate() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("Template1", (task, resourceCollector) -> {
            assertThat(task.getPriority().toString())
                    .isEqualTo(new SandboxTaskPriority(PriorityClass.SERVICE, PrioritySubclass.NORMAL).toString());

            assertThat(task.getContext()).containsOnlyKeys("__CI_CONTEXT");
            assertThat(task.getType()).isNull();
            assertThat(task.getTemplateAlias()).isEqualTo("Template1");

            resourceCollector.addResource("PRODUCER", Map.of("title", "ООО Пилорама не пилорама"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна"));
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));

            resourceCollector.setOutput(Map.of("name", "ИП Иванов не Иванов"));
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_SANDBOX_TEMPLATE_RELEASE_PROCESS_ID;
        var revision = TestData.TRUNK_R2;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, revision);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы', " +
                                "произведенного [ИП Иванов не Иванов, ООО Пилорама не пилорама, ОАО Липа]"
                ));
    }

    @Test
    void simplestSandboxContext() throws YavDelegationException, InterruptedException {
        var tags = new AtomicReference<List<String>>();
        var hints = new AtomicReference<List<String>>();
        var killTimeout = new AtomicLong();
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            assertThat(task.getPriority().toString())
                    .isEqualTo(new SandboxTaskPriority(PriorityClass.USER, PrioritySubclass.HIGH).toString());

            killTimeout.set(Objects.requireNonNullElse(task.getKillTimeout(), 0L));
            assertThat(task.getContext()).containsAllEntriesOf(
                    Map.of("param_string", "Test",
                            "param_int", 42,
                            "param_resolve", TestData.CI_USER)
            );

            tags.set(task.getTags());
            hints.set(task.getHints());

            resourceCollector.addResource("PRODUCER", Map.of("title", "ООО Пилорама не пилорама"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна"));
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));

            resourceCollector.setOutput(Map.of("name", "ИП Иванов не Иванов"));
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_SANDBOX_CONTEXT_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_SANDBOX_CONTEXT_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы', " +
                                "произведенного [ИП Иванов не Иванов, ООО Пилорама не пилорама, ОАО Липа]"
                ));

        assertThat(finalLaunch.getFlowLaunchId()).isNotNull();
        assertThat(tags.get())
                .isEqualTo(List.of(
                        "TAG-1",
                        "TAG-2",
                        "JOB-TAG-1-BY-ANDREEVDM",
                        "JOB-TAG-2",
                        "CI",
                        "RELEASE/SAWMILL::SIMPLEST-SANDBOX-CONTEXT-FLOW",
                        "RELEASE:SIMPLEST-SANDBOX-CONTEXT-RELEASE",
                        "JOB-ID:GENERATE"
                ));

        assertThat(hints.get())
                .isEqualTo(List.of(
                        "hint-1",
                        "hint-2",
                        "job-hint-1",
                        "job-hint-2-by-andreevdm",
                        "LAUNCH:" + finalLaunch.getFlowLaunchId().substring(0, 12)
                ));

        assertThat(killTimeout.get())
                .isEqualTo(Duration.ofMinutes(7).toSeconds()); // The one from runtime config
    }


    @Test
    void simplestSandboxBinaryTask() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            var resourceId = task.getRequirements().getTasksResource();
            var containerResourceId = task.getRequirements().getContainerResource();
            var tcpDumpArgs = task.getRequirements().getTcpdumpArgs();
            var desc = task.getDescription();
            if (desc.contains("prepare1")) {
                assertThat(resourceId).isEqualTo(1L); // release
                assertThat(containerResourceId).isNull();
                assertThat(tcpDumpArgs).isNull();
            } else if (desc.contains("prepare2")) {
                assertThat(resourceId).isEqualTo(2L); // testing
                assertThat(containerResourceId).isNull();
                assertThat(tcpDumpArgs).isNull();
            } else if (desc.contains("prepare3")) {
                assertThat(resourceId).isEqualTo(3L); // custom version
                assertThat(containerResourceId).isNull();
                assertThat(tcpDumpArgs).isNull();
            } else if (desc.contains("prepare4")) {
                assertThat(resourceId).isEqualTo(3L); // custom version from prepare3
                assertThat(containerResourceId).isEqualTo(3L);
                assertThat(tcpDumpArgs).isEqualTo("-A 'tcp port 8080'");
            } else {
                fail("Unexpected task: " + desc);
            }
            resourceCollector.addResource("MY_TYPE", Map.of("version", String.valueOf(resourceId)));

            resourceCollector.setOutput(Map.of("name", "ИП Иванов не Иванов"));
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_SANDBOX_BINARY_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_SANDBOX_BINARY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));
    }

    @Test
    void startSandboxTaskFailed() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            assertThat(task.getContext()).containsOnlyKeys("__CI_CONTEXT");

            resourceCollector.addResource("PRODUCER", Map.of("title", "ООО Пилорама не пилорама"));
            resourceCollector.addResource("LOG", Map.of("title", "Сосна"));
            resourceCollector.addResource("PRODUCER", Map.of("title", "ОАО Липа"));

            resourceCollector.setOutput(Map.of("name", "ИП Иванов не Иванов"));
        });

        sandboxClientStub.setFailStart(true);
        upsertCommit(TestData.TRUNK_COMMIT_2);
        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), TestData.TRUNK_R2.toRevision(), false);
        engineTester.delegateToken(TestData.SIMPLEST_SANDBOX_RELEASE_PROCESS_ID.getPath());
        Launch launch = launch(TestData.SIMPLEST_SANDBOX_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.FAILURE);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        FlowLaunchEntity flowLaunch =
                flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));
        var state = flowLaunch.getFailedJobs().get(0);
        assertThat(state.getLastLaunch()).isNotNull();
        assertThat(state.getLastLaunch().getLastStatusChange().getType()).isEqualTo(StatusChangeType.FAILED);
        var taskState = state.getLastLaunch().getTaskStates().get(0);
        assertThat(taskState.getStatus().toString()).isEqualTo("FAILED");
        assertThat(taskState.getModule()).isEqualTo("SANDBOX");
        assertThat(taskState.getId()).isEqualTo("ci:sandbox");
    }


    @Test
    void fromReleaseBranch() throws YavDelegationException, InterruptedException {
        CiProcessId processId = TestData.EMPTY_RELEASE_PROCESS_ID;

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), TestData.TRUNK_R6.toRevision(), false);
        engineTester.delegateToken(processId.getPath());

        Branch branch = db.currentOrTx(() ->
                branchService.createBranch(processId, TestData.TRUNK_R6, TestData.CI_USER));

        List.of(
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        ).forEach(revision -> discoveryServicePostCommits.processPostCommit(
                revision.getBranch(), revision.toRevision(), false
        ));

        ArcBranch createdArcBranch = branch.getItem().getArcBranch();
        assertThat(createdArcBranch.isRelease()).isTrue();

        assertThat(getTimeline(processId, ArcBranch.trunk()))
                .extracting(TimelineItem::isInStable)
                .containsExactly(false);

        // TODO CI-1801 делегация токена в ветку должны быть не нужна, поправить
        engineTester.delegateToken(processId.getPath(), createdArcBranch);

        Launch launch = launch(processId, TestData.RELEASE_R6_2);

        assertThat(getTimeline(processId, ArcBranch.trunk()))
                .extracting(item -> item.getBranch().getItem().getArcBranch())
                .containsExactly(createdArcBranch);

        assertThat(getTimeline(processId, createdArcBranch))
                .extracting(item -> item.getLaunch().getLaunchId())
                .containsExactly(launch.getLaunchId());

        assertThat(launch.getVcsInfo().getRevision()).isEqualTo(TestData.RELEASE_R6_2);
        assertThat(launch.getVersion()).isEqualTo(Version.majorMinor("1", "1"));

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        assertThat(getTimeline(processId, ArcBranch.trunk()))
                .extracting(TimelineItem::isInStable)
                .containsExactly(true);
    }

    @Test
    void cancel() throws YavDelegationException, InterruptedException {
        ManualStatusTaskExecutor sawmillExecutor = new ManualStatusTaskExecutor();
        sandboxClientStub.uploadTasklet(TaskletResources.SAWMILL, new SawmillStub(), sawmillExecutor);

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SAWMILL_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        Launch runningLaunch = engineTester.waitJob(
                launch.getLaunchId(), WAIT, "sawmill-2", StatusChangeType.RUNNING
        ).getLaunch();

        assertThat(getTimeline(TestData.SAWMILL_RELEASE_PROCESS_ID))
                .extracting(TimelineItem::getLaunch)
                .contains(runningLaunch);

        sawmillExecutor.setTaskStatus(SandboxTaskStatus.SUCCESS);
        launchService.cancel(launch.getLaunchId(), TestData.CI_USER, "test cancel");

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.CANCELED);
        assertThat(getTimeline(TestData.SAWMILL_RELEASE_PROCESS_ID)).isEmpty();
    }

    //    @Test
//  TODO  https://st.yandex-team.ru/CI-3385
    void cancelReleaseWithInterruptingRunningJobs() throws YavDelegationException, InterruptedException {
        //This test runs longer, especially with temporal (for now)
        var overridedWait = Duration.ofMinutes(3);

        ManualStatusTaskExecutor sawmillExecutor = new ManualStatusTaskExecutor();
        sandboxClientStub.uploadTasklet(TaskletResources.SAWMILL, new SawmillStub(), sawmillExecutor);

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SAWMILL_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        var launchAndJob = engineTester.waitJob(
                launch.getLaunchId(), overridedWait, "sawmill-2", StatusChangeType.RUNNING
        );
        var job = launchAndJob.getJobState();
        assertThat(job.getLastLaunch()).isNotNull();
        Launch runningLaunch = launchAndJob.getLaunch();

        assertThat(getTimeline(TestData.SAWMILL_RELEASE_PROCESS_ID))
                .extracting(TimelineItem::getLaunch)
                .contains(runningLaunch);

        launchService.cancel(launch.getLaunchId(), TestData.CI_USER, "test cancel");

        engineTester.waitJob(launch.getLaunchId(), overridedWait, "sawmill-2", StatusChangeType.INTERRUPTING);

        sawmillExecutor.setTaskStatus(SandboxTaskStatus.FAILURE);
        engineTester.waitJob(launch.getLaunchId(), overridedWait, "sawmill-2", StatusChangeType.INTERRUPTED);

        engineTester.waitLaunch(launch.getLaunchId(), overridedWait, Status.CANCELED);
        assertThat(getTimeline(TestData.SAWMILL_RELEASE_PROCESS_ID)).isEmpty();
    }

    @Test
    void internalJobFromPr() throws YavDelegationException, InterruptedException {
        internalJobParameters.getStringValue().set("test");

        processCommits(TestData.TRUNK_COMMIT_2);
        engineTester.delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        engineTester.delegateToken(TestData.ON_PR_INTERNAL_PROCESS_ID.getPath());
        upsertCommit(TestData.DS4_COMMIT);
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(TestData.DIFF_SET_4));

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds4.json");
        List<LaunchId> launchIds = discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_4, false);

        assertThat(launchIds.size()).isEqualTo(1);

        Launch finalLaunch = engineTester.waitLaunch(launchIds.get(0), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launchIds.get(0)), "internal-task-job");

        assertThat(resources.getResources().size()).isEqualTo(1);
        assertThat(
                resources.getResources().stream()
                        .map(r -> r.getObject().getAsJsonObject("data").get("string").getAsString())
                        .collect(Collectors.joining())
        ).isEqualTo("test");

        var job = engineTester.waitJob(launchIds.get(0), WAIT, "verify", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job.isConditionalSkip()).isFalse();
    }

    @ParameterizedTest
    @EnumSource(value = PullRequestDiffSet.Status.class, names = {"COMPLETE", "SKIP"})
    void internalJobFromPrAndClosedCheck(PullRequestDiffSet.Status status)
            throws YavDelegationException, InterruptedException {
        internalJobParameters.getStringValue().set("-");

        processCommits(TestData.TRUNK_COMMIT_2);
        engineTester.delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        engineTester.delegateToken(TestData.ON_PR_INTERNAL_PROCESS_ID.getPath());
        upsertCommit(TestData.DS4_COMMIT);
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(TestData.DIFF_SET_4));

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds4.json");
        List<LaunchId> launchIds = discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_4, false);

        assertThat(launchIds.size()).isEqualTo(1);

        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(TestData.DIFF_SET_4
                .withStatus(status)));

        Launch finalLaunch = engineTester.waitLaunch(launchIds.get(0), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(finalLaunch.getLaunchId()), "internal-task-job");

        assertThat(resources.getResources().size()).isEqualTo(1);
        assertThat(
                resources.getResources().stream()
                        .map(r -> r.getObject().getAsJsonObject("data").get("string").getAsString())
                        .collect(Collectors.joining())
        ).isEqualTo("-");

        var job = engineTester.waitJob(launchIds.get(0), WAIT, "verify", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job.isConditionalSkip()).isTrue();
    }

    @Test
    void woodcutterExceedArtifactCount() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID.getPath());

        Launch launchR1 = launch(TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        var waitLaunch = engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);


        var id = FlowLaunchId.of(waitLaunch.getLaunchId());
        var list = new ArrayList<StoredResource>();
        list.addAll(resources(id, "t-1", 50, 1));
        list.addAll(resources(id, "t-2", 50, 1));
        list.addAll(resources(id, "t-3", 50, 1));
        list.addAll(resources(id, "t-4", 50, 1));

        var container = new StoredResourceContainer(list);
        db.currentOrTx(() ->
                db.resources().saveResources(container, ResourceEntity.ResourceClass.DYNAMIC, clock));

        disableManualSwitch(waitLaunch, "sawmill-wait");

        // Слишком много ресурсов сохранено во flow (200+), задача не сможет завершиться
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.FAILURE);
        var failedJob = engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-wait",
                StatusChangeType.FAILED).getJobState();
        assertThat(failedJob.getLastLaunch()).isNotNull();
        assertThat(failedJob.getLastLaunch().getExecutionExceptionStacktrace())
                .contains("Unable to save resources. Total artifact count limit is 200");
    }

    @Test
    void woodcutterWithPermissionsCheck() throws YavDelegationException, InterruptedException {
        abcServiceStub.addService(Abc.TE); // Remove default access

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        Launch launchR1 = launch(processId, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // OK
        var configRevision = launchR1.getFlowInfo().getConfigRevision();
        permissionsService.checkAccess(TestData.CI_USER, processId, configRevision, PermissionScope.START_JOB);

        assertThatThrownBy(() -> permissionsService.checkJobApprovers(
                TestData.CI_USER,
                processId,
                configRevision,
                launchR1.getFlowInfo().getFlowId().getId(),
                "sawmill-wait",
                "manual trigger"))
                .hasMessage(String.format("PERMISSION_DENIED: User [%s] has no access to manual trigger within rules " +
                        "[[ABC Service = testenv, scopes = [administration]]]", TestData.CI_USER));

        abcServiceStub.addService(Abc.TE, TestData.CI_USER);
        permissionsService.checkJobApprovers(
                TestData.CI_USER,
                processId,
                configRevision,
                launchR1.getFlowInfo().getFlowId().getId(),
                "sawmill-wait",
                "manual trigger");
    }

    @Test
    void startReleaseWithoutNewCommits() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3);

        engineTester.delegateToken(TestData.DUMMY_RELEASE_PROCESS_ID.getPath());

        Launch firstLaunch = launch(TestData.DUMMY_RELEASE_PROCESS_ID, TestData.TRUNK_R3);

        assertThat(firstLaunch.getVersion()).isEqualTo(Version.major("1"));
        assertThat(firstLaunch.getVcsInfo().getCommitCount()).isEqualTo(2);
        assertThat(firstLaunch.getVcsInfo().getPreviousRevision()).isNull();
        engineTester.waitLaunch(firstLaunch.getLaunchId(), Duration.ofSeconds(20));

        var secondLaunch = launchService.startRelease(TestData.DUMMY_RELEASE_PROCESS_ID, TestData.TRUNK_R3,
                ArcBranch.trunk(), "user_1", null, false, false, null, true, null, null, null);
        assertThat(secondLaunch.getVersion()).isEqualTo(Version.major("2"));
        assertThat(secondLaunch.getVcsInfo().getCommitCount()).isEqualTo(0);
        assertThat(secondLaunch.getVcsInfo().getPreviousRevision()).isEqualTo(TestData.TRUNK_R3);
    }

    @Test
    void startFlowFromRoot() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.ROOT_RELEASE_PROCESS_ID;

        engineTester.delegateToken(releaseProcessId.getPath());
        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var jobWoodcutter = flowLaunch.getJobState("woodcutter");
        assertThat(jobWoodcutter.getTitle()).isEqualTo("Дровосек");
        assertThat(jobWoodcutter.getDescription()).isEqualTo("Рубит деревья на бревна");

        var jobSawmill1 = flowLaunch.getJobState("furniture-factory");
        assertThat(jobSawmill1.getTitle()).isEqualTo("Фабрика");
    }

    @Test
    void startFlowWithAnyFailSuccess() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.ROOT_ANY_FAIL_RELEASE_PROCESS_ID;

        engineTester.delegateToken(releaseProcessId.getPath());
        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var factoryError = flowLaunch.getJobState("factory-error");
        assertThat(factoryError.getLastLaunch())
                .isNull();

        var factorySuccess = flowLaunch.getJobState("factory-success");
        assertThat(factorySuccess.getLastLaunch())
                .isNotNull();
    }

    @Test
    void startFlowWithAnyFailFailure() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadTasklet(TaskletResources.WOODCUTTER, new WoodcutterStub() {
            @Override
            public Woodcutter.Output execute(Woodcutter.Input input) {
                throw new RuntimeException("Internal error");
            }
        });
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.ROOT_ANY_FAIL_RELEASE_PROCESS_ID;

        engineTester.delegateToken(releaseProcessId.getPath());
        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var factoryError = flowLaunch.getJobState("factory-error");
        assertThat(factoryError.getLastLaunch())
                .isNotNull();

        var factorySuccess = flowLaunch.getJobState("factory-success");
        assertThat(factorySuccess.getLastLaunch())
                .isNull();
    }

    @Test
    void startFlowForTickets() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.SAWMILL_TICKETS_RELEASE_PROCESS_ID;
        engineTester.delegateToken(releaseProcessId.getPath());

        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Check if input parameters are resolved with JMES
        var resources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "start-release");
        assertThat(
                resources.getResources().stream()
                        .map(r -> r.getObject().getAsJsonObject("data").getAsJsonObject("secret"))
                        .filter(Objects::nonNull)
                        .map(j -> j.get("key").getAsString())
                        .collect(Collectors.joining())
        ).isEqualTo("my-token");

        // Check if expression (with those resolved parameters) is true, i.e. everything was resolved proper
        var checkJob = flowLaunch.getJobState("check-completion");
        assertThat(checkJob.getLastLaunch()).isNotNull();
        assertThat(checkJob.isConditionalSkip()).isFalse();


    }

    @Test
    void flowWithOnFail() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            resourceCollector.addResource("LOGS", Map.of("on", "failure"));
            resourceCollector.setStatus(SandboxTaskStatus.FAILURE);
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.SAWMILL_CONDITIONAL_FAIL_PROCESS_ID;
        engineTester.delegateToken(releaseProcessId.getPath());

        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var jobRun = flowLaunch.getJobState("run");
        assertThat(jobRun.getLastLaunch()).isNotNull();
        assertThat(jobRun.getLastStatusChangeType()).isEqualTo(StatusChangeType.FAILED);

        var resources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "run");
        assertThat(resources.getResources().size()).isEqualTo(0); // No resources stored

        var jobOnSuccess = flowLaunch.getJobState("on-success");
        assertThat(jobOnSuccess.getLastLaunch()).isNull(); // Skipped

        var jobOnFailure = flowLaunch.getJobState("on-failure");
        assertThat(jobOnFailure.getLastLaunch()).isNotNull();
        assertThat(jobOnFailure.getLastStatusChangeType()).isEqualTo(StatusChangeType.SUCCESSFUL);
    }


    @Test
    void flowWithOnFailWithResources() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            resourceCollector.addResource("LOGS", Map.of("on", "failure"));
            resourceCollector.setStatus(SandboxTaskStatus.FAILURE);
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var releaseProcessId = TestData.SAWMILL_CONDITIONAL_FAIL_WITH_RESOURCES_PROCESS_ID;
        engineTester.delegateToken(releaseProcessId.getPath());

        Launch launch = launch(releaseProcessId, TestData.TRUNK_R2);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        var jobRun = flowLaunch.getJobState("run");
        assertThat(jobRun.getLastLaunch()).isNotNull();
        assertThat(jobRun.getLastStatusChangeType()).isEqualTo(StatusChangeType.FAILED);

        var resources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "run");
        assertThat(resources.getResources().size()).isEqualTo(1); // Stored LOGS resource
        assertThat(
                resources.getResources().stream()
                        .map(r -> r.getObject().getAsJsonObject("data").get("type").getAsString())
                        .collect(Collectors.joining())
        ).isEqualTo("LOGS");

        var jobOnSuccess = flowLaunch.getJobState("on-success");
        assertThat(jobOnSuccess.getLastLaunch()).isNull(); // Skipped

        var jobOnFailure = flowLaunch.getJobState("on-failure");
        assertThat(jobOnFailure.getLastLaunch()).isNotNull();
        assertThat(jobOnFailure.getLastStatusChangeType()).isEqualTo(StatusChangeType.SUCCESSFUL);
    }

    @Test
    void flowForLargeTest() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);
        engineTester.delegateToken(VirtualCiProcessId.LARGE_FLOW.getPath());

        var processId = VirtualCiProcessId.toVirtual(CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java"), VirtualType.VIRTUAL_LARGE_TEST);

        // We cannot use direct launchService call
        var launch = onCommitLaunchService.startFlow(StartFlowParameters.builder()
                .processId(processId)
                .branch(TestData.TRUNK_R2.getBranch())
                .revision(TestData.TRUNK_R2.toRevision())
                .triggeredBy(TestData.CI_USER)
                .build()
        );
        assertThat(launch.getProject())
                .isEqualTo(VirtualType.VIRTUAL_LARGE_TEST.getService());

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);


        // Virtual launch with job
        engineTester.waitJob(launch.getLaunchId(), WAIT, "execute-single-task", StatusChangeType.SUCCESSFUL);
    }

    @Test
    void sourcePathOfMovedFileAffectsAYaml() {
        arcServiceStub.reset();
        arcServiceStub.initRepo("test-repos/moved-file");
        arcServiceStub.addCommit(TestData.TRUNK_COMMIT_2, null, Map.of(
                Path.of("from/a.yaml"), ChangeType.Add,
                Path.of("from/file.txt"), ChangeType.Add
        ));
        arcServiceStub.addCommitExtended(TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_2, Map.of(
                Path.of("to/file.txt"), ChangeInfo.of(ChangeType.Move, Path.of("from/file.txt"))
        ));

        processCommits(TestData.TRUNK_COMMIT_2);
        processCommits(TestData.TRUNK_COMMIT_3);

        db.currentOrTx(() -> {
            var r3IsDiscovered = db.discoveredCommit().findCommit(
                            CiProcessId.ofRelease(Path.of("from/a.yaml"), "release1"),
                            TestData.TRUNK_R3
                    )
                    .map(DiscoveredCommit::getState)
                    .map(DiscoveredCommitState::isDirDiscovery)
                    .orElse(false);
            assertThat(r3IsDiscovered).isTrue();
        });
    }
}
