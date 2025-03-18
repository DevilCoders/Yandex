package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.tests.tester.StatisticsTemplates;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.core.db.model.test.TestTag.YA_EXTERNAL;

public class StatisticsTest extends StorageTestsYdbTestBase {
    @Test
    public void suiteFailedResultFlaky() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var suiteResult = exampleSuite(100L);
        var testResult = exampleMediumTest(1L, 100L);

        storageTester.writeAndDeliver(writer ->
                writer.to(leftTask)
                        .trace("distbuild/started")
                        .results(suiteResult.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .to(rightTask).results(
                                suiteResult,
                                testResult.toBuilder().setTestStatus(Common.TestStatus.TS_FLAKY).build()
                        )
                        .to(leftTask, rightTask)
                        .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        assertThat(iteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1) // suite
                                        .passedAdded(1) // suite
                                        .build()
                        )
                        .mediumTests(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(1) // suite
                                        .passedAdded(1) // suite
                                        .build()
                        )
                        .build()
        );

        assertThat(iteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                ExtendedStatistics.builder()
                        .flaky(ExtendedStageStatistics.builder().total(1).build())
                        .build()

        );
    }

    @Test
    public void withStrongMode() {
        this.aYamlerClient.enableStrongMode();

        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var suiteResult = exampleSuite(100L);

        storageTester.writeAndDeliver(writer ->
                writer
                        .to(leftTask)
                        .results(suiteResult)
                        .to(rightTask)
                        .results(suiteResult.toBuilder().setTestStatus(Common.TestStatus.TS_FLAKY).build())
                        .to(leftTask, rightTask)
                        .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());
        var statistics = iteration.getStatistics().getAllToolchain();
        assertThat(statistics.getMain().getMediumTestsOrEmpty().getFailedAdded()).isEqualTo(1);
        assertThat(statistics.getExtended().getFlakyOrEmpty().getFailedAdded()).isEqualTo(1);
    }

    @Test
    public void chunkFailed() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var suiteResult = exampleSuite(100L);
        var chunkResult = exampleMediumTest(2L, suiteResult.getId().getHid());
        var testResult = exampleMediumTest(1L, suiteResult.getId().getHid());

        storageTester.writeAndDeliver(writer -> writer
                .to(leftTask)
                .trace("distbuild/started")
                .results(suiteResult, chunkResult, testResult.toBuilder().setChunkHid(2L).build())
                .finish()
                .to(rightTask)
                .results(suiteResult, chunkResult.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        var statistics = StageStatistics.builder()
                .total(3) // suite, chunk, test
                .failed(1) // chunk
                .failedAdded(1) // chunk
                .passed(1) // suite
                .build();

        assertThat(iteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(statistics)
                        .mediumTests(statistics)
                        .build()
        );

        assertThat(iteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                ExtendedStatistics.EMPTY

        );
    }

    @Test
    public void externalFailedToXPassed() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var testResult = exampleMediumTest(1L, 100L);

        var suiteResult = exampleSuite(100L);

        storageTester.writeAndDeliver(
                writer -> writer.to(leftTask, rightTask).results(suiteResult)
                        .to(leftTask).results(testResult.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .to(rightTask).results(
                                testResult.toBuilder()
                                        .addTags(YA_EXTERNAL)
                                        .setTestStatus(Common.TestStatus.TS_XFAILED)
                                        .build()
                        )
                        .to(leftTask, rightTask).finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        assertThat(iteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder()
                        .total(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(2)
                                        .build()
                        )
                        .mediumTests(
                                StageStatistics.builder()
                                        .total(2)
                                        .passed(2)
                                        .build()
                        )
                        .build()
        );

        assertThat(iteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                ExtendedStatistics.builder()
                        .external(
                                ExtendedStageStatistics.builder()
                                        .total(1)
                                        .passedAdded(1)
                                        .build()
                        )
                        .build()
        );
    }

    @Test
    public void buildFailureAlwaysInMainStatistics() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var result = exampleBuild(1L);

        storageTester.writeAndDeliver(
                writer -> writer
                        .to(leftTask).results(result.toBuilder().setTestStatus(Common.TestStatus.TS_OK).build())
                        .to(rightTask).results(
                                result.toBuilder().setTestStatus(Common.TestStatus.TS_TIMEOUT).build()
                        )
                        .to(leftTask, rightTask).finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var stage = StatisticsTemplates.stageFailedAdded(1);

        assertThat(iteration.getStatistics().getAllToolchain().getMain()).isEqualTo(
                MainStatistics.builder().total(stage).build(stage).build()
        );

        assertThat(iteration.getStatistics().getAllToolchain().getExtended()).isEqualTo(
                StatisticsTemplates.timeoutAdded(1)
        );
    }

    @Test
    public void externalCountsAsPassedOnRightOk() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var leftTask = registration.getTask(TASK_ID_LEFT);
        var rightTask = registration.getTask(TASK_ID_RIGHT);

        var result = exampleTest(1L, 100L, Common.ResultType.RT_TEST_MEDIUM).toBuilder()
                .addTags(YA_EXTERNAL).build();

        storageTester.writeAndDeliver(
                writer -> writer
                        .to(leftTask).results(result.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .to(rightTask).results(result)
                        .to(leftTask, rightTask).finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var stage = StatisticsTemplates.extendedPassedAdded(1);

        assertThat(iteration.getStatistics().getAllToolchain().getExtended().getExternalOrEmpty())
                .isEqualTo(stage);

        assertThat(iteration.getStatistics().getAllToolchain().getMain().getTotalOrEmpty().getPassed())
                .isEqualTo(1);

        var suites = storageTester.frontApi().searchSuites(
                iteration.getId(),
                StorageFrontApi.SuiteSearch.newBuilder()
                        .setToolchain("@all")
                        .addResultType(Common.ResultType.RT_TEST_MEDIUM)
                        .setCategory(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .setStatusFilter(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .build()
        ).getSuitesList();

        assertThat(suites).hasSize(1);
        assertThat(suites.get(0).getStatistics().getPassed()).isEqualTo(1);
        assertThat(suites.get(0).getStatistics().getFailed()).isEqualTo(0);
    }

    @Test
    public void notLaunchedLargeTest() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        var result = exampleLargeTestSuite(100L).toBuilder().setTestStatus(Common.TestStatus.TS_NOT_LAUNCHED).build();
        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result).finish());
        var iteration = storageTester.getIteration(registration.getIteration().getId());
        assertThat(iteration.getStatistics().getAllToolchain().getMain().getTotalOrEmpty().getSkipped()).isEqualTo(1);
    }

    @Test
    public void nativeTest() {
        var registration = storageTester.register(registrar -> registrar.check()
                .heavyIteration()
                .task(TASK_ID_LEFT, 1, false, Common.CheckTaskType.CTT_NATIVE_BUILD)
                .task(TASK_ID_RIGHT, 1, true, Common.CheckTaskType.CTT_NATIVE_BUILD)
        );

        var result = exampleNativeBuild(1L);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result).finish());

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var statistics = iteration.getStatistics().getAllToolchain().getMain().getNativeBuildsOrEmpty();
        assertThat(statistics.getTotal()).isEqualTo(1);
        assertThat(statistics.getPassed()).isEqualTo(1);
    }
}
