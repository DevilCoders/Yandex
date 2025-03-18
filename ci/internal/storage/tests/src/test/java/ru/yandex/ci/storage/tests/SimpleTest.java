package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.tester.StatisticsTemplates;

import static org.assertj.core.api.Assertions.assertThat;

public class SimpleTest extends StorageTestsYdbTestBase {

    @Test
    public void startsIteration() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        storageTester.writeAndDeliver(writer -> writer.to(registration.getFirstLeft()).trace("distbuild/started"));
        storageTester.assertIterationStatus(registration.getIteration().getId(), Common.CheckStatus.RUNNING);
    }

    @Test
    public void singleResult() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        storageTester.writeAndDeliver(
                writer -> writer.to(registration.getTasks()).trace("distbuild/started").results(exampleBuild(1L))
        );

        storageTester.assertIterationStatus(registration.getIteration(), Common.CheckStatus.RUNNING);
        storageTester.assertIterationStatistics(
                registration.getIteration().getId(), StatisticsTemplates.mainBuild(StatisticsTemplates.stagePassed(1))
        );

        storageTester.writeAndDeliver(writer -> writer.to(registration.getFirstLeft()).finish());
        storageTester.assertIterationStatus(registration.getIteration().getId(), Common.CheckStatus.RUNNING);
        storageTester.writeAndDeliver(writer -> writer.to(registration.getFirstRight()).finish());
        storageTester.assertIterationStatus(registration.getIteration().getId(), Common.CheckStatus.COMPLETED_SUCCESS);
        storageTester.executeAllOnetimeTasks();
    }

    @Test
    public void testenvResult() {
        var registration = storageTester.register(registrar -> registrar.testenvIteration().leftTask().rightTask());

        var leftTask = registration.getFirstLeft();
        var rightTask = registration.getFirstRight();

        var updatedIteration = CheckProtoMappers.toCheckIteration(
                storageTester.api().allowTestenvFinish(
                        CheckProtoMappers.toProtoIterationId(registration.getIteration().getId())
                )
        );

        var registeredExpectedTasks = updatedIteration.getRegisteredExpectedTasks();

        assertThat(registeredExpectedTasks).hasSize(1);
        assertThat(registeredExpectedTasks.iterator().next().getJobName()).isEqualTo("te-finish-1");

        var teResult = exampleTest(1L, 1L, Common.ResultType.RT_TEST_TESTENV);

        storageTester.writeAndDeliver(
                writer -> writer.to(leftTask, rightTask).results(teResult)
        );

        logbrokerService.deliverAllMessages();

        storageTester.assertIterationStatistics(
                registration.getIteration().getId(),
                MainStatistics.builder()
                        .total(StatisticsTemplates.stagePassed(1))
                        .teTests(StatisticsTemplates.stagePassed(1))
                        .build()
        );

        storageTester.writeAndDeliver(writer -> writer.to(leftTask).finish());

        assertThat(storageTester.getIteration(registration.getIteration().getId()).getInfo().getProgress())
                .isEqualTo(50);

        storageTester.writeAndDeliver(writer -> writer.to(rightTask).finish());

        storageTester.assertIterationStatus(registration.getIteration(), Common.CheckStatus.COMPLETED_SUCCESS);
    }
}
