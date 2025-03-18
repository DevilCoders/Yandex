package ru.yandex.ci.observer.tests;

import java.time.Instant;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.tester.StorageTesterRegistrar;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.tests.StorageTestsYdbTestBase.TASK_ID_LEFT;
import static ru.yandex.ci.storage.tests.StorageTestsYdbTestBase.TASK_ID_LEFT_TWO;
import static ru.yandex.ci.storage.tests.StorageTestsYdbTestBase.TASK_ID_RIGHT;
import static ru.yandex.ci.storage.tests.StorageTestsYdbTestBase.TASK_ID_RIGHT_TWO;

public class SimpleTest extends ObserverTestsTestBase {
    @Test
    public void registerObserverEntities() {
        var registration = storageTester.register(registrar ->
                registrar.check().fullIteration().leftTask().complete()
        );

        var check = registration.getCheck();
        var protoIteration = CheckProtoMappers.toProtoIteration(registration.getIteration());
        var protoTask = CheckProtoMappers.toProtoCheckTask(registration.getTasks().iterator().next());
        logbrokerService.deliverAllMessages();

        var expectedCheckId = ObserverProtoMappers.toCheckId(check.getId().getId().toString());
        var expectedIterationId = ObserverProtoMappers.toIterationId(protoIteration.getId());
        var expectedTaskId = ObserverProtoMappers.toTaskId(protoTask.getId());

        assertThat(logbrokerService.areAllReadsCommited()).isTrue();

        ciObserverDb.readOnly(() -> {
            ciObserverDb.checks().get(expectedCheckId);

            var actualIteration = ciObserverDb.iterations().get(expectedIterationId);
            assertThat(actualIteration).usingRecursiveComparison()
                    .ignoringFields("created", "timestampedStagesAggregation")
                    .isEqualTo(CheckIterationEntity.builder()
                            .id(expectedIterationId)
                            .created(Instant.MIN)
                            .checkType(check.getType())
                            .author(check.getAuthor())
                            .left(check.getLeft())
                            .right(check.getRight())
                            .status(registration.getIteration().getStatus())
                            .checkRelatedLinks(Map.of(
                                    "CI_BADGE",
                                    "https://a.yandex-team.ru/ci-card-preview/100000000000",
                                    // ---
                                    "DISTBUILD",
                                    "https://datalens.yandex-team.ru/0e0iocvqdgsuv-distbuildprofiler?review=42",
                                    // ---
                                    "REVIEW",
                                    "https://a.yandex-team.ru/review/42/details",
                                    // ---
                                    "DISTBUILD_VIEWER",
                                    "https://viewer-distbuild.n.yandex-team.ru/search?query=100000000000",
                                    // ---
                                    "SANDBOX",
                                    "https://sandbox.yandex-team.ru/tasks?limit=100" +
                                            "&hints=100000000000%2FFULL&tags=AUTOCHECK",
                                    // ---
                                    "FLOW", "https://a.yandex-team.ru/projects/autocheck/ci/actions/flow" +
                                            "?dir=dir&id=id&number=1"
                            ))
                            .diffSetEventCreated(Instant.EPOCH)
                            .testenvId("")
                            .advisedPool("")
                            .build()
                    );

            ciObserverDb.tasks().get(expectedTaskId);
        });
    }

    @Test
    public void produceEventsOnTaskAndIterationFinish() {
        var registration = storageTester.register(registrar ->
                registrar.check().fullIteration().leftTask().rightTask().complete()
        );

        storageTester.writeAndDeliver(registration, writer -> writer
                .to(registration.getFirstLeft()).results(storageTestEntitiesBase.exampleBuild(1L))
                .to(registration.getFirstRight()).results(storageTestEntitiesBase.exampleFailedBuild(1L))
                .toAll().finish()
        );

        assertThat(logbrokerService.areAllReadsCommited()).isTrue();

        var protoIteration = CheckProtoMappers.toProtoIteration(registration.getIteration());
        ciObserverDb.readOnly(() -> {
            var iterationStatus = ciObserverDb.iterations()
                    .get(ObserverProtoMappers.toIterationId(protoIteration.getId()))
                    .getStatus();
            var traces = ciObserverDb.traces().findAll().stream()
                    .map(t -> t.getId().getTraceType())
                    .collect(Collectors.toSet());

            assertThat(traces).contains("storage/check_task_finished");
            assertThat(iterationStatus).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);
        });
    }

    @Test
    public void processTraces() {
        var registration = storageTester.register(
                registrar -> registrar.iteration(
                        CheckIteration.IterationType.FULL, 1,
                        Common.CheckTaskType.CTT_AUTOCHECK,
                        Set.of(
                                CheckIteration.ExpectedTask.newBuilder()
                                        .setRight(false)
                                        .setJobName("task-id")
                                        .build()
                        )
                ).leftTask("task-id")
        );

        assertThat(logbrokerService.areAllReadsCommited()).isTrue();

        var taskId = CheckProtoMappers.toProtoCheckTaskId(registration.getTasks().iterator().next().getId());

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().trace("distbuild/started"));

        ciObserverDb.readOnly(() -> {
            var traces = ciObserverDb.traces().findAll().stream()
                    .map(t -> t.getId().getTraceType())
                    .collect(Collectors.toSet());
            var task = ciObserverDb.tasks().get(ObserverProtoMappers.toTaskId(taskId));

            assertThat(traces).contains("storage/check_task_created", "distbuild/started");
            assertThat(task.getStatus()).isEqualTo(Common.CheckStatus.RUNNING);
            assertThat(task.getTimestampedStagesAggregation().getStagesFinishes().keySet())
                    .containsExactlyInAnyOrder("creation", "sandbox");
            assertThat(task.getTimestampedStagesAggregation().getStagesStarts().keySet())
                    .containsExactlyInAnyOrder("creation", "sandbox", "configure");
        });
    }

    @Test
    public void observerShouldCancelIterationsOnFatalError() {
        var registration = storageTester.register(registrar -> {
            registrar.check()
                    .fastIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
                    .fullIteration().leftTask(TASK_ID_LEFT_TWO).rightTask(TASK_ID_RIGHT_TWO)
                    .complete();
        });
        storageTester.writeAndDeliver(registration, writer -> writer
                .to(TASK_ID_RIGHT_TWO).autocheckFatalError(storageTestEntitiesBase.exampleAutocheckFatalError())
        );

        assertThat(logbrokerService.areAllReadsCommited()).isTrue();

        ciObserverDb.readOnly(() -> {
            var fastIteration = ciObserverDb.iterations().get(
                    getIteration(registration, CheckIteration.IterationType.FAST)
            );

            var fullIteration = ciObserverDb.iterations().get(
                    getIteration(registration, CheckIteration.IterationType.FULL)
            );

            assertThat(fastIteration.getStatus()).isEqualTo(Common.CheckStatus.CANCELLED);
            assertThat(fullIteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR);
        });
    }

    private CheckIterationEntity.Id getIteration(
            StorageTesterRegistrar.RegistrationResult registration,
            CheckIteration.IterationType iterationType
    ) {
        return registration.getIterations().stream()
                .filter(it -> it.getId().getIterationType() == iterationType)
                .findFirst()
                .map(CheckProtoMappers::toProtoIteration)
                .map(it -> ObserverProtoMappers.toIterationId(it.getId()))
                .orElseThrow();
    }
}
