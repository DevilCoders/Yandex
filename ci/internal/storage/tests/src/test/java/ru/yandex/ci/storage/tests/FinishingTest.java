package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;

import static org.assertj.core.api.Assertions.assertThat;

public class FinishingTest extends StorageTestsYdbTestBase {
    @Test
    public void startsIterationOnTaskFinish() {
        var registration = storageTester.register(registrar ->
                registrar.check().fullIteration().leftTask(TASK_ID_LEFT).rightTask().complete()
        );

        storageTester.writeAndDeliver(registration, writer -> writer.toLeft().finish());

        assertThat(storageTester.getCheck(registration.getCheck().getId()).getStatus())
                .isEqualTo(Common.CheckStatus.RUNNING);

        assertThat(storageTester.getIteration(registration.getIteration().getId()).getStatus())
                .isEqualTo(Common.CheckStatus.RUNNING);
    }

    @Test
    public void pureTestTypeFinalizationByPartitionAndTasks() {
        var registration = storageTester.register(registrar -> registrar.check()
                .fullIteration()
                .task(TASK_ID_LEFT, 2, false, Common.CheckTaskType.CTT_AUTOCHECK)
                .rightTask(TASK_ID_RIGHT).complete()
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .toRight()
                        .trace("distbuild/started")
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.SMALL)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.MEDIUM)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to(registration.getTasks())
                        .toLeft()
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.SMALL)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.MEDIUM)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
        );

        storageTester.writeAndDeliver(
                registration, writer -> writer.toAll().finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var statistics = iteration.getTestTypeStatistics();
        assertThat(statistics.getConfigure().getCompletedTasks()).isEqualTo(1);
        assertThat(statistics.getBuild().getCompletedTasks()).isEqualTo(1);
        assertThat(statistics.getSmallTests().getCompletedTasks()).isEqualTo(1);
        assertThat(statistics.getMediumTests().getCompletedTasks()).isEqualTo(1);
        assertThat(statistics.getLargeTests().getCompletedTasks()).isEqualTo(1);

        var leftTask = storageTester.getTask(registration.getTask(TASK_ID_LEFT).getId());
        assertThat(leftTask.getStatus()).isEqualTo(Common.CheckStatus.CREATED);

        // left task all types partition 1
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to(registration.getTasks()).partition(1)
                        .toLeft()
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.SMALL)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.MEDIUM)
                        .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
                        .finish()
        );

        iteration = storageTester.getIteration(iteration.getId());

        statistics = iteration.getTestTypeStatistics();
        assertThat(statistics.getConfigure().getCompletedTasks()).isEqualTo(2);
        assertThat(statistics.getBuild().getCompletedTasks()).isEqualTo(2);
        assertThat(statistics.getSmallTests().getCompletedTasks()).isEqualTo(2);
        assertThat(statistics.getMediumTests().getCompletedTasks()).isEqualTo(2);
        assertThat(statistics.getLargeTests().getCompletedTasks()).isEqualTo(2);

        assertThat(statistics.getConfigure().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getBuild().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getSmallTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getMediumTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getLargeTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);

        leftTask = storageTester.getTask(leftTask.getId());
        var rightTask = storageTester.getTask(registration.getTask(TASK_ID_RIGHT).getId());

        assertThat(leftTask.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
        assertThat(rightTask.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
    }

    @Test
    public void testTypeChunkFinalization() {
        var registration = storageTester.register(registrar -> registrar.check()
                .fullIteration()
                .leftTask()
                .complete()
        );

        var small = exampleSmallTest(1L, 100L);
        var medium = exampleMediumTest(2L, 100L);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll()
                .results(small, medium)
                .testTypeFinish(Actions.TestType.BUILD)
                .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.SMALL)
        );

        var iteration = registration.getIteration();
        var statistics = storageTester.getIteration(iteration.getId()).getTestTypeStatistics();

        assertThat(statistics.getBuild().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_WAITING_FOR_CONFIGURE);
        assertThat(statistics.getSmallTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_WAITING_FOR_CONFIGURE);
        assertThat(statistics.getMediumTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_RUNNING);

        // Simultaneous finish of configure and medium
        storageTester.write(registration, writer -> writer.toAll()
                .testTypeFinish(Actions.TestType.CONFIGURE)
                .testTypeFinish(Actions.TestType.STYLE)
                .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.MEDIUM)
                .testTypeSizeFinish(Actions.TestTypeSizeFinished.Size.LARGE)
        );
        logbrokerService.deliverMainStreamMessages();
        logbrokerService.deliverShardOutMessages(); // Finish messages forwarded to corresponding reader.

        statistics = storageTester.getIteration(iteration.getId()).getTestTypeStatistics();

        assertThat(statistics.getBuild().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getSmallTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_WAITING_FOR_CHUNKS);
        assertThat(statistics.getMediumTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_WAITING_FOR_CHUNKS);

        logbrokerService.deliverShardInMessages(); // Shard received chunk finalization
        logbrokerService.deliverShardOutMessages(); // Returned chunk finished

        statistics = storageTester.getIteration(iteration.getId()).getTestTypeStatistics();

        assertThat(statistics.getSmallTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);
        assertThat(statistics.getMediumTests().getStatus()).isEqualTo(Common.TestTypeStatus.TTS_COMPLETED);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().finish());

        assertThat(storageTester.getIteration(iteration.getId()).getStatus())
                .isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
    }

    @Test
    public void nativeFinalization() {
        var registration = storageTester.register(registrar -> registrar.check()
                .heavyIteration()
                .task(TASK_ID_LEFT, 1, false, Common.CheckTaskType.CTT_NATIVE_BUILD)
                .task(TASK_ID_RIGHT, 1, true, Common.CheckTaskType.CTT_NATIVE_BUILD)
        );

        storageTester.writeAndDeliver(registration, writer -> writer.toLeft().finish());

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var statistics = iteration.getTestTypeStatistics();
        assertThat(statistics.getNativeBuilds().getCompletedTasks()).isEqualTo(1);
        assertThat(iteration.getInfo().getProgress()).isEqualTo(50);

        storageTester.writeAndDeliver(registration, writer -> writer.toRight().finish());

        iteration = storageTester.getIteration(registration.getIteration().getId());

        statistics = iteration.getTestTypeStatistics();
        assertThat(statistics.getNativeBuilds().getCompletedTasks()).isEqualTo(2);
        assertThat(iteration.getInfo().getProgress()).isEqualTo(100);
        assertThat(iteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);

        storageTester.executeAllOnetimeTasks();

        var mergeRequirementsId = new CheckMergeRequirementsEntity.Id(
                iteration.getId().getCheckId(), ArcanumCheckType.CI_BUILD_NATIVE
        );

        var merge = db.currentOrReadOnly(() -> db.checkMergeRequirements().find(mergeRequirementsId));
        assertThat(merge).isPresent();
        assertThat(merge.get().getStatus()).isEqualTo(ArcanumMergeRequirementDto.Status.SUCCESS);
    }

    @Test
    public void largeAndTeFinalization() {
        var registration = storageTester.register(
                registrar -> registrar
                        .heavyIteration()
                        .task("task-id-one", 1, false, Common.CheckTaskType.CTT_TESTENV)
                        .complete()
        );

        var taskOneId = registration.getTask("task-id-one").getId();

        var secondRegistration = storageTester.register(
                registration,
                registrar -> registrar
                        .heavyIteration()
                        .task("task-id-two", 1, false, Common.CheckTaskType.CTT_LARGE_TEST)
                        .task("task-id-three", 1, false, Common.CheckTaskType.CTT_LARGE_TEST)
                        .complete()
        );

        var taskTwoId = secondRegistration.getTask("task-id-two").getId();
        var taskThreeId = secondRegistration.getTask("task-id-three").getId();

        var iterationOne = storageTester.getIteration(registration.getIteration().getId());
        var iterationTwo = storageTester.getIteration(secondRegistration.getIteration().getId());

        assertThat(iterationOne.getTestTypeStatistics().getTeTests().getRegisteredTasks()).isEqualTo(1);
        assertThat(iterationTwo.getTestTypeStatistics().getLargeTests().getRegisteredTasks()).isEqualTo(2);

        var iterationMeta = storageTester.getIteration(iterationOne.getId().toMetaId());

        assertThat(iterationMeta.getTestTypeStatistics().getTeTests().getRegisteredTasks()).isEqualTo(1);
        assertThat(iterationMeta.getTestTypeStatistics().getLargeTests().getRegisteredTasks()).isEqualTo(2);

        storageTester.writeAndDeliver(registration, writer -> writer.to(taskOneId).finish());

        storageTester.writeAndDeliver(secondRegistration, writer -> writer.to(taskTwoId).finish());

        storageTester.executeAllOnetimeTasks();

        iterationMeta = storageTester.getIteration(iterationMeta.getId());
        iterationOne = storageTester.getIteration(iterationOne.getId());
        iterationTwo = storageTester.getIteration(iterationTwo.getId());

        assertThat(iterationMeta.getStatus()).isEqualTo(Common.CheckStatus.RUNNING);
        assertThat(iterationOne.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
        assertThat(iterationTwo.getStatus()).isEqualTo(Common.CheckStatus.RUNNING);
        assertThat(iterationTwo.getInfo().getProgress()).isEqualTo(50);

        assertThat(iterationMeta.getTestTypeStatistics().getTeTests().isCompleted()).isTrue();
        assertThat(iterationMeta.getTestTypeStatistics().getLargeTests().isCompleted()).isFalse();

        var mergeRequirementsId = new CheckMergeRequirementsEntity.Id(
                iterationMeta.getId().getCheckId(), ArcanumCheckType.CI_LARGE_TESTS
        );

        assertThat(db.currentOrReadOnly(() -> db.checkMergeRequirements().find(mergeRequirementsId))).isEmpty();

        storageTester.writeAndDeliver(secondRegistration, writer -> writer.to(taskThreeId).finish());

        storageTester.executeAllOnetimeTasks();

        iterationMeta = storageTester.getIteration(iterationMeta.getId());
        iterationTwo = storageTester.getIteration(iterationTwo.getId());
        assertThat(iterationMeta.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
        assertThat(iterationOne.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);
        assertThat(iterationTwo.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);

        assertThat(iterationMeta.getTestTypeStatistics().getLargeTests().isCompleted()).isTrue();
    }
}
