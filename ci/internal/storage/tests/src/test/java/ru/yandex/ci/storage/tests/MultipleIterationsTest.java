package ru.yandex.ci.storage.tests;

import java.util.Set;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.Common.TestTypeStatus;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.tests.tester.StatisticsTemplates;

import static org.assertj.core.api.Assertions.assertThat;

public class MultipleIterationsTest extends StorageTestsYdbTestBase {
    @Test
    public void parallel() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );
        var secondRegistration = storageTester.register(
                registration, registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        storageTester.writeAndDeliver(writer -> writer.to(registration.getTasks()).trace("distbuild/started"));

        var firstTestResult = exampleMediumTest(1L, 100L);
        var secondTestResult = exampleMediumTest(2L, 200L);

        storageTester.writeAndDeliver(writer -> writer.to(registration.getTasks()).results(firstTestResult));
        storageTester.writeAndDeliver(writer -> writer.to(secondRegistration.getTasks()).results(firstTestResult));

        // At this moment both iterations are running and chunk statistics was copied to meta iteration.

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .toLeft().results(secondTestResult)
                        .toRight()
                        .results(secondTestResult.toBuilder().setTestStatus(TestStatus.TS_FAILED).build())
        );

        // Second test fails in first iteration.

        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer
                        .toLeft().results(secondTestResult)
                        .toRight().results(secondTestResult)
        );

        // Second test passes in second iteration, it causes statistics rollback.
        logbrokerService.deliverAllMessages();

        var iteration = storageTester.getIteration(registration.getIteration().getId());
        assertThat(iteration.getStatus()).isEqualTo(CheckStatus.RUNNING);

        // Ensure test itself works and first iteration have expected statistics.
        assertThat(iteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1)
                                        .failed(1)
                                        .failedAdded(1)
                                        .build()
                        )
                        .mediumTests(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1)
                                        .failed(1)
                                        .failedAdded(1)
                                        .build()
                        )
                        .build()
        );

        var metaIteration = storageTester.getIteration(iteration.getId().toMetaId());

        // Validate meta iteration statistics.
        assertThat(metaIteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1)
                                        .build()
                        )
                        .mediumTests(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1)
                                        .build()
                        )
                        .build()
        );

        assertThat(metaIteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                ExtendedStatistics.builder()
                        .flaky(
                                ExtendedStageStatistics.builder()
                                        .total(1)
                                        .failedAdded(1)
                                        .build()
                        )
                        .build()
        );
    }

    @Test
    public void three() {
        var registration = storageTester.register(registrar -> registrar.check()
                .fullIteration()
                .leftTask(TASK_ID_LEFT)
                .rightTask(TASK_ID_RIGHT)
                .fullIteration()
                .leftTask(TASK_ID_LEFT_TWO)
                .rightTask(TASK_ID_RIGHT_TWO)
                .complete()
        );

        var result = exampleSuite(100L);

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .toAll()
                        .trace(DISTBUILD_STARTED)
                        .results(result)
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
        );

        var metaIterationId = registration.getIteration().getId().toMetaId();
        var metaIteration = storageTester.getIteration(metaIterationId);

        assertThat(metaIteration.getTestTypeStatistics().getBuild().getStatus())
                .isEqualTo(TestTypeStatus.TTS_COMPLETED);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().finish());

        storageTester.assertIterationStatus(metaIterationId.toIterationId(1), CheckStatus.COMPLETED_SUCCESS);
        storageTester.assertIterationStatus(metaIterationId.toIterationId(2), CheckStatus.COMPLETED_SUCCESS);

        metaIteration = storageTester.getIteration(metaIterationId);

        // Validate meta iteration statistics.
        assertThat(metaIteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(StatisticsTemplates.stagePassed(1))
                        .mediumTests(StatisticsTemplates.stagePassed(1))
                        .build()
        );

        registration = storageTester.register(
                registration, registrar -> registrar
                        .fullIteration()
                        .leftTask(TASK_ID_LEFT_THREE)
                        .rightTask(TASK_ID_RIGHT_THREE)
                        .complete()
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_LEFT_THREE).results(result)
                        .to(TASK_ID_RIGHT_THREE).results(result.toBuilder().setTestStatus(TestStatus.TS_FAILED).build())
                        .toAll().finish()
        );

        metaIteration = storageTester.getIteration(metaIteration.getId());

        assertThat(metaIteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                StatisticsTemplates.mainMedium(StatisticsTemplates.stageTotal(1))
        );

        assertThat(metaIteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                ExtendedStatistics.builder().flaky(StatisticsTemplates.extendedFailedAdded(1)).build()
        );

        assertThat(metaIteration.getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS);
        assertThat(metaIteration.getNumberOfCompletedTasks()).isEqualTo(6);
        assertThat(metaIteration.getTestTypeStatistics().getNotCompleted()).isEqualTo(Set.of());
    }

    @Test
    public void firstFinishAfterSecond() {
        var registration = storageTester.register(registrar -> registrar.check()
                .heavyIteration()
                .leftTask(TASK_ID_LEFT)
                .rightTask(TASK_ID_RIGHT)
                .heavyIteration()
                .leftTask(TASK_ID_LEFT_TWO)
                .rightTask(TASK_ID_RIGHT_TWO)
                .complete()
        );

        var metaIterationId = registration.getIteration().getId().toMetaId();

        var resultOne = exampleLargeTestSuite(100L);
        var resultTwo = exampleLargeTestSuite(101L);

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_LEFT, TASK_ID_LEFT_TWO, TASK_ID_RIGHT_TWO)
                        .trace(DISTBUILD_STARTED)
                        .to(TASK_ID_LEFT_TWO, TASK_ID_RIGHT_TWO)
                        .results(resultTwo)
                        .pureFinish()
        );

        storageTester.assertIterationStatus(metaIterationId.toIterationId(2), CheckStatus.COMPLETED_SUCCESS);
        storageTester.assertIterationStatus(metaIterationId.toIterationId(1), CheckStatus.RUNNING);
        storageTester.assertIterationStatus(metaIterationId.toIterationId(0), CheckStatus.RUNNING);

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .toLeft().results(resultOne)
                        .toRight().results(resultOne.toBuilder().setTestStatus(TestStatus.TS_FAILED).build())
                        .to(TASK_ID_LEFT, TASK_ID_RIGHT).finish()
        );

        storageTester.assertIterationStatus(metaIterationId.toIterationId(2), CheckStatus.COMPLETED_SUCCESS);
        storageTester.assertIterationStatus(metaIterationId.toIterationId(1), CheckStatus.COMPLETED_FAILED);
        storageTester.assertIterationStatus(metaIterationId.toIterationId(0), CheckStatus.COMPLETED_FAILED);

        var metaIteration = storageTester.getIteration(metaIterationId);

        assertThat(metaIteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                StatisticsTemplates.mainLarge(
                        StageStatistics.builder()
                                .total(2)
                                .passed(1)
                                .failed(1)
                                .failedAdded(1)
                                .build()
                )
        );
    }
}
