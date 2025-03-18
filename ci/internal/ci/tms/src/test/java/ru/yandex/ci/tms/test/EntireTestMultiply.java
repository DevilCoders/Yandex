package ru.yandex.ci.tms.test;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import java.util.stream.IntStream;

import WoodflowCi.sawmill.Sawmill;
import com.google.protobuf.Timestamp;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.ArgumentCaptor;

import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.TaskSemaphoreAcquire;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.job.TaskletContext;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.test.woodflow.SawmillStub;
import ru.yandex.ci.tms.test.woodflow.WoodcutterStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

@Slf4j
public class EntireTestMultiply extends AbstractEntireTest {

    @Test
    void simplestMultiply() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            assertThat(task.getCustomFields()).isEqualTo(
                    List.of(
                            new SandboxCustomField("boards_1_count", 3),
                            new SandboxCustomField("boards_2_count", 3)
                    ));
            for (var state : ResourceState.values()) {
                resourceCollector.addResource(state, "R-" + state, Map.of());
            }
            assertThat(task.getRequirements().getSemaphores().getAcquires())
                    .isEqualTo(List.of(new TaskSemaphoreAcquire().setName("acquire-andreevdm-3")));
        });

        var taskletSpy = spy(new SawmillStub());
        sandboxClientStub.uploadTasklet(TaskletResources.SAWMILL, taskletSpy);

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        var launch = launch(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Make sure all 3 dummy tasks are executed
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        for (int i = 1; i <= 3; i++) {
            var job = flowLaunch.getJobState("dummy-" + i);
            assertThat(job).isNotNull();
            assertThat(job.getLastLaunch()).isNotNull();
        }

        launchJob(waitLaunch, "sawmill-1-2", TestData.CI_USER);
        var triggeredAt = disableManualSwitch(waitLaunch, "sawmill-1-1");

        var job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();

        var job12 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job12.getLastLaunch()).isNotNull();
        assertThat(Set.of(
                Objects.requireNonNull(job11.getManualTriggerPrompt().getQuestion()),
                Objects.requireNonNull(job12.getManualTriggerPrompt().getQuestion())))
                .isEqualTo(Set.of(
                        "Process value1 for бревно из дерева Липа от sawmill-1?",
                        "Process value1 for бревно из дерева Береза от sawmill-1?"));

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        assertThat(furnitures).isEqualTo(
                List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Береза', " +
                                "произведенного [Пилит бревно из дерева Береза by %s]", user),
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Липа', " +
                                "произведенного [Пилит бревно из дерева Липа by %s]", user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

        // only 2 sandbox resources expected
        var sandboxResources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "prepare-1");
        assertThat(sandboxResources.getResources())
                .isNotEmpty()
                .extracting(StoredResource::instantiate)
                .extracting(Resource::getData)
                .extracting(json -> json.get("type").getAsString())
                .containsExactlyInAnyOrder("R-READY", "R-NOT_READY");


        var inputCapture = ArgumentCaptor.forClass(Sawmill.Input.class);
        verify(taskletSpy, atLeast(2)).execute(inputCapture.capture());

        var inputs = inputCapture.getAllValues();
        var input = inputs.stream()
                .map(Sawmill.Input::getContext)
                .filter(context -> "sawmill-1-1".equals(context.getJobInstanceId().getJobId()))
                .distinct()
                .toList();

        var flowLaunchId = finalLaunch.getFlowLaunchId();
        assertThat(flowLaunchId).isNotNull();

        flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var expectedContext = TestUtils.parseProtoText("expected-context.pb", TaskletContext.class);
        expectedContext = updateContext(expectedContext, finalLaunch, flowLaunch);
        expectedContext = expectedContext.toBuilder()
                .setJobTriggeredAt(ProtoConverter.convert(triggeredAt))
                .build();

        assertThat(input)
                .isEqualTo(List.of(expectedContext));

        var sawmill12Context = inputs.stream()
                .map(Sawmill.Input::getContext)
                .filter(context -> "sawmill-1-2".equals(context.getJobInstanceId().getJobId()))
                .findFirst()
                .orElseThrow();

        assertThat(sawmill12Context.getJobTriggeredBy()).isEqualTo(TestData.CI_USER);
        assertThat(sawmill12Context.getJobTriggeredAt()).isEqualTo(Timestamp.getDefaultInstance());
    }


    @ParameterizedTest
    @MethodSource
    void simplestMultiplyVirtual(CiProcessId processId) throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) ->
                // Artifacts are loaded from current task (not from all upstreams)
                assertThat(task.getCustomFields()).isEqualTo(
                        List.of(
                                new SandboxCustomField("boards_1_count", 3L),
                                new SandboxCustomField("boards_2_count", 3L)
                        ))
        );

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
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

    @Test
    void simplestSandboxMultiply() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (context, resourceCollector) -> {
            resourceCollector.addResource("SRC", Map.of("name", "ООО Пилорама"));
            resourceCollector.addResource("SRC", Map.of("name", "ОАО Сосна"));
            resourceCollector.addResource("SRC", Map.of("name", "ЗАО Липа"));
        });

        var tags = new AtomicReference<List<String>>();
        var hints = new AtomicReference<List<String>>();
        var jobId = new AtomicReference<String>();
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
            String type = "unknown";
            for (SandboxCustomField field : task.getCustomFields()) {
                if (field.getName().equals("type")) {
                    type = String.valueOf(field.getValue());
                }
            }
            for (SandboxCustomField field : task.getCustomFields()) {
                if (field.getName().equals("produced")) {
                    var produced = String.valueOf(field.getValue());
                    resourceCollector.addResource(type, Map.of("title", produced));
                    if (produced.startsWith("ООО Пилорама")) {
                        tags.set(task.getTags());
                        hints.set(task.getHints());
                        jobId.set(task.getTags().stream()
                                .filter(tag -> tag.startsWith("JOB-ID:"))
                                .findAny()
                                .orElseThrow(() -> new RuntimeException("Unable to find tag starts from JOB-ID:")));
                    }
                }
            }
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_SANDBOX_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_SANDBOX_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        Launch finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        assertThat(furnitures)
                .isEqualTo(List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из липы', " +
                                        "произведенного [ОАО Сосна by %s, ООО Пилорама by %s, ЗАО Липа by %s]",
                                user, user, user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();


        assertThat(finalLaunch.getFlowLaunchId()).isNotNull();
        assertThat(tags.get())
                .isEqualTo(List.of(
                        "JOB-TAG-1-BY-",
                        "JOB-TAG-2",
                        "CI",
                        "RELEASE/SAWMILL::SIMPLEST-SANDBOX-MULTIPLY-FLOW",
                        "RELEASE:SIMPLEST-SANDBOX-MULTIPLY-RELEASE",
                        jobId.get().toUpperCase()
                ));

        assertThat(hints.get())
                .isEqualTo(List.of(
                        "job-hint-1",
                        "job-hint-2-by-",
                        "LAUNCH:" + finalLaunch.getFlowLaunchId().substring(0, 12)
                ));

    }

    @Test
    void simplestSandboxMultiplyException() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (context, resourceCollector) ->
                IntStream.rangeClosed(1, 21).forEach(id ->
                        resourceCollector.addResource("SRC", Map.of("name", "Ресурс " + id)))
        );

        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX", (task, resourceCollector) -> {
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_SANDBOX_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        Launch launch = launch(TestData.SIMPLEST_SANDBOX_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        // We generate 21 jobs in 'generate' stage with maximum of 20
        var ret = engineTester.waitJob(launch.getLaunchId(), WAIT,
                "generate", StatusChangeType.FAILED);

        var runningLaunch = ret.getLaunch();
        var failedJob = ret.getJobState();

        assertThat(runningLaunch.getStatus()).isEqualTo(Status.FAILURE);
        assertThat(runningLaunch.getVersion()).isEqualTo(Version.major("1"));

        assertThat(failedJob.getLastLaunch()).isNotNull();
        assertThat(failedJob.getLastLaunch().getExecutionExceptionStacktrace())
                .contains("Job generate size cannot exceed 20, got 21");
    }

    @Test
    void simplestMultiplyRetryMultiplicationUnchanged() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            assertThat(task.getCustomFields()).isEqualTo(
                    List.of(
                            new SandboxCustomField("boards_1_count", 3),
                            new SandboxCustomField("boards_2_count", 3)
                    ));
            for (var state : ResourceState.values()) {
                resourceCollector.addResource(state, "R-" + state, Map.of());
            }
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        var launch = launch(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Execute first job
        disableManualSwitch(waitLaunch, "sawmill-1-1");

        // Recalc with same parameters
        log.info("Recalc job multiply...");
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new TriggerEvent("sawmill-1", "username", false));

        // Wait again
        waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);
        disableManualSwitch(waitLaunch, "sawmill-1-1");
        disableManualSwitch(waitLaunch, "sawmill-1-2");

        var job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();
        // recalc (executed before multiplication restart)
        assertThat(job11.getLastLaunch().getNumber()).isGreaterThanOrEqualTo(2);

        var job12 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job12.getLastLaunch()).isNotNull();

        assertThat(Set.of(
                Objects.requireNonNull(job11.getManualTriggerPrompt().getQuestion()),
                Objects.requireNonNull(job12.getManualTriggerPrompt().getQuestion())))
                .isEqualTo(Set.of(
                        "Process value1 for бревно из дерева Липа от sawmill-1?",
                        "Process value1 for бревно из дерева Береза от sawmill-1?"));

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        assertThat(furnitures).isEqualTo(
                List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Береза', " +
                                "произведенного [Пилит бревно из дерева Береза by %s]", user),
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Липа', " +
                                "произведенного [Пилит бревно из дерева Липа by %s]", user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

        // only 2 sandbox resources expected
        var sandboxResources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "prepare-1");
        assertThat(sandboxResources.getResources())
                .isNotEmpty()
                .extracting(StoredResource::instantiate)
                .extracting(Resource::getData)
                .extracting(json -> json.get("type").getAsString())
                .containsExactlyInAnyOrder("R-READY", "R-NOT_READY");
    }

    @Test
    void simplestMultiplyRetryMultiplicationUnchangedParent() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            assertThat(task.getCustomFields()).isEqualTo(
                    List.of(
                            new SandboxCustomField("boards_1_count", 3),
                            new SandboxCustomField("boards_2_count", 3)
                    ));
            for (var state : ResourceState.values()) {
                resourceCollector.addResource(state, "R-" + state, Map.of());
            }
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        var launch = launch(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Execute first job
        disableManualSwitch(waitLaunch, "sawmill-1-1");

        // Recalc with same parameters
        log.info("Recalc job before multiply...");
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new TriggerEvent("woodcutter", "username", false));

        // Wait again
        waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);
        disableManualSwitch(waitLaunch, "sawmill-1-1");
        disableManualSwitch(waitLaunch, "sawmill-1-2");

        var job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();
        // recalc (executed before multiplication restart)
        assertThat(job11.getLastLaunch().getNumber()).isGreaterThanOrEqualTo(2);

        var job12 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job12.getLastLaunch()).isNotNull();

        // Context is used to evaulate expression just once, during multiply/by evaluation
        assertThat(Set.of(
                Objects.requireNonNull(job11.getManualTriggerPrompt().getQuestion()),
                Objects.requireNonNull(job12.getManualTriggerPrompt().getQuestion())))
                .isEqualTo(Set.of(
                        "Process value1 for бревно из дерева Липа от sawmill-1?",
                        "Process value1 for бревно из дерева Береза от sawmill-1?"));

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        assertThat(furnitures).isEqualTo(
                List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Береза', " +
                                "произведенного [Пилит бревно из дерева Береза by %s]", user),
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Липа', " +
                                "произведенного [Пилит бревно из дерева Липа by %s]", user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

        // only 2 sandbox resources expected
        var sandboxResources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "prepare-1");
        assertThat(sandboxResources.getResources())
                .isNotEmpty()
                .extracting(StoredResource::instantiate)
                .extracting(Resource::getData)
                .extracting(json -> json.get("type").getAsString())
                .containsExactlyInAnyOrder("R-READY", "R-NOT_READY");
    }


    @SuppressWarnings("MethodLength")
    @Test
    void simplestMultiplyRetryMultiplicationChangedConfiguration() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadTasklet(TaskletResources.WOODCUTTER,
                new WoodcutterStub(List.of("Дерево 1", "Дерево 2")));

        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            assertThat(task.getCustomFields()).isEqualTo(
                    List.of(
                            new SandboxCustomField("boards_1_count", 3),
                            new SandboxCustomField("boards_2_count", 3)
                    ));
            for (var state : ResourceState.values()) {
                resourceCollector.addResource(state, "R-" + state, Map.of());
            }
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID.getPath());

        var launch = launch(TestData.SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID, TestData.TRUNK_R2);

        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Execute first job
        disableManualSwitch(waitLaunch, "sawmill-1-1");
        var job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();
        assertThat(job11.getTitle()).isEqualTo("Лесопилка для бревно из дерева Дерево 1 от andreevdm");
        assertThat(job11.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 1");

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        var job12 = flowLaunch.getJobState("sawmill-1-2");
        assertThat(job12.getLastLaunch()).isNull();
        assertThat(job12.getTitle()).isEqualTo("Лесопилка для бревно из дерева Дерево 2 от andreevdm");
        assertThat(job12.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 2");


        // RECALC 1
        log.info("Recalc job before multiply, step 1...");
        sandboxClientStub.uploadTasklet(TaskletResources.WOODCUTTER,
                new WoodcutterStub(List.of("Дерево 1-1", "Дерево 1-2", "Дерево 1-3")));
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new TriggerEvent("woodcutter", "username", false));

        // WAIT RECALC 1
        waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);
        flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        job11 = flowLaunch.getJobState("sawmill-1-1");
        assertThat(job11.getTitle()).isEqualTo("Лесопилка для бревно из дерева Дерево 1-1 от andreevdm");
        assertThat(job11.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 1-1");
        job12 = flowLaunch.getJobState("sawmill-1-2");
        assertThat(job12.getTitle()).isEqualTo("Лесопилка для бревно из дерева Дерево 1-2 от andreevdm");
        assertThat(job12.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 1-2");
        var job13 = flowLaunch.getJobState("sawmill-1-3");
        assertThat(job13.getTitle()).isEqualTo("Лесопилка для бревно из дерева Дерево 1-3 от andreevdm");
        assertThat(job13.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 1-3");
        assertThat(job13.isDisabled()).isFalse();

        // MANUAL TRIGGER FOR RECALC 1
        disableManualSwitch(waitLaunch, "sawmill-1-1");
        disableManualSwitch(waitLaunch, "sawmill-1-3");

        job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();

        job13 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-3", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();
        assertThat(job13.isDisabled()).isFalse();


        // RECALC 2
        log.info("Recalc job before multiply, step 2...");
        sandboxClientStub.uploadTasklet(TaskletResources.WOODCUTTER,
                new WoodcutterStub(List.of("Дерево 1-1", "Дерево 1-2")));
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new TriggerEvent("woodcutter", "username", false));
        waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        disableManualSwitch(waitLaunch, "sawmill-1-1");
        disableManualSwitch(waitLaunch, "sawmill-1-2");

        job11 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job11.getLastLaunch()).isNotNull();
        // recalc (executed before multiplication restart)
        assertThat(job11.getLastLaunch().getNumber()).isGreaterThanOrEqualTo(2);

        job12 = engineTester.waitJob(launch.getLaunchId(), WAIT, "sawmill-1-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(job12.getLastLaunch()).isNotNull();

        assertThat(Set.of(
                Objects.requireNonNull(job11.getManualTriggerPrompt().getQuestion()),
                Objects.requireNonNull(job12.getManualTriggerPrompt().getQuestion())))
                .isEqualTo(Set.of(
                        "Process value1 for бревно из дерева Дерево 1-1 от sawmill-1?",
                        "Process value1 for бревно из дерева Дерево 1-2 от sawmill-1?"));

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        job13 = flowLaunch.getJobState("sawmill-1-3");
        assertThat(job13.getTitle()).isEqualTo("Removed: Лесопилка для бревно из дерева Дерево 1-3 от andreevdm");
        assertThat(job13.getDescription()).isEqualTo("Описание лесопилки для бревно из дерева Дерево 1-3");
        assertThat(job13.isConditionalSkip()).isTrue(); // Пропущена

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        assertThat(furnitures).isEqualTo(
                List.of(
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Дерево 1-1', " +
                                "произведенного [Пилит бревно из дерева Дерево 1-1 by %s]", user),
                        String.format("Шкаф из 3 досок, полученных из материала 'бревно из дерева Дерево 1-2', " +
                                "произведенного [Пилит бревно из дерева Дерево 1-2 by %s]", user)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();

        // only 2 sandbox resources expected
        var sandboxResources = flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "prepare-1");
        assertThat(sandboxResources.getResources())
                .isNotEmpty()
                .extracting(StoredResource::instantiate)
                .extracting(Resource::getData)
                .extracting(json -> json.get("type").getAsString())
                .containsExactlyInAnyOrder("R-READY", "R-NOT_READY");
    }


    static List<CiProcessId> simplestMultiplyVirtual() {
        return List.of(
                TestData.SIMPLEST_MULTIPLY_VIRTUAL_RELEASE_PROCESS_ID,
                TestData.SIMPLEST_MULTIPLY_VIRTUAL_RELEASE_VARS_PROCESS_ID);
    }
}
